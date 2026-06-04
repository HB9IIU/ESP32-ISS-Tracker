# HB9IIU ISS Tracker
### Real-Time Satellite Tracking on an ESP32 — No API Key Required

---

<div align="center">

## ⚡ Flash your CYD board directly from your browser

### No Visual Studio. No PlatformIO. No software to install.

<a href="https://hb9iiu.github.io/ESP32-ISS-Tracker/">
  <img src="https://img.shields.io/badge/⚡%20OPEN%20WEB%20FLASHER-Click%20Here-blue?style=for-the-badge&logoColor=white" alt="Open Web Flasher" height="50"/>
</a>

**Plug your CYD board into your PC via USB → click the button above → flash → done.**  
*Works in Chrome and Edge on Windows, macOS and Linux.*

</div>

---

## What is this?

This project is an **ESP32-based tracking system for the International Space Station (ISS)** — and any other satellite you choose. Unlike systems that rely on external APIs for live position data, this one retrieves only the **Two-Line Elements (TLEs)** and current time from the internet. All orbital calculations are done in real time on the ESP32 itself using the **SGP4 library**, making the solution self-contained and dynamic.

The system predicts and displays **AOS (Acquisition of Signal)**, **TCA (Time of Closest Approach)** and **LOS (Loss of Signal)** for upcoming passes. A 480×320 touchscreen TFT shows polar plots, azimuth/elevation graphs, world map overlays with multi-pass predictions, ISS crew information, and a custom 7-segment clock — all navigable by touch.

---

## Demo

<div align="center">
  <a href="https://youtu.be/Vp4qGIXc1Ag">
    <img src="https://img.youtube.com/vi/Vp4qGIXc1Ag/0.jpg" alt="HB9IIU ISS Tracker Demo" width="480">
  </a>
  <p><strong>Click the image to watch the demo on YouTube</strong></p>
</div>

---

## Screenshots

<div align="center">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7612.png" alt="Screenshot 1" width="300">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7613.png" alt="Screenshot 2" width="300">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7615.png" alt="Screenshot 3" width="300">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7616.png" alt="Screenshot 4" width="300">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7617.png" alt="Screenshot 5" width="300">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7618.png" alt="Screenshot 6" width="300">
</div>

---

## First-Boot Setup (CYD)

After flashing, the device walks you through setup automatically — no computer needed after this point.

### Step 1 — Touch calibration

On first boot the screen asks: **"Is this text upside-down?"**

- If the screen looks correct, simply wait for the countdown (8 seconds).
- If the screen is upside-down, **tap anywhere** to flip the orientation. Calibration then proceeds in the correct orientation.

After the orientation check, follow the on-screen arrows to calibrate the touchscreen. This only needs to be done once.

### Step 2 — Wi-Fi & location setup

After calibration the device starts a **captive portal**:

1. The screen displays:
   - WiFi network name: **`ISS-Tracker-Setup`**
   - Browser address: **`192.168.4.1`**
2. On your phone or PC, connect to the **`ISS-Tracker-Setup`** WiFi network.
3. Your browser should open automatically. If not, navigate to **`192.168.4.1`**.
4. Fill in the form:
   - Your WiFi network name and password
   - Your location (latitude, longitude, altitude)
   - Satellite to track (ISS is pre-selected)
   - Notifications, display preferences, and more
5. Tap **Save & Reboot**.

The device reboots, connects to your WiFi and starts tracking. That's it.

> **Tip:** Right-click your location on Google Maps to copy coordinates directly.

---

## Factory Reset

If you need to start from scratch (new WiFi, new location, or give the device to someone else):

1. Power on the device (or reboot it).
2. While the **splash screen** is showing, **hold your finger on the screen for 3 seconds**.
3. A confirmation screen appears — tap to confirm within 5 seconds.

The device erases all settings and calibration data, then reboots into the first-boot setup flow.

---

## Changing Settings After First Boot

Once the device is running on your WiFi network, you can change any setting at any time — no factory reset needed.

Open a browser on any phone or PC connected to the **same WiFi network** and go to:

**`http://iss-tracker.local`**

Or use the IP address shown on the TFT screen during boot.

The settings page opens pre-filled with all your current values. Change what you need, tap **Save & Reboot**, and the device restarts with the new configuration.

> **Note:** The password field is intentionally left blank — leave it empty to keep your current password, or type a new one to change it.

> **Tip:** `http://iss-tracker.local` works on Windows, macOS, iOS and Android. On some older Android devices you may need to use the IP address instead.

---

## Hardware

### Option 1 — CYD 4" Integrated Board (recommended)

<div align="center">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/ESP32-32E_4inch_CYD_board.png" alt="ESP32-32E 4 inch CYD board" width="600">
</div>

The **ESP32-32E 4-inch integrated display board** (also known as "Cheap Yellow Display" or CYD) has everything built in — ESP32, 480×320 ST7796 display, resistive touchscreen. No wiring required.

Search AliExpress for: **"ESP32-32E 4 inch display"**

- This is the board supported by the web flasher above.
- Captive portal setup included — no `config.h` editing, no recompiling.

### Option 2 — Standard ESP32 + External ILI9488 Display

The original hardware configuration: a standard ESP32 dev board wired to an external 480×320 ILI9488 SPI TFT with XPT2046 touch controller.

- Wiring diagram available in the `doc/` folder.
- No custom PCB needed — direct pin-to-pin connections with Dupont cables.
- Requires building from source with PlatformIO (see Developer Setup below).

---

## Developer Setup (PlatformIO)

For those who want to build from source, modify the code, or use the external display variant.

### Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)

### Steps

1. **Download the repository:**

   ```bash
   git clone https://github.com/HB9IIU/ESP32-ISS-Tracker.git
   ```
   Or download the ZIP from GitHub and unzip it.

2. **Open the project folder in VS Code.**  
   PlatformIO will automatically detect the project.

3. **Select your build profile** in `platformio.ini`:

   ```ini
   [platformio]
   default_envs = CHEAP_YELLOW_DISPLAY_4IN   ; CYD 4" board
   ; default_envs = EXTERNAL_DISPLAY_ILI9488  ; ESP32 + external ILI9488
   ```

   Or build a specific profile explicitly:

   ```bash
   pio run -e CHEAP_YELLOW_DISPLAY_4IN
   pio run -e EXTERNAL_DISPLAY_ILI9488
   ```

4. **Build and upload** using the PlatformIO toolbar (checkmark = build, arrow = upload).

5. **For the CYD build:** configuration is done via the captive portal on first boot — no source code editing needed.  
   **For the external display build:** edit the defaults at the top of `src/main.cpp` (WiFi credentials, observer location, etc.) before uploading.

> **Note:** All required libraries are vendored in the `lib/` folder. No additional library installation is needed.

---

## Features

- **Real-time SGP4 orbital calculations** — fully on-device, no position API
- **Pass prediction** — AOS, TCA, LOS with azimuth, elevation and duration
- **Multiple display pages** navigable by touch:
  - 7-segment style clock
  - Polar pass plot
  - Azimuth / elevation graph
  - World map with footprint and multi-pass overlay
  - ISS crew information
  - System info page
- **Audio notifications** — configurable buzzer beeps before AOS/LOS and at TCA
- **WebSocket output** — for driving an external azimuth/elevation rotor
- **Any satellite** — track ISS, NOAA weather satellites, HAM radio birds, Hubble, and more by catalogue number
- **Automatic timezone** — retrieved via [Open-Meteo](https://open-meteo.com/), no API key required
- **TLE caching** — orbital elements stored in flash, available offline after first fetch
- **Captive portal provisioner** (CYD) — browser-based first-boot configuration, no code editing
- **Persistent settings server** — change any setting at any time via `http://iss-tracker.local`, no factory reset needed
- **Factory reset** — hold touch during splash screen to wipe and reconfigure

---

## 3D Printing

STL files for a custom enclosure are included in the `doc/Enclosure3DprintFiles/` folder.

<div align="center">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/Enclosure3DprintFiles/Renderings/TFTESP32enclsoure_1.png" alt="Enclosure render" width="500">
</div>

---

## Libraries Used

All libraries are pre-included in the `lib/` folder — no separate installation needed.

| Library | Purpose |
|---|---|
| TFT_eSPI | High-performance TFT graphics |
| SGP4 | Satellite orbital calculations |
| ArduinoJson | JSON parsing |
| NTPClient | Network time synchronisation |
| PNGdec | PNG image decoding |
| ESPAsyncWebServer | Captive portal web server |
| AsyncTCP | Async TCP for ESPAsyncWebServer |
| Preferences | Persistent NVS storage |
| WebSocketsServer | WebSocket rotor output |
| HB9IIU7segFonts | Custom 7-segment style fonts |

---

## Contributing

Contributions are welcome!

- Fork the repository
- Create a branch for your feature or fix
- Submit a pull request with a clear description

---

## License

This project is licensed under the [MIT License](LICENSE).

---

## Author

**HB9IIU — Daniel Staempfli**  
*Amateur Radio Enthusiast & Developer*  
Contact: daniel at hb9iiu.com

---

*This is my corner of GitHub — still learning, always tinkering. Feedback and suggestions are always welcome. Thanks for stopping by! 🙌*

![ESP32-1](https://raw.githubusercontent.com/HB9IIU/ESP32-ISS-Tracker/main/doc/Misc/ESP32-1.png)
