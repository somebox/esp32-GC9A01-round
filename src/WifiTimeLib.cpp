#include "WifiTimeLib.h"
// inspired by https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino

WifiTimeLib::WifiTimeLib(const char* ntp_server, const char* tz_info) : NTP_SERVER(ntp_server), TZ_INFO(tz_info) {}

String WifiTimeLib::getFormattedDate(){
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", &timeinfo);
    return String(time_output);
}

String WifiTimeLib::getFormattedTime(){
    char time_output[30];
    strftime(time_output, 30, "%H:%M:%S", &timeinfo);
    return String(time_output);
}

bool WifiTimeLib::connectToWiFi(const char* ap_name) {
  wm.setAPCallback(std::bind(&WifiTimeLib::configModeCallback, this, &wm));
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);

  // wm.resetSettings();   // uncomment to force a reset
  bool wifi_connected = wm.autoConnect(ap_name);
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
    return true;
  } else {
    Serial.println("WiFi connection failed");
    return false;
  }
}

void WifiTimeLib::configModeCallback(WiFiManager *wm) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(wm->getConfigPortalSSID());
}

// retrieve NTP time with an optional timeout in seconds
bool WifiTimeLib::getNTPtime(int timeout=10) {
    if (WiFi.isConnected()) {
        bool timeout_reached = false;
        long start = millis();

        Serial.println(" updating:");
        configTime(0, 0, NTP_SERVER);
        setenv("TZ", TZ_INFO, 1);

        do {
            timeout_reached = (millis() - start) > (1000 * timeout);
            time(&now);
            localtime_r(&now, &timeinfo);
            if (timeinfo.tm_year > (2023 - 1900)) break;
            Serial.print(" . ");
            delay(100);
        } while (!timeout_reached);

        // print what we got
        Serial.println();
        Serial.println(getFormattedDate());
        Serial.println(getFormattedTime());

        if (timeinfo.tm_year < (2023 - 1900)){
            Serial.println("Error: Invalid date received!");
            Serial.println(timeinfo.tm_year);
            return false;  // the NTP call was not successful
        } else if (timeout_reached) {
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