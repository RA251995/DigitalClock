#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include <RTClib.h>
#include <U8g2lib.h>

// --- Wi-Fi Credentials ---
const char* STATION_SSID     = "MANAYIL NET NALUPLACKAL";
const char* STATION_PASSWORD = "9447806258";

// --- Timezone Configuration ---
// Indian Standard Time (IST) is UTC +5:30. 
// Calculation: (5 hours * 3600s) + (30 minutes * 60s) = 19800 seconds
const long UTC_OFFSET_SECS = 19800;

// Initialize the RTC
RTC_DS3231 rtc;

// Initialize the 1.3" OLED (SH1106 driver) using hardware I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Initialize Network Objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_SECS, 60000);

void syncTimeWithNTP() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 25, "Connecting to WiFi...");
  u8g2.sendBuffer();

  Serial.print("Connecting to ");
  Serial.println(STATION_SSID);
  
  WiFi.begin(STATION_SSID, STATION_PASSWORD);
  
  // Wait for connection with a 15-second timeout window
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 30) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    u8g2.clearBuffer();
    u8g2.drawStr(10, 25, "Connected! Syncing...");
    u8g2.sendBuffer();

    // Start the NTP communication layer
    timeClient.begin();
    
    // Force an update from the remote server pool
    if (timeClient.update()) {
      unsigned long epochTime = timeClient.getEpochTime();
      
      // Update the local physical DS3231 module memory registers
      rtc.adjust(DateTime(epochTime));
      Serial.println("RTC successfully synchronized via NTP!");
      
      u8g2.clearBuffer();
      u8g2.drawStr(10, 25, "Sync complete!");
      u8g2.sendBuffer();
      delay(1000);
    } else {
      Serial.println("NTP Update failed. Using existing RTC time data.");
    }
  } else {
    Serial.println("\nWiFi connection timed out. Falling back to offline RTC memory.");
  }

  // Shut down the Wi-Fi radio assembly completely to minimize power draw and radio noise
  WiFi.disconnect(true);
  Serial.println("WiFi Radio powered down.");
}

void setup() {
  Serial.begin(115200);

  // Initialize I2C with NodeMCU default pins: SDA=D2, SCL=D1
  Wire.begin(D2, D1);

  // Initialize OLED display
  u8g2.begin();

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC module!");
    while (1);
  }

  // NOTE: If your RTC battery is fresh and working, keep this line commented out 
  // so it doesn't overwrite your time every time the board restarts.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Execute network time calibration alignment routines on startup initialization
  syncTimeWithNTP();
}

void loop() {
  // Get current time data from the DS3231 chip
  DateTime now = rtc.now();

  // Dynamic character buffers for string formatting
  char timeBuffer[9];     // Format: HH:MM:SS
  char dateBuffer[12];    // Format: DD-MMM-YYYY
  char tempBuffer[10];    // Format: XX.XX C

  // Array of months for clean presentation text
  const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  // Populate data buffers using standard formatters
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  sprintf(dateBuffer, "%02d %s %d", now.day(), months[now.month() - 1], now.year());
  
  float currentTemp = rtc.getTemperature();
  dtostrf(currentTemp, 4, 1, tempBuffer); // Convert float to string with 1 decimal place

  // ---------------- UI Rendering Phase ----------------
  u8g2.clearBuffer();

  // Outer framing boundary layout
  u8g2.drawFrame(0, 0, 128, 64);

  // Render Time Header (Large, clean high-contrast font)
  u8g2.setFont(u8g2_font_logisoso22_tf); 
  u8g2.drawStr(4, 28, timeBuffer);

  // Horizontal separating design rule line
  u8g2.drawHLine(6, 35, 116);

  // Render Date String (Medium clean font)
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(8, 52, dateBuffer);

  // Render Temperature Reading in corner
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(82, 51);
  u8g2.print(tempBuffer);
  u8g2.print("C");

  // Push frame layout memory to the physical screen panel
  u8g2.sendBuffer();

  // Refresh interval cadence
  delay(1000); 
}