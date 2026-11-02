# ⏱️ Chronos

**An IoT-powered, power-optimized hybrid digital clock built with ESP32 and cascaded shift registers.**

Why choose between the atomic precision of the internet and the offline reliability of hardware? Chronos leverages an ESP32 and a DS3231 RTC module to create a fault-tolerant timekeeping system. It automatically syncs with global NTP servers when Wi-Fi is available, then immediately cuts the radio to save power, running the display offline via the hardware RTC.

By daisy-chaining four 74HC595 shift registers, Chronos drives 32 individual 7-segment LED segments using only **three** GPIO pins on the ESP32.

---

## ✨ Key Features

* 🌐 **Hybrid Timekeeping:** Automatically fetches atomic time via NTP once a week to correct drift, then writes it to the local DS3231 RTC module.
* 🔋 **Modem Sleep Optimization:** The ESP32 shuts down its power-hungry Wi-Fi radio immediately after syncing, minimizing heat and power draw while the CPU continues to tick.
* 🧠 **Non-Blocking Architecture:** Built with a `millis()`-based state machine. The display rendering loop never blocks, meaning your clock keeps ticking and blinking seamlessly even while the ESP32 is actively hunting for a Wi-Fi connection.
* ⛓️ **Cascaded Output:** Controls 4 individual 7-segment displays through a shift-register pipeline, freeing up the ESP32's pins for future sensors or web-server expansions.

---

## 📦 Bill of Materials

* **1x** ESP32 Development Board
* **1x** DS3231 (or DS1307) I2C Real-Time Clock Module
* **4x** 74HC595 Shift Register ICs
* **4x** Single-Digit 7-Segment Displays (Code supports Common Cathode & Common Anode)
* **32x** 220Ω or 330Ω Resistors (One for each segment line)
* Breadboard & jumper wires

---

## ⚡ Hardware Architecture & Wiring

### 1. ESP32 to Modules

| ESP32 Pin | Component | Function |
| --- | --- | --- |
| **3.3V** | RTC & 74HC595s | Main Power (VCC / SRCLR) |
| **GND** | RTC & 74HC595s | Ground (GND / OE) |
| **GPIO 21** | RTC Module | I2C SDA |
| **GPIO 22** | RTC Module | I2C SCL |
| **GPIO 23** | IC 1 (Pin 14) | Shift Register Data (`SER`) |
| **GPIO 18** | All ICs (Pin 11) | Shift Register Clock (`SRCLK`) |
| **GPIO 5** | All ICs (Pin 12) | Shift Register Latch (`RCLK`) |

### 2. The Shift Register Daisy Chain

Data flows sequentially from the tens-hour digit to the ones-minute digit:

* **IC 1 Pin 9 (`QH'`/ Serial Out)** ➔ **IC 2 Pin 14 (`SER`)**
* **IC 2 Pin 9 (`QH'`)** ➔ **IC 3 Pin 14 (`SER`)**
* **IC 3 Pin 9 (`QH'`)** ➔ **IC 4 Pin 14 (`SER`)**

### 3. Display Segment Mapping

Connect outputs **QA through QH** on each 74HC595 to segments **A through DP** on its respective 7-segment display. *Always use current-limiting resistors on every data line.*

---

## 🚀 Getting Started

1. **Install Dependencies:** Open the Arduino IDE Library Manager and install **`RTClib` by Adafruit**.
2. **Configure Code:**
* Enter your network credentials in `ssid` and `password`.
* The default timezone is configured for India Standard Time (`IST-5:30`). Update the `time_zone` variable if you are in a different region using standard POSIX timezone strings.
* If you are using Common Anode displays, change `const bool isCommonAnode = false;` to `true`.


3. **Flash:** Upload the sketch to your ESP32.

Upon boot, the Serial Monitor (at `115200` baud) will output the connection status, NTP fetch results, and RTC sync confirmations before shifting to Modem Sleep mode.
