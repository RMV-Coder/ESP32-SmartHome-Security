//ESP32-CAM Livestreaming with Remote Webserver and Websockets for SmartHome Security
/*
  TO DO: 
  1. Implement Servo Control for Angle Control - DONE
  2. Create Google Cloud Platform (GCP) VirtualMachine Webserver and Connect - DONE
  3. Implement SPIFFS or SD Card Saving of Videos and Photos
  4. Create a Website with HTML, CSS and JavaScript. - DONE
  Optional:
  5. Add Joystick for Angle control
  6. Add solar panel with batteries and charging board

 */

#include "esp_camera.h"
#include "camera_pins.h"
#include "time.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"
#include "esp_log.h"
extern "C" {
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
}  // Disable brownout problems
#include "driver/rtc_io.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

const char* ntpServer = "asia.pool.ntp.org"; // or use pool.nyp.org
const long  gmtOffset_sec = 28800; // UTC +8:00 >> 8 * 60 * 60 = 28800
const int   daylightOffset_sec = 0;

#define servoTiltPin 12 //change accordingly
#define servoPanPin 13 //change accordingly
int panValue = 0;
int tiltValue = 0;
int powerValue = 1;
int isCapture = 0;
int isRecording = 0;
int isNowStreaming = 0;
int wifi_timeout = 120; //120 seconds timeout
int wifi_connection_attempts = 0;
int quality = 15, saturation=1, contrast=2, brightness=2, awb=1, awb_gain=1,wb_mode=0, aec=1, aec2=1, ae_level=0, agc=1, gainceiling=1, bpc=1, wpc=1, raw_gma=1, lenc=1, dcw=1, special_effect=0;
//int quality = 15;//10-20
const char* websocket_server_host = "35.185.186.229";
const uint16_t websocket_server_port = 65080;
const size_t jsonBufferSize = 1024;
using namespace websockets;
camera_config_t initConfig;
WebsocketsClient cameraClient;  //Websocket connection to /camera path
WebsocketsClient controlClient; //Websocket connection to /control path
Servo servoPan; //horizontal servo control
Servo servoTilt;//vertical servo control

void setSensor(){
  sensor_t *s = esp_camera_sensor_get();
  s->set_quality(s, quality);
  s->set_contrast(s, contrast);
  s->set_brightness(s, brightness);
  s->set_saturation(s, saturation);
  s->set_gainceiling(s, (gainceiling_t)gainceiling);
  s->set_whitebal(s, awb);
  s->set_gain_ctrl(s, agc);
  s->set_exposure_ctrl(s, aec);
  s->set_awb_gain(s, awb_gain);
  s->set_aec2(s, aec2);
  s->set_dcw(s, dcw);
  s->set_bpc(s, bpc);
  s->set_wpc(s, wpc);
  s->set_raw_gma(s, raw_gma);
  s->set_lenc(s, lenc);
  s->set_special_effect(s, special_effect);
  s->set_wb_mode(s, wb_mode);
  s->set_ae_level(s, ae_level);
  delay(100);
}
void initialCameraConfig(){
  initConfig.ledc_channel = LEDC_CHANNEL_0;
  initConfig.ledc_timer = LEDC_TIMER_0;
  initConfig.pin_d0 = Y2_GPIO_NUM;
  initConfig.pin_d1 = Y3_GPIO_NUM;
  initConfig.pin_d2 = Y4_GPIO_NUM;
  initConfig.pin_d3 = Y5_GPIO_NUM;
  initConfig.pin_d4 = Y6_GPIO_NUM;
  initConfig.pin_d5 = Y7_GPIO_NUM;
  initConfig.pin_d6 = Y8_GPIO_NUM;
  initConfig.pin_d7 = Y9_GPIO_NUM;
  initConfig.pin_xclk = XCLK_GPIO_NUM;
  initConfig.pin_pclk = PCLK_GPIO_NUM;
  initConfig.pin_vsync = VSYNC_GPIO_NUM;
  initConfig.pin_href = HREF_GPIO_NUM;
  initConfig.pin_sscb_sda = SIOD_GPIO_NUM;
  initConfig.pin_sscb_scl = SIOC_GPIO_NUM;
  initConfig.pin_pwdn = PWDN_GPIO_NUM;
  initConfig.pin_reset = RESET_GPIO_NUM;
  initConfig.xclk_freq_hz = 10000000;
  initConfig.pixel_format = PIXFORMAT_JPEG;
}
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}
void setup() {
  //WiFi.mode(WIFI_STA);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  WiFiManager wm;
  bool result;
  /*uncomment for servos
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  servoPan.setPeriodHertz(50);      // Standard 50hz servo
	servoTilt.setPeriodHertz(50);
  //small servo 500, 2400 / large servo 1000, 2000
  servoPan.attach(servoPanPin, 500, 2400);
  servoTilt.attach(servoTiltPin, 500, 2400);
  */
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(LED_GPIO_NUM, OUTPUT);
  
  initialCameraConfig();
  
  wm.setConfigPortalTimeout(wifi_timeout);
  //wm.resetSettings();
  //wm.preloadWiFi("Walay Tulog","prinniegwapo");
  result = wm.autoConnect("ESP32CAM","12345678"); // password protected ap

  if(!result) {
    Serial.println("Failed to connect");
    //ESP.restart();
  }  
  else{
    Serial.println("Connected to WiFi Successfully!");
  }
  //Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    if(WiFi.RSSI() < -65){//Weak WiFi
      initConfig.frame_size = FRAMESIZE_CIF;//CIF,VGA,SVGA
      initConfig.jpeg_quality = 18;//0-63, 0 highest - 63 lowest
      initConfig.fb_count = 2;
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(", Frame Size: CIF, Quality: 18, FB Count: 2");
    } else if(WiFi.RSSI() >= -65 && WiFi.RSSI() <= -55){//Decent WiFi
      initConfig.frame_size = FRAMESIZE_VGA;//CIF,VGA,SVGA
      initConfig.jpeg_quality = 20;//0-63, 0 highest - 63 lowest
      initConfig.fb_count = 2;
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(", Frame Size: VGA, Quality: 20, FB Count: 2");
    } else if(WiFi.RSSI() >= -54 && WiFi.RSSI() <= -48){//Good WiFi
      initConfig.frame_size = FRAMESIZE_VGA;//CIF,VGA,SVGA
      initConfig.jpeg_quality = 16;//0-63, 0 highest - 63 lowest
      initConfig.fb_count = 2;
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(", Frame Size: VGA, Quality: 16, FB Count: 2");
    } else if(WiFi.RSSI() > -47){//Very Strong WiFi
      initConfig.frame_size = FRAMESIZE_SVGA;//CIF,VGA,SVGA
      initConfig.jpeg_quality = 18;//0-63, 0 highest - 63 lowest
      initConfig.fb_count = 2;
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(", Frame Size: SVGA, Quality: 18, FB Count: 2");
    }
  } else {
    if(WiFi.RSSI() < -65){//Weak WiFi
      initConfig.frame_size = FRAMESIZE_CIF;//CIF,VGA,SVGA
      initConfig.jpeg_quality = 25;//0-63, 0 highest - 63 lowest
      initConfig.fb_count = 1;
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(", Frame Size: CIF, Quality: 25, FB Count: 1");
    } else if(WiFi.RSSI() >= -65 && WiFi.RSSI() <= -48){//Good WiFi
     initConfig.frame_size = FRAMESIZE_CIF;//CIF,VGA,SVGA
     initConfig.jpeg_quality = 20;//0-63, 0 highest - 63 lowest
     initConfig.fb_count = 1;
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(", Frame Size: CIF, Quality: 20, FB Count: 1");
    } else if(WiFi.RSSI() > -47){//Very Strong WiFi
     initConfig.frame_size = FRAMESIZE_CIF;//CIF,VGA,SVGA
     initConfig.jpeg_quality = 15;//0-63, 0 highest - 63 lowest
     initConfig.fb_count = 1;
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(", Frame Size: CIF, Quality: 15, FB Count: 1");
    }
  }
  // camera init
  esp_err_t err = esp_camera_init(&initConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  setSensor();
  //Serial.println("Starting SD Card");
  if(!SD_MMC.begin()){ 
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
  Serial.print("SD_MMC Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }
  listDir(SD_MMC, "/", 0);
  // createDir(SD_MMC, "/captures");
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
  while(!cameraClient.connect(websocket_server_host, websocket_server_port, "/camera")){
    delay(500);
    Serial.print(".");
    wifi_connection_attempts++;
    if(wifi_connection_attempts == 4){
      Serial.print("initiating ESP restart to refresh...");
      ESP.restart();
    }
  }
  Serial.println("Connected to camera websocket server!");
  wifi_connection_attempts = 0;
  isNowStreaming = 1;
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
  wifi_connection_attempts = 0;
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
      powerValue = doc["POWER"];
      isCapture = doc["CAPTURE"];
      isNowStreaming = doc["STREAM"];
      Serial.print("PAN: ");
      Serial.println(panValue);
      
      Serial.print("TILT: ");
      Serial.println(tiltValue);

      Serial.print("POWER: ");
      Serial.println(powerValue);

      Serial.print("CAPTURE: ");
      Serial.println(isCapture);

      Serial.print("STREAM: ");
      Serial.println(isNowStreaming);
      //analogWrite(LED_GPIO_NUM, flashValue);

      if(isCapture == 1){
        Serial.println("Capturing image...");
        isNowStreaming = 0;
        capture();
        isCapture = 0;
        delay(1000);
        isNowStreaming = 1;
      }

      if(isNowStreaming == 1){
        //Stream Video
      }
      if(powerValue == 0){
        WiFi.disconnect();
        esp_camera_deinit();
        ESP.restart();
      }
      //controlClient.send(message.data());
      moveServo(panValue, tiltValue);
    }
  });
}

