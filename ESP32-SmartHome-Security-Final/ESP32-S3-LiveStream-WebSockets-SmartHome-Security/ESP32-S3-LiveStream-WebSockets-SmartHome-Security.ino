#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#define servoTiltPin 4 //change accordingly
#define servoPanPin 13 //change accordingly

int panValue = 0;
int tiltValue = 0;
int wifi_timeout = 120; //120 seconds timeout
int wifi_connection_attempts = 0;

const char* websocket_server_host = "35.185.186.229";
const uint16_t websocket_server_port = 65080;
const size_t jsonBufferSize = 1024;

using namespace websockets;
WebsocketsClient controlClient;  //Websocket connection to /control path

Servo servoPan; //horizontal control
Servo servoTilt;//vertical control
void setup() {
  //WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool result;

  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  servoPan.setPeriodHertz(50);      // Standard 50hz servo
	servoTilt.setPeriodHertz(50);

  servoPan.attach(servoPanPin, 500, 2400);
  servoTilt.attach(servoTiltPin, 500, 2400);
  servoPan.write(panValue);
  servoTilt.write(tiltValue);
  Serial.begin(115200);

wm.setConfigPortalTimeout(wifi_timeout);
  //wm.resetSettings();
  wm.preloadWiFi("Walay Tulog","prinniegwapo");
  result = wm.autoConnect("ESP32CAM","12345678"); // password protected ap

  if(!result) {
    Serial.println("Failed to connect");
    //ESP.restart();
  }  
  else{
    Serial.println("Connected to WiFi Successfully!");
  }

  while(!controlClient.connect(websocket_server_host, websocket_server_port, "/control")){
    delay(500);
    Serial.print(".");
    wifi_connection_attempts++;
    if(wifi_connection_attempts == 4){
      Serial.print("initiating ESP restart to refresh...");
      ESP.restart();
    }
  }
  Serial.println("Connected to control websocket server!");
  controlClient.onMessage([&](WebsocketsMessage message){
    if (message.isText()) {
      Serial.print("Received JSON Message: ");
      Serial.println(message.data());
      DynamicJsonDocument doc(200);
      DeserializationError error = deserializeJson(doc, message.data());

      if (error) {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        return; // Handle the parsing error as needed
      }

      // Access JSON data
      panValue = doc["PAN"];
      tiltValue = doc["TILT"];
      Serial.print("PAN: ");
      Serial.println(panValue);
      
      Serial.print("TILT: ");
      Serial.println(tiltValue);

      moveServo(panValue, tiltValue);
    }
  });
}

void loop() {
  // put your main code here, to run repeatedly:
  if(controlClient.available()){
    controlClient.poll();
  }
  delay(20);
}

void moveServo(int pan, int tilt){
  servoPan.write(pan);
  servoTilt.write(tilt);
  delay(50);
}