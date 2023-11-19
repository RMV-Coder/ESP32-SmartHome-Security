#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#define servoTiltPin 4 //change accordingly
#define servoPanPin 13 //change accordingly
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

int panValue = 0;
int tiltValue = 0;

const char* websocket_server_host = "35.185.186.229";
const uint16_t websocket_server_port = 65080;
const size_t jsonBufferSize = 1024;
const uint64_t deepSleepTime = 30 * 60 * 1e6; // 30 minutes in microseconds
using namespace websockets;
WebsocketsClient controlClient;

Servo servoPan; //horizontal control
Servo servoTilt;//vertical control
void setup() {
  WiFi.mode(WIFI_STA);
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
  servoPan.write(0);
  servoTilt.write(0);
  Serial.begin(115200);

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
      sendData(panValue, tiltValue);
      //controlClient.send(message.data());
      moveServo(panValue, tiltValue);
    }
  });
}

void loop() {
  // put your main code here, to run repeatedly:
  if(controlClient.available()){
    controlClient.poll();
    servoPan.write(panValue);
    servoTilt.write(tiltValue);
  }
  delay(20);

  static unsigned long lastConnectionAttempt = 0;
  static int retryCountCamera = 0, retryCountControl = 0;
  const int maxRetryCount = 5; // Adjust as needed
  unsigned long minRetryInterval = 500; // Initial retry interval in milliseconds
  const unsigned long maxRetryInterval = 60000; // Maximum retry interval in milliseconds

  if (!controlClient.connect(websocket_server_host, websocket_server_port, "/control")) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastConnectionAttempt >= minRetryInterval) {
      Serial.print("Attempting to reconnect to control websocket server... ");
      if (retryCountControl < maxRetryCount) {
        if (controlClient.connect(websocket_server_host, websocket_server_port, "/control")) {
          Serial.println("Reconnected to control websocket server!");
          retryCountControl = 0; // Reset retry count upon successful reconnection
        } else {
          Serial.println("Connection failed.");
          // Implement error handling if needed
          retryCountControl++;
          lastConnectionAttempt = currentMillis;
          // Exponential backoff with a maximum interval
          minRetryInterval = min(minRetryInterval * 2, maxRetryInterval);
        }
      } else {
        Serial.println("Maximum retry count reached. Giving up.");
        // Enter deep sleep mode before retrying
        enterDeepSleep();
      }
    }
  }
}
void sendData(int panVal, int tiltVal){
  String  JSON_data = "{\"PAN\":";
          JSON_data+= panVal;
          JSON_data+= ",\"TILT\":";
          JSON_data+= tiltVal;
          JSON_data+= ",\"Sender\":";
          JSON_data+= "ESP32-S3";
          JSON_data+= "}";
  Serial.print("Sent data: ");
  Serial.println(JSON_data);
  controlClient.send(JSON_data);
}
void moveServo(int pan, int tilt){
  servoPan.write(pan);
  servoTilt.write(tilt);
  delay(10);
}
void enterDeepSleep() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}