void loop() {
  if(isNowStreaming == 1){
    camera_fb_t *fb = esp_camera_fb_get();
    if(!fb){
      Serial.println("Camera capture failed");
      esp_camera_fb_return(fb);
      return;
    }

    // if(fb->format != PIXFORMAT_JPEG){
    //   Serial.println("Non-JPEG data not implemented");
    //   return;
    // }
  
    cameraClient.sendBinary((const char*) fb->buf, fb->len);
    esp_camera_fb_return(fb);
    delay(5);
  }
  if(controlClient.available()){
    controlClient.poll();
  } else {
    //Initiate ESP Restart to reconnect to server.
    Serial.println("Got disconnected from Control WebSocket Server, will initiate ESP Restart...");
    ESP.restart();
  }
  if(!cameraClient.available()){
    //Initiate ESP Restart to reconnect to server.
    Serial.println("Got disconnected from Camera WebSocket Server, will initiate ESP Restart...");
    ESP.restart();
  }
}
void writeFile(fs::FS &fs, const char *path, const char *buffer, size_t length) {
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    } else {
      Serial.println("Successfully opened file for writing.");
    }
    if (file.write(reinterpret_cast<const uint8_t*>(buffer), length)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}


void capture(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time.");
    ESP.restart();
    return;
  }
  // New configuration for ESP CAM with better image quality
  camera_config_t highConfig;
  highConfig.ledc_channel = LEDC_CHANNEL_0;
  highConfig.ledc_timer = LEDC_TIMER_0;
  highConfig.pin_d0 = Y2_GPIO_NUM;
  highConfig.pin_d1 = Y3_GPIO_NUM;
  highConfig.pin_d2 = Y4_GPIO_NUM;
  highConfig.pin_d3 = Y5_GPIO_NUM;
  highConfig.pin_d4 = Y6_GPIO_NUM;
  highConfig.pin_d5 = Y7_GPIO_NUM;
  highConfig.pin_d6 = Y8_GPIO_NUM;
  highConfig.pin_d7 = Y9_GPIO_NUM;
  highConfig.pin_xclk = XCLK_GPIO_NUM;
  highConfig.pin_pclk = PCLK_GPIO_NUM;
  highConfig.pin_vsync = VSYNC_GPIO_NUM;
  highConfig.pin_href = HREF_GPIO_NUM;
  highConfig.pin_sscb_sda = SIOD_GPIO_NUM;
  highConfig.pin_sscb_scl = SIOC_GPIO_NUM;
  highConfig.pin_pwdn = PWDN_GPIO_NUM;
  highConfig.pin_reset = RESET_GPIO_NUM;
  highConfig.xclk_freq_hz = 10000000;
  highConfig.pixel_format = PIXFORMAT_JPEG;
  highConfig.frame_size = FRAMESIZE_UXGA;//CIF,VGA,SVGA,XGA,SXGA,UXGA
  highConfig.jpeg_quality = 15;//0-63, 0 highest - 63 lowest
  highConfig.fb_count = 2;

  // Apply the new configuration
  esp_camera_deinit();
  esp_err_t highErr = esp_camera_init(&highConfig);
  if (highErr != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", highErr);
    ESP.restart();
  }
  setSensor();
  camera_fb_t * fbCapture;
  // Capture the image
  for(int i = 0; i < 100; i++){
    fbCapture = esp_camera_fb_get();
    if(!fbCapture){
      Serial.println("Camera capture failed");
      esp_camera_fb_return(fbCapture);
    }
    if(i!=99){
      delay(10);
      esp_camera_fb_return(fbCapture);
    }
  }
  // Create a char array to store the formatted time
  char filename[22]; // Adjust the size as needed

  // Format the time as "YYYY-MM-DD_HH-MM-SS" and store it in the filename array
  strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S", &timeinfo);

  // Now, filename contains the formatted time as a string
  Serial.println("Filename: " + String(filename));

  String path = "/" + String(filename) + ".jpg";

  
  Serial.printf("Picture file name: %s\n", path.c_str());
  writeFile(SD_MMC, path.c_str(), reinterpret_cast<const char*>(fbCapture->buf), fbCapture->len);
  // Send Captured image to Server
  cameraClient.sendBinary((const char*) fbCapture->buf, fbCapture->len);
  // Revert back to the initial configuration
  esp_camera_fb_return(fbCapture);
  esp_camera_deinit();
  esp_camera_init(&initConfig);
}
void moveServo(int pan, int tilt){
  servoPan.write(pan);
  servoTilt.write(tilt);
  delay(20);
}