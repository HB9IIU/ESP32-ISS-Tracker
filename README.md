
# HB9IIU ISS Life Tracker

![Project Banner](https://example.com/banner-image-url)

### **Efficient Real-Time Calculations on an ESP32**

This project is an **ESP32-based tracking system for the International Space Station (ISS)** that demonstrates the impressive functionality and versatility of this microcontroller. Unlike other applications that rely on external APIs to fetch the ISS's current position, this system retrieves only the **Two-Line Elements (TLEs)** and current time from online sources. All orbital calculations are performed in real time using the **SGDP4 library**, making the solution self-contained and dynamic.

The system provides detailed information about ISS passes, including **Acquisition of Signal (AOS)**, **Time of Closest Approach (TCA)**, and **Loss of Signal (LOS)**. The data is displayed on a **480x320 TFT screen** with a touchscreen interface, offering **clear and informative visualizations** such as polar plots, azimuth/elevation graphs, and satellite footprint maps. 

This project highlights how much capability can be packed into an ESP32, handling computationally intensive tasks while remaining compact and efficient.


---

## Features

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
- **ESP32-S3** (or any compatible ESP32 board).
- **480x320 TFT Display** (ILI9488 or compatible) with touch support.
- **Wi-Fi Access** for retrieving TLE data and syncing time.

### Software
- **Arduino IDE** or **PlatformIO** with required libraries:
  - `TFT_eSPI`
  - `Preferences`
  - `ArduinoJson`
  - `WiFi`
  - `HTTPClient`
  - `NTPClient`
  - `PNGdec`
  - `Sgp4`

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
