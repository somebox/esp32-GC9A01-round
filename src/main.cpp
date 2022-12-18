#include <Arduino.h>
#include <WiFiManager.h>
#include "time.h"
#include <SPI.h>
#include <TFT_eSPI.h>     // https://github.com/Bodmer/TFT_eSPI

// Timezone config
/* 
  Enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
  See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  based on https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
*/
const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // Switzerland

// Wifi
WiFiManager wm;   // looking for credentials? don't need em! ... google "ESP32 WiFiManager"

// Font files are stored in SPIFFS (flash ram)
#define FS_NO_GLOBALS
#include <FS.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite digital_face_hours = TFT_eSprite(&tft);
TFT_eSprite digital_face_minutes = TFT_eSprite(&tft);
TFT_eSprite analog_face = TFT_eSprite(&tft);

#define CLOCK_X_POS 118
#define CLOCK_Y_POS 118

#define CLOCK_FG   TFT_LIGHTGREY
#define CLOCK_BG   TFT_BROWN
#define SECCOND_FG TFT_YELLOW
#define LABEL_FG   TFT_RED

#define CLOCK_R       240.0f / 2.0f // Clock face radius (float type)
#define H_HAND_LENGTH CLOCK_R/2.2f
#define M_HAND_LENGTH CLOCK_R/1.5f
#define S_HAND_LENGTH CLOCK_R/1.2f

// Calculate 1 second increment angles. Hours and minute hand angles
// change every second so we see smooth sub-pixel movement
#define SECOND_ANGLE 360.0 / 60.0
#define MINUTE_ANGLE SECOND_ANGLE / 60.0
#define HOUR_ANGLE   MINUTE_ANGLE / 12.0

// Screen width and height
#define SCREEN_W 240
#define SCREEN_H 240

// handle multiple displays via CS pin
#define num_displays 2
uint8_t display_cs_pins[num_displays] = {22,21};
uint16_t bg_colors[num_displays] = {TFT_OLIVE, TFT_PURPLE};

// Time 
tm timeinfo;
time_t now;
int hour = 0;
int minute = 0;
int second = 0;

String getFormattedDate(){
  char time_output[30];
  strftime(time_output, 30, "%a  %d-%m-%y %T", &timeinfo);
  return String(time_output);
}

String getFormattedTime(){
  char time_output[30];
  strftime(time_output, 30, "%H:%M:%S", &timeinfo);
  return String(time_output);
}

void configModeCallback(WiFiManager *wm) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(wm->getConfigPortalSSID());
}

bool getNTPtime(int sec) {
  if (WiFi.isConnected()) {
    bool timeout = false;
    bool date_is_valid = false;
    long start = millis();

    Serial.println(" updating:");
    configTime(0, 0, NTP_SERVER);
    setenv("TZ", TZ_INFO, 1);

    do {
      timeout = (millis() - start) > (1000 * sec);
      time(&now);
      localtime_r(&now, &timeinfo);
      Serial.print(" . ");
      date_is_valid = timeinfo.tm_year > (2016 - 1900);
      delay(100);
    } while (!timeout && !date_is_valid);
    
    Serial.println("\nSystem time is now:");
    Serial.println(getFormattedDate());
    Serial.println(getFormattedTime());
    
    if (!date_is_valid){
      Serial.println("Error: Invalid date received!");
      Serial.println(timeinfo.tm_year);
      return false;  // the NTP call was not successful
    } else if (timeout) {
      Serial.println("Error: Timeout while trying to update the current time with NTP");
      return false;
    } else {
      Serial.println("[ok] time updated: ");
      return true;
    }
  } else {
    Serial.println("Error: Update time failed, no WiFi connection!");
    return false;
  }
}


void ConnectToWifi(){
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wm.setAPCallback(configModeCallback);

  // wm.resetSettings();   // uncomment to force a reset
  bool wifi_connected = wm.autoConnect("ESP32 Clock - Round Displays");
  int t=0;
  if (wifi_connected){
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println("db");

    delay(1000);

    Serial.println("getting current time...");
    
    // timeClient.begin();
    // timeClient.update();
    if (getNTPtime(10)) {  // wait up to 10sec to sync
      Serial.println("Time sync complete");
    } else {
      Serial.println("Error: NTP time update failed!");
    }
  } else {
    Serial.println("ERROR: WiFi connect failure");
  }
}

// =========================================================================
// Get coordinates of end of a line, pivot at x,y, length r, angle a
// =========================================================================
// Coordinates are returned to caller via the xp and yp pointers
#define DEG2RAD 0.0174532925
void getCoord(int16_t x, int16_t y, float *xp, float *yp, int16_t r, float a)
{
  float sx1 = cos( (a - 90) * DEG2RAD);
  float sy1 = sin( (a - 90) * DEG2RAD);
  *xp =  sx1 * r + x;
  *yp =  sy1 * r + y;
}

// =========================================================================
// Draw the clock face in the sprite
// =========================================================================
static void renderDigitalFace(float t, uint16_t bg_color) {
  static int last_hr = 1000;
  char cnum[10];

  // update hours
  if (last_hr != (int)t/3600){
    last_hr = (int)t/3600;
    digital_face_hours.fillSprite(bg_color);
    digital_face_hours.setTextColor(CLOCK_FG, bg_color);  
    digital_face_hours.setTextDatum(MR_DATUM);
    snprintf(cnum, 10, "%02d", (int)t/3600);  // hours
    digital_face_hours.drawString(cnum, digital_face_hours.width()-2, digital_face_hours.height()/2);    
    digital_face_hours.pushSprite(2, tft.height()/2 - digital_face_hours.height()/2); 
  }
  
  // update minutes and seconds
  digital_face_minutes.fillSprite(bg_color);
  digital_face_minutes.setTextColor(TFT_ORANGE, bg_color);  
  digital_face_minutes.setTextDatum(ML_DATUM);
  // minutes
  snprintf(cnum, 10, "%02d", (int)t/60 % 60);
  digital_face_minutes.drawString(cnum, 0, digital_face_minutes.height()*0.3);
  digital_face_minutes.setTextColor(TFT_SKYBLUE, bg_color);  
  // seconds
  snprintf(cnum, 10, "%02d", (int)floor(t) % 60);
  digital_face_minutes.drawString(cnum, 0, digital_face_minutes.height()*0.7);

  digital_face_minutes.pushSprite(tft.width()/1.8, tft.height()/2 - digital_face_minutes.height()/2); 
}

// =========================================================================
// Draw the clock face in the sprite
// =========================================================================
static void renderAnalogFace(float t, uint16_t bg_color) {
  float h_angle = t * HOUR_ANGLE;
  float m_angle = t * MINUTE_ANGLE;
  float s_angle = t * SECOND_ANGLE;
  
  // The face is completely redrawn - this can be done quickly
  analog_face.fillSprite(bg_color);

  // Set text datum to middle centre and the colour
  analog_face.setTextDatum(MC_DATUM);

  // The background colour will be read during the character rendering
  analog_face.setTextColor(CLOCK_FG, bg_color);

  // Text offset adjustment
  constexpr uint32_t dialOffset = CLOCK_R - 15;

  float xp = 0.0, yp = 0.0; // Use float pixel position for smooth AA motion

  // Draw digits around clock perimeter
  for (uint32_t h = 1; h <= 12; h++) {
    getCoord(CLOCK_R, CLOCK_R, &xp, &yp, dialOffset, h * 360.0 / 12);
    analog_face.drawNumber(h, xp, 2 + yp);
  }

  // Add text (could be digital time...)
  analog_face.setTextColor(LABEL_FG, bg_color);
  //face.drawString("TFT_eSPI", CLOCK_R, CLOCK_R * 0.75);

  // Draw minute hand
  getCoord(CLOCK_R, CLOCK_R, &xp, &yp, M_HAND_LENGTH, m_angle);
  analog_face.drawWideLine(CLOCK_R, CLOCK_R, xp, yp, 8.0f, CLOCK_FG);
  analog_face.drawWideLine(CLOCK_R, CLOCK_R, xp, yp, 4.0f, TFT_GREEN);

  // Draw hour hand
  getCoord(CLOCK_R, CLOCK_R, &xp, &yp, H_HAND_LENGTH, h_angle);
  analog_face.drawWideLine(CLOCK_R, CLOCK_R, xp, yp, 8.0f, CLOCK_FG);
  analog_face.drawWideLine(CLOCK_R, CLOCK_R, xp, yp, 4.0f, TFT_GREENYELLOW);

  // Draw the central pivot circle
  analog_face.fillSmoothCircle(CLOCK_R, CLOCK_R, 8, CLOCK_FG);

  // Draw second hand
  getCoord(CLOCK_R, CLOCK_R, &xp, &yp, S_HAND_LENGTH, s_angle);
  analog_face.drawWedgeLine(CLOCK_R, CLOCK_R, xp, yp, 3.5, 1.5, SECCOND_FG);
  analog_face.pushSprite(0, 0, TFT_TRANSPARENT);
}


// =========================================================================
// Setup
// =========================================================================
uint32_t targetTime = 0;    // Time for next tick
void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");

  ConnectToWifi();

  for (int i=0; i < num_displays; i++){
    pinMode(display_cs_pins[i], OUTPUT);
    digitalWrite(display_cs_pins[i],HIGH);
  }
  
  // Create the clock face sprite
  //face.setColorDepth(8); // 8 bit will work, but reduces effectiveness of anti-aliasing
  digital_face_minutes.createSprite(SCREEN_W / 2, SCREEN_H / 2);
  digital_face_minutes.loadFont("Mali-Bold-60");

  digital_face_hours.createSprite(SCREEN_W / 2, SCREEN_H / 2);  
  digital_face_hours.loadFont("Mali-Bold-90");

  analog_face.createSprite(SCREEN_W, SCREEN_H);
  analog_face.loadFont("Futura-MediumItalic-18"); // prepare for analog updates afterwards

  // Initialize displays
  for (int i=0; i < num_displays; i++){
    digitalWrite(display_cs_pins[i],LOW);
  }
  // Initialise the screen
  tft.init();    
  // Ideally set orientation for good viewing angle range because
  // the anti-aliasing effectiveness varies with screen viewing angle
  // Usually this is when screen ribbon connector is at the bottom
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  for (int i=0; i < num_displays; i++){
    digitalWrite(display_cs_pins[i],HIGH);
  }
  digitalWrite(display_cs_pins[1], LOW);
  tft.fillSmoothCircle( CLOCK_R-1, CLOCK_R-1, CLOCK_R, bg_colors[1] );
  digitalWrite(display_cs_pins[1], HIGH);

  targetTime = millis();
}

// =========================================================================
// Loop
// =========================================================================
int fps=18;                 // frame rate counter, start val is an estimate
float avg_fps=18.0;         // running average across 2 loop samples
float time_secs;            // time in seconds+ms sent to rendering functions
int last_second = 0;        // for checking when to update the digital clock
int ms_offset = 0;          // offset of millis() since start of the last sec

void loop() {
  long m = millis();
  if (targetTime < m) {   
    // schedule next tick time for smoother movement
    targetTime = m +  3;

    // Update time periodically
    time(&now);
    localtime_r(&now, &timeinfo);
    int secs = timeinfo.tm_sec;
    time_secs = timeinfo.tm_hour*3600
              + timeinfo.tm_min*60 
              + secs;

    if (secs != last_second){
      ms_offset = m;
      last_second = secs;
      Serial.print(" FPS > ");
      Serial.println(fps);
      avg_fps = (avg_fps + fps)/2;
      fps = 0;

      // digital clock
      digitalWrite(display_cs_pins[1], LOW);
      renderDigitalFace(time_secs, bg_colors[1]);
      digitalWrite(display_cs_pins[1], HIGH);
    } 

    // analog clock
    digitalWrite(display_cs_pins[0], LOW);
    renderAnalogFace(time_secs + (millis()-ms_offset)/1000.0, bg_colors[0]);
    digitalWrite(display_cs_pins[0], HIGH);

    // Keep track of frame rate and use it to keep the animation consistent
    fps++;

    // Serial.println(time_secs);
  }
}