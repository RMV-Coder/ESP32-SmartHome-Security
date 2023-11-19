//ESP32-CAM Livestreaming with Remote Webserver and Websockets for SmartHome Security
/*
  TO DO: 
  1. Implement Servo Control for Angle Control - DONE
  2. Create Google Cloud Platform (GCP) VirtualMachine Webserver and Connect - DONE
  3. Implement SPIFFS or SD Card Saving of Videos and Photos
  4. Create a Website with HTML, CSS and JavaScript. - Almost DONE
  Optional:
  5. Add Joystick for Angle control
  6. Add solar panel with batteries and charging board

 */

#include "esp_camera.h"
#include "camera_pins.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#define servoTiltPin 12 //change accordingly
#define servoPanPin 13 //change accordingly
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
int panValue = 0;
int tiltValue = 0;
int flashValue = 0;
const char* websocket_server_host = "35.185.186.229";
const uint16_t websocket_server_port = 65080;
const size_t jsonBufferSize = 1024;
const uint64_t deepSleepTime = 30 * 60 * 1e6; // 30 minutes in microseconds
using namespace websockets;

WebsocketsClient cameraClient;  //Websocket connection to /camera path
WebsocketsClient controlClient; //Websocket connection to /control path
Servo servoPan; //horizontal servo control
Servo servoTilt;//vertical servo control

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
  //small servo 500, 2400 / large servo 1000, 2000
  servoPan.attach(servoPanPin, 500, 2400);
  servoTilt.attach(servoTiltPin, 500, 2400);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(LED_GPIO_NUM, OUTPUT);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_CIF;//CIF,VGA,SVGA
    config.jpeg_quality = 10;//0-63, 0 highest - 63 lowest
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

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
  

  while(!cameraClient.connect(websocket_server_host, websocket_server_port, "/camera")){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to camera websocket server!");
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
      flashValue = doc["FLASH"];
      Serial.print("PAN: ");
      Serial.println(panValue);
      
      Serial.print("TILT: ");
      Serial.println(tiltValue);

      Serial.print("FLASH: ");
      Serial.println(flashValue);
      analogWrite(LED_GPIO_NUM, flashValue);
      //controlClient.send(message.data());
      moveServo(panValue, tiltValue);
    }
  });
}

void loop() {

  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }

  if(fb->format != PIXFORMAT_JPEG){
    Serial.println("Non-JPEG data not implemented");
    return;
  }

  if(controlClient.available()){
    controlClient.poll();
  }
  
  cameraClient.sendBinary((const char*) fb->buf, fb->len);
  esp_camera_fb_return(fb);
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
  } else if (!cameraClient.connect(websocket_server_host, websocket_server_port, "/camera")) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastConnectionAttempt >= minRetryInterval) {
      Serial.print("Attempting to reconnect to camera websocket server... ");
      if (retryCountCamera < maxRetryCount) {
        if (cameraClient.connect(websocket_server_host, websocket_server_port, "/camera")) {
          Serial.println("Reconnected to camera websocket server!");
          retryCountCamera = 0; // Reset retry count upon successful reconnection
        } else {
          Serial.println("Connection failed.");
          // Implement error handling if needed
          retryCountCamera++;
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
void moveServo(int pan, int tilt){
  servoPan.write(pan);
  servoTilt.write(tilt);
  delay(20);
}
void enterDeepSleep() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}