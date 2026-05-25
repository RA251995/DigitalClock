#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <U8g2lib.h>

// Initialize the RTC
RTC_DS3231 rtc;

// Initialize the 1.3" OLED (SH1106 driver) using hardware I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

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