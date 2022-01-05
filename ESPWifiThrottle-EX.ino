#include <ESP8266WiFi.h>

const char* ssid = "wifi-network-name";
const char* password = "wifi-netowrk-password";

// CommandStation-EX connection details.
byte ip[] = { 192, 168, 0, 181 };
int port = 2560;

// Rotary encoder inputs.
#define CLK 13
#define DT 12
#define SW 2

WiFiClient client;

// State variables for rotary encoder.
int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  delay(1000);

  Serial.println("Startup." );

  initWiFi();

  // Pins for the rotary encoder.
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);
}

int flip = 0;

void loop() {
  // if the client is no longer connected, the stop the client and reconnect.
  if ( !client.connected() ) {
    client.stop();
    Serial.print("Connecting to CommandStation-DX...");
    if ( client.connect( ip, port ) ) {
      client.setNoDelay(true);
      Serial.println("Connected.");
    } else {
      Serial.println("Connection Failed");
    }
  } else {
    // Serial.println("Connected to CommandStation-DX.");
  }

  // Handled the rotary encoder.
  // Read the current state of CLK
  currentStateCLK = digitalRead(CLK);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1) {

    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(DT) != currentStateCLK) {
      counter ++;
      currentDir = "CCW";
    } else {
      // Encoder is rotating CW so increment
      counter --;
      currentDir = "CW";
    }

    // Proof of concept. Prove that we can send a throttle command message
    // to the CommandStation-DX and that locomotive 7036 will respond.
    String s = "<t 0 7036 ";    // address loco #7036
    s.concat( abs( counter ) ); // loco speed
    s.concat( " " );
    if ( counter >= 0 ) { // loco direction
      s.concat( 1 );      // forward
    } else {
      s.concat( 0 );      // reverse
    }
    s.concat( ">" );
    Serial.println(s);
    client.println(s);
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;

  // Read the button state
  int btnState = digitalRead(SW);

  //If we detect LOW signal, button is pressed
  if (btnState == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50) {

      // Proof of concept. Prove that we can send a throttle command message
      // to the CommandStation-DX and that turns track power on and off.
      Serial.println("Click!");
      if ( flip == 0 ) {
        client.println("<1 MAIN>");
        flip = 1;
      } else {
        client.println("<0 MAIN>");
        flip = 0;
      }
    }

    // Remember last button press event
    lastButtonPress = millis();
  }

  // Put in a slight delay to help debounce the readings
  delay(1);

  // if there's a response from the service, read and print it out.
  while ( client.available() ) {
    char c = client.read();
    Serial.print(c);
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi." );

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
