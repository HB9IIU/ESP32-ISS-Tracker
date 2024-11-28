
# HB9IIU ISS Life Tracker
### **Efficient Real-Time Calculations on an ESP32**


This project is an **ESP32-based tracking system for the International Space Station (ISS)** that demonstrates the impressive functionality and versatility of this microcontroller. Unlike other applications that rely on external APIs to fetch the ISS's current position, this system retrieves only the **Two-Line Elements (TLEs)** and current time from online sources. All orbital calculations are performed in real time using the **SGDP4 library**, making the solution self-contained and dynamic.

The system provides detailed information about ISS passes, including **Acquisition of Signal (AOS)**, **Time of Closest Approach (TCA)**, and **Loss of Signal (LOS)**. The data is displayed on a **480x320 TFT screen** with a touchscreen interface, offering **clear and informative visualizations** such as polar plots, azimuth/elevation graphs, and satellite footprint maps. 

This project highlights how much capability can be packed into an ESP32, handling computationally intensive tasks while remaining compact and efficient.


---

<div align="center">
  <a href="https://www.youtube.com/watch?v=-qaXMxvWq9A">
    <img src="https://img.youtube.com/vi/-qaXMxvWq9A/0.jpg" alt="HB9IIU ISS Life Tracker Demo">
  </a>
  <p><strong>Click the image above to watch the demo on YouTube!</strong><br>
  (Right-click and select "Open in New Tab" to keep this page open)</p>
</div>

### Screenshots
<div align="center">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7612.png" alt="Screenshot 1" width="300"> 
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7613.png" alt="Screenshot 2" width="300">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7615.png" alt="Screenshot 3" width="300"> 
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7616.png" alt="Screenshot 4" width="300">
    <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7617.png" alt="Screenshot 5" width="300"> 
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7618.png" alt="Screenshot 6" width="300">
    <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/ScreenShots/IMG_7619.png" alt="Screenshot 6" width="300">
</div>



- 🌍 **Real-Time ISS Tracking**
  - Displays live ISS position on a world map (equirectangular projection).
  - Shows satellite footprint based on current altitude.
  - Tracks the ISS’s orbit number dynamically.

- 📊 **Pass Prediction**
  - Calculates next passes for the observer's location.
  - Displays key pass details:
    - Acquisition of Signal (AOS) time and azimuth.
    - Maximum Elevation (TCA) time and angle.
    - Loss of Signal (LOS) time and azimuth.
    - Pass duration and maximum elevation.

- 📈 **Interactive Graphs**
  - Polar plots for satellite passes.
  - Azimuth/elevation time plots.

- 🕒 **Time Management**
  - Synchronizes with **NTP servers** for accurate timekeeping.
  - Adjusts for local time zone and daylight saving time.

- 🖼 **Visual Enhancements**
  - Displays splash screens and static images like ISS expeditions or logos.
  - Dynamically updates live metrics such as altitude, distance, azimuth, and elevation.

- ⚡ **Efficient UI Updates**
  - Updates only changed screen elements for reduced flicker and improved performance.
  - Touchscreen navigation between multiple pages.

---

## Requirements

### Hardware
- **ESP32** (ESP32-WROOM-32 or similar variations).
- **480x320 TFT Display** (ILI9488 or compatible) with touch support.
- **Wi-Fi Access** for retrieving TLE data and syncing time.
### Software

#### Included Libraries
The following libraries are used in this project. **No additional installation is required**, as all libraries have been pre-copied into the `lib` folder of this repository.

- **`WiFi.h`**  
  Provides support for connecting the ESP32 to a Wi-Fi network, managing connections, and handling networking.

- **`HTTPClient.h`**  
  Enables the ESP32 to make HTTP GET and POST requests for interacting with REST APIs or downloading data from the web.

- **`ArduinoJson.h`**  
  A lightweight and efficient JSON library for parsing and generating JSON data, commonly used with web APIs.

- **`Sgp4.h`**  
  Implements the Simplified General Perturbations Model 4 (SGP4) for satellite orbit calculations, critical for tracking objects like the ISS.

- **`NTPClient.h`**  
  A simple Network Time Protocol (NTP) client for synchronizing the ESP32's internal clock with an NTP server.

- **`WiFiUdp.h`**  
  Provides UDP communication capabilities, used in conjunction with protocols like NTP.

- **`TFT_eSPI.h`**  
  A high-performance graphics library for rendering graphics and text on TFT screens, optimized for use with ESP32.

- **`Preferences.h`**  
  A library for reading and writing small pieces of data to the ESP32's flash memory, useful for storing persistent settings or data.

- **`PNGdec.h`**  
  Decodes PNG images for rendering on the TFT screen, enabling the use of rich graphical content.

- **`SolarCalculator.h`**  
  Provides tools for solar position calculations, helpful for determining solar angles or daylight conditions.

- **`HB9IIU7segFonts.h`**  
  Contains custom seven-segment display-like fonts, suitable for numeric or retro-style displays.

---

#### Additional Files in the Codebase
- **Custom Images**: Files like `ISSsplashImage.h`, `worldMap.h`, and `fancySplashImage.h` store images in 8-bit array format for rendering on the TFT screen.
- **TLE Data Management**: The `Preferences` library is used to persistently store and retrieve Two-Line Element (TLE) satellite data required for ISS tracking.

---

### Note
All the required libraries are already included in the repository’s `lib` folder. **There is no need to install additional libraries**—this ensures that the project compiles and runs seamlessly.


---

### Acknowledgments

A heartfelt thank you to the authors and contributors of the libraries used in this project. Your work has made it possible to bring this project to life. Each library brings a unique capability, and we deeply appreciate the time, effort, and expertise invested in creating and maintaining them. 🙏



---

## Setup

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/username/ISS-Life-Tracker.git
   cd ISS-Life-Tracker
   ```

2. **Install Dependencies**:
   Ensure the necessary libraries are installed in your IDE.

3. **Configure `config.h`**:
   Update the following parameters:
   ```cpp
   #define WIFI_SSID "your-ssid"
   #define WIFI_PASSWORD "your-password"
   #define OBSERVER_LATITUDE 46.4717
   #define OBSERVER_LONGITUDE 6.8768
   #define OBSERVER_ALTITUDE 400
   ```

4. **Upload to ESP32**:
   - Connect the ESP32 to your computer.
   - Compile and upload the code using Arduino IDE or PlatformIO.

5. **Run the Tracker**:
   The TFT screen will display a splash image, connect to Wi-Fi, and start retrieving and displaying ISS tracking data.

---

## Screenshots

### 1. World Map with ISS Position
![World Map Screenshot](https://example.com/world-map-screenshot)

### 2. Polar Plot of Satellite Pass
![Polar Plot Screenshot](https://example.com/polar-plot-screenshot)

### 3. Pass Prediction Table
![Pass Prediction Table](https://example.com/pass-table-screenshot)

---

## How It Works

1. **Retrieve TLE Data**: The tracker fetches Two-Line Element (TLE) data for the ISS from online APIs.
2. **Predict Satellite Passes**: Using the **Sgp4** library, it calculates upcoming passes for the observer's location.
3. **Render Visualizations**: The TFT display shows live satellite data, including passes, azimuth, elevation, and more.

---

## Contributing

Contributions are welcome! If you’d like to improve this project, please:
- Fork the repository.
- Create a new branch for your feature or bugfix.
- Submit a pull request with a detailed description.

---

## License

This project is licensed under the [MIT License](LICENSE).

---

## Author

**HB9IIU - Daniel**  
*Amateur Radio Enthusiast & Developer*  
[GitHub Profile](https://github.com/username) | [Contact Me](mailto:email@example.com)
