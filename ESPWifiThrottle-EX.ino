#include <ESP8266WiFi.h>
#include <Wire.h>     // for I2C 
#include <LCD_I2C.h>  // for LCD
#include <PCF8574.h>  // for PCF8574 I/O board

#include "Encoder.h"

// Rotary encoder inputs.
#define CLK 13
#define DT 12
#define SW 14

// Pinout for I/O expander board.
#define PFC_ESTOP_PIN 0
#define PFC_FUNC_LIGHT_PIN 1
#define PFC_FUNC_BELL_PIN 2
#define PFC_FUNC_HORN_PIN 3
#define PFC_FWD_PIN 4
#define PFC_REV_PIN 5

// Wifi network details.
const char* ssid = "wifi-network-name";
const char* password = "wifi-netowrk-password";

// CommandStation-EX connection details.
byte ip[] = { 192, 168, 0, 180 };
int port = 2560;

WiFiClient client;

LCD_I2C lcd = LCD_I2C(0x27, 16, 2);

PCF8574 pfc = PCF8574();

Encoder encoder( CLK, DT, SW );

int cab_number[4] = { 7036, 6792, 333, 339 };
int cab_speed[4] = { 0, 0 };
int cab_direction[4] = { 1, 1 };
int cab_f0[4] = { 0, 0 };

int cab_idx = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  delay(1000);

  lcd.begin();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  Serial.println("Initializing... " );
  lcd.print("Initializing... ");

  Serial.println("PFC I/O Board..." );
  lcd.setCursor(0, 1); lcd.print("PFC I/O Board...");

  if (!pfc.begin())
  {
    Serial.println("pfc not initialized");
  }
  if (!pfc.isConnected())
  {
    Serial.println("pfc not connected");
    while (1);
  }

  Serial.println("Rotary Encoder.." );
  lcd.setCursor(0, 1); lcd.print("Rotary Encoder..");

  encoder.init();

  Serial.println("Wifi...         " );
  lcd.setCursor(0, 1); lcd.print("Wifi...         ");

  initWiFi();
}

#define PFC_TRUE 0
#define PFC_FALSE 1

uint8_t last_estop_state = PFC_FALSE;
uint8_t last_f0_state = PFC_FALSE;
uint8_t last_fwd_state = PFC_FALSE;
uint8_t last_rev_state = PFC_FALSE;
uint8_t last_brk_state = PFC_FALSE;

unsigned long last_brk_time = 0;

int last_encoder_bt_state = 0;

void loop() {
  // if the client is no longer connected, the stop the client and reconnect.
  if ( !client.connected() ) {
    client.stop();
    Serial.print("Connect to DCC-DX...");
    lcd.setCursor(0, 1); lcd.print("Connect DCC-EX..");
    if ( client.connect( ip, port ) ) {
      client.setNoDelay(true);
      Serial.println("Connected.");
    } else {
      Serial.println("Connection Failed");
    }
  } else {
    // Serial.println("Connected to CommandStation-DX.");
  }

  //
  // Check inputs
  //
  int b_buff = 0;
  String clientCommand = "";

  // Emergency stop
  b_buff = pfc.read(PFC_ESTOP_PIN);
  if (b_buff == 0 && b_buff != last_estop_state) {
    clientCommand = "<!>";
    Serial.println(clientCommand);
    client.println(clientCommand);
    encoder.write( 0 );
    int len = (sizeof(cab_speed) / sizeof(cab_speed[0])) - 1;
    for (int i=0; i<len; i++) {
      cab_speed[i] = 0;
    }
    lcd.setCursor(0, 1); lcd.print("Emergency Stop! ");
  }
  last_estop_state = b_buff;


  // F0 Headlight
  b_buff = pfc.read(PFC_FUNC_LIGHT_PIN);
  if (b_buff == 0 && b_buff != last_f0_state) {
    cab_f0[cab_idx] = !cab_f0[cab_idx];
    clientCommand = "<F ";
    clientCommand.concat(cab_number[cab_idx]);
    clientCommand.concat(" 0 ");
    clientCommand.concat(cab_f0[cab_idx]);
    clientCommand.concat(">");
    Serial.println(clientCommand);
    client.println(clientCommand);
    lcd.setCursor(0, 1); lcd.print("Light ON/OFF    ");
  }
  last_f0_state = b_buff;

  // Direction
  // Forward
  b_buff = pfc.read(PFC_FWD_PIN);
  if (b_buff == 0 && b_buff != last_fwd_state) {
    last_brk_state = PFC_FALSE;
    cab_direction[cab_idx] = 1;
    Serial.println("Forward!");
    txThrottleCommand();
    lcd.setCursor(0, 1); lcd.print("Forward!        ");
  }
  last_fwd_state = b_buff;

  // Direction
  // Reverse
  b_buff = pfc.read(PFC_REV_PIN);
  if (b_buff == 0 && b_buff != last_rev_state) {
    last_brk_state = PFC_FALSE;
    cab_direction[cab_idx] = 0;
    Serial.println("Reverse!");
    txThrottleCommand();
    lcd.setCursor(0, 1); lcd.print("Reverse!        ");
  }
  last_rev_state = b_buff;

  // Direction
  // Brake
  if ( last_fwd_state == PFC_FALSE && last_rev_state == PFC_FALSE && cab_speed[cab_idx] > 0 && millis() - last_brk_time > 250 ) {
    Serial.println("Brake!");
    last_brk_time = millis();
    // BRAKING CYCLE Incremental decrease throttle to zero.
    // To Do: Prevent change of direction until breaking cycle is complete.
    cab_speed[cab_idx] = cab_speed[cab_idx] - 1;
    encoder.write( cab_speed[cab_idx] );
    txThrottleCommand();
    lcd.setCursor(0, 1); lcd.print("Braking!        ");
  }

  // Throttle Mode
  // Encoder rotation
  encoder.loop();
  b_buff = encoder.read();
  if ( b_buff != cab_speed[cab_idx] ) {
    cab_speed[cab_idx] = b_buff;
    if (b_buff < 0) {
      encoder.write( 0 );
      cab_speed[cab_idx] = 0;
    }
    if (b_buff > 28) {
      encoder.write( 28 );
      cab_speed[cab_idx] = 28;
    }
    txThrottleCommand();
  }

  // Encoder click
  // Read the button state
  b_buff = digitalRead(SW);
  if (b_buff == LOW && b_buff != last_encoder_bt_state ) {
    clientCommand = "<1 MAIN>";
    Serial.println(clientCommand);
    client.println(clientCommand);
    lcd.setCursor(0, 1); lcd.print("Track Power ON  ");
  }
  last_encoder_bt_state = b_buff;

  // Menu Mode

  // if there's a response from the service, read and print it out.
  while ( client.available() ) {
    char c = client.read();
    Serial.print(c);
  }
}

void txThrottleCommand() {
  String clientCommand = "";
  clientCommand = "<t 1 ";
  clientCommand.concat(cab_number[cab_idx]);
  clientCommand.concat(" ");
  clientCommand.concat(cab_speed[cab_idx]);
  clientCommand.concat(" ");
  clientCommand.concat(cab_direction[cab_idx]);
  clientCommand.concat(">");
  Serial.println(clientCommand);
  client.println(clientCommand);
  lcd.setCursor(0, 0); lcd.print(clientCommand);
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connect to WiFi..." );

  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print(".");
    delay(1000);
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  Serial.println();

  Serial.print("Connected, network SSID: ");
  Serial.println(WiFi.SSID());

  Serial.print("Connected, MAC address: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Connected, network RSSI: ");
  Serial.println(WiFi.RSSI());
}
