#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <RTClib.h> // Install "RTClib" by Adafruit via Library Manager

RTC_DS3231 rtc; // Works for DS3231. Change to RTC_DS1307 rtc; if using a DS1307.

// Wi-Fi Credentials
const char* ssid       = "YOUR_WIFI_SSID";
const char* password   = "YOUR_WIFI_PASSWORD";

// NTP Settings
const char* ntpServer = "pool.ntp.org";
const char* time_zone = "IST-5:30"; // Modern ESP32 POSIX timezone string for India Standard Time

// Shift Register Pins
const int DATA_PIN = 23;  
const int LATCH_PIN = 5;  
const int CLOCK_PIN = 18; 

const bool isCommonAnode = false; 

const byte digitArray[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// Sync tracking using millis() to prevent RTC-time-related infinite loops
unsigned long lastSyncAttemptMillis = 0;
bool timeIsSynced = false;

const unsigned long SYNC_INTERVAL_MS = 604800000; // 7 days in milliseconds
const unsigned long RETRY_INTERVAL_MS = 300000;   // 5 minutes in milliseconds

// Forward declaration so we can use it in the sync function
void updateDisplay();

// Function to connect to Wi-Fi and update the RTC
void syncTimeFromNTP() {
  Serial.print("Waking up Wi-Fi to sync time...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int retries = 0;
  // Non-blocking-ish wait: keeps the clock rendering while connecting
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    updateDisplay(); // Keep display alive and blinking during Wi-Fi wait
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Connected. Fetching NTP...");
    configTzTime(time_zone, ntpServer); // Modern Timezone Handling
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10000)) {
      // Successfully got web time, now push it to the RTC hardware
      rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
      timeIsSynced = true;
      Serial.println("RTC successfully synchronized with NTP!");
    } else {
      timeIsSynced = false;
      Serial.println("NTP fetch failed.");
    }
  } else {
    timeIsSynced = false;
    Serial.println("\nWi-Fi connection failed.");
  }

  // Update the attempt tracker whether it succeeded or failed to prevent instant loop retries
  lastSyncAttemptMillis = millis();

  // MODEM SLEEP: Turn off the Wi-Fi radio to save massive amounts of power
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF); 
  Serial.println("Wi-Fi disabled (Modem Sleep active).");
}

// Extracted the display logic into a function so we can call it while WiFi is waiting
void updateDisplay() {
  // 1. Get the current time from the RTC hardware
  DateTime now = rtc.now();

  // 2. Format the time (12-hour format)
  int currentHour = now.hour();
  int currentMinute = now.minute();
  int currentSecond = now.second();

  if (currentHour == 0) currentHour = 12;
  if (currentHour > 12) currentHour -= 12;

  int tensHour = currentHour / 10;
  int onesHour = currentHour % 10;
  int tensMinute = currentMinute / 10;
  int onesMinute = currentMinute % 10;

  byte byte4 = digitArray[tensHour];   
  byte byte3 = digitArray[onesHour];   
  byte byte2 = digitArray[tensMinute]; 
  byte byte1 = digitArray[onesMinute]; 

  // Blink the decimal point on the Ones Hour display
  if (currentSecond % 2 == 0) {
    byte3 |= 0x80; 
  }

  if (isCommonAnode) {
    byte1 = ~byte1;
    byte2 = ~byte2;
    byte3 = ~byte3;
    byte4 = ~byte4; 
  }

  // 3. Update the displays
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, byte1); // IC 4
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, byte2); // IC 3
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, byte3); // IC 2
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, byte4); // IC 1
  digitalWrite(LATCH_PIN, HIGH);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, LOW);

  // Initialize I2C and RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC! Check wiring to GPIO 21 and 22.");
    while (1) delay(10); // Halt if RTC is missing
  }

  // Handle RTC Loss of Power
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, forcing an immediate sync!");
    syncTimeFromNTP();
  } else {
    // Standard initial sync
    syncTimeFromNTP();
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if we need to sync based on millis() instead of RTC unix time
  // If synced successfully: wait 7 days. If failed: wait 5 minutes before trying again.
  unsigned long intervalToUse = timeIsSynced ? SYNC_INTERVAL_MS : RETRY_INTERVAL_MS;

  // Handle the millis() timing check
  if (currentMillis - lastSyncAttemptMillis >= intervalToUse) {
    syncTimeFromNTP();
  }

  // Render the display
  updateDisplay();
  
  delay(50); // Keep loop stable
}