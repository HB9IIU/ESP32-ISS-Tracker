# HB9IIU ISS Life Tracker
### **Efficient Real-Time Calculations on an ESP32**

This project is an **ESP32-based tracking system for the International Space Station (ISS)** that demonstrates the impressive functionality and versatility of this microcontroller. Unlike other applications that rely on external APIs to fetch the ISS's current position, this system retrieves only the **Two-Line Elements (TLEs)** and current time from online sources. All orbital calculations are performed in real time using the **SGDP4 library**, making the solution self-contained and dynamic.

The system provides detailed information about ISS passes, including **Acquisition of Signal (AOS)**, **Time of Closest Approach (TCA)**, and **Loss of Signal (LOS)**. The data is displayed on a **480x320 TFT screen** with a touchscreen interface, offering **clear and informative visualizations** such as polar plots, azimuth/elevation graphs, and satellite footprint maps. 

This project highlights how much capability can be packed into an ESP32, handling computationally intensive tasks while remaining compact and efficient.

---

### 🚨 **NEW VERSION 2025** 🚨

With the support of those who decided to build the unit (thank you Joël, Paul, and John), several bugs have been fixed in this latest version. Along with these fixes, the following major improvements have been introduced:

1. **Enhanced Satellite Tracking:** The system can now track any existing satellite by specifying its catalogue number in the `config.h` file.
2. **Audio Notifications:** Added an output for a buzzer on pin 21 (configurable) to notify users of upcoming AOS, TCA, and LOS. This feature can be enabled or disabled by tapping on a speaker icon on the TFT screen.
3. **WebSocket Output:** Implemented WebSocket output to enable communication with a separate unit for controlling an azimuth/elevation rotor.

Additionally, TLE management has been improved, and the NTP server time synchronization has been optimized for better accuracy and reliability.

---
### Demo
<div align="center">
  <a href="https://youtu.be/Vp4qGIXc1Ag">
    <img src="https://img.youtube.com/vi/Vp4qGIXc1Ag/0.jpg" alt="HB9IIU ISS Life Tracker Demo">
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

</div>

---

## Description

This ESP32-based ISS tracking application combines satellite data processing, real-time visualization, 
and an intuitive user interface to deliver a robust experience. Here's how it works:

#####  Data Retrieval
- The application periodically fetches the latest Two-Line Element (TLE) data for the ISS from 
  primary and fallback APIs, ensuring up-to-date orbital calculations.

##### Pass Prediction
- Using the SGP4 library, it predicts satellite passes based on the user's geographical location, 
  calculating essential parameters such as azimuth, elevation, acquisition of signal (AOS), and loss of signal (LOS).

##### Real-Time Tracking
- The system updates ISS latitude, longitude, altitude, azimuth, and elevation data in real-time, 
  enabling live positional tracking.

##### Visual Display
- A TFT screen showcases detailed ISS information through:
  - Custom-rendered 7-segment clocks for precise time tracking.
  - Dynamic plots, including polar and azimuth-elevation graphs.
  - World map overlays with multi-pass predictions.
  - Additional contextual graphics such as the ISS orbit path and current crew data.

##### User-Friendly Features
- Smooth touchscreen navigation across multiple pages.
- Auto-refresh for critical information such as TLE updates, next-pass predictions, and ISS current position.
- Localized time calculations based on user-configured timezone and daylight saving settings.

##### Resilience & Redundancy
- Automated fallback mechanisms for network and data retrieval issues.
- TLE data is cached in flash memory, providing offline capabilities.

##### Interactive Insights
- Displays upcoming ISS passes with key metrics like duration, max elevation, and visibility times.
- Highlights conditions suitable for amateur radio communication.

---

## Requirements

### Hardware
- **ESP32** (ESP32-WROOM-32 or similar variations).
- **480x320 TFT Display** (ILI9488 or compatible) with touch support.
- **Wi-Fi Access** for retrieving TLE data and syncing time.

---

## Software

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

### Note
All the required libraries are already included in the repository’s `lib` folder. **There is no need to install additional libraries**—this ensures that the project compiles and runs seamlessly.

---
## Hardware Assembly

- You will find the wiring diagram in the `doc` folder of this repository.
- The assembly process is straightforward: you simply need to wire specific pins of the ESP32 to the TFT display as shown in the diagram.
- There is **no need for a custom PCB**. The connections have been made pin-to-pin directly.
- To simplify the soldering process, you can use **Dupont cables** with their plastic housings removed. This makes handling and soldering much easier.
## 3D Printing

To enhance the usability and aesthetics of the project, I have included **all necessary STL files** for 3D printing in the `doc` folder of this repository. These files allow you to print the enclosure for your hardware, ensuring a neat and organized setup.
<div align="center">
  <img src="https://github.com/HB9IIU/ESP32-ISS-Tracker/blob/main/doc/Enclosure3DprintFiles/Renderings/TFTESP32enclsoure_1.png" alt="Enclosure" width="500"> 

</div>

## Setup

1. **Copy the Repository**:  
   Download the project files from the repository:  
   [https://github.com/HB9IIU/ESP32-ISS-Tracker/archive/refs/heads/main.zip](https://github.com/HB9IIU/ESP32-ISS-Tracker/archive/refs/heads/main.zip)

2. **Install PlatformIO**:  
   PlatformIO is the development environment used to compile and upload the code to the ESP32. Follow these steps to install it:  

   - **Install Visual Studio Code (VS Code)**:  
     Download and install VS Code from the [official website](https://code.visualstudio.com/).  

   - **Install the PlatformIO IDE Extension**:  
     1. Open VS Code.  
     2. Navigate to the Extensions view by clicking on the square icon in the sidebar or pressing `Ctrl+Shift+X`.  
     3. Search for "**PlatformIO IDE**" and click "**Install**".  

   - **Verify Installation**:  
     1. Restart VS Code after installation.  
     2. Access PlatformIO by clicking on its icon (a small alien head) in the sidebar or by pressing `Ctrl+Alt+P`.

3. **Open the Project Folder**:  
   - Unzip the downloaded repository.  
   - Open VS Code.  
   - Click on **File > Open Folder**, then select the unzipped project folder.  
   - Wait for PlatformIO to automatically download all necessary dependencies (this may take a few minutes).

4. **Configure `config.h`**:  
   To run this project successfully, you need to configure the `config.h` file by providing your specific settings for Wi-Fi, API, and observer location. Follow the instructions below:

### **Wi-Fi Configuration**  
Set up your primary and alternate Wi-Fi credentials in the `config.h` file. These settings will allow your device to connect to the network.

```cpp
// Wi-Fi configuration
const char* WIFI_SSID = "your SSID";              // Replace with your primary Wi-Fi SSID
const char* WIFI_PASSWORD = "your Password";      // Replace with your primary Wi-Fi password

// Alternative AP
const char* WIFI_SSID_ALT = "your alternate SSID";   // Replace with your alternate Wi-Fi SSID
const char* WIFI_PASSWORD_ALT = "your alternate Password"; // Replace with your alternate Wi-Fi password
```

---

### **API Configuration**  
The project uses the [TimeZoneDB API](https://timezonedb.com/) to fetch accurate time zone data.  
1. Visit [TimeZoneDB](https://timezonedb.com/) and create a free account.  
2. Generate your API key.  
3. Update the following line in `config.h` with your API key:  

```cpp
// API configuration
const char* TIMEZONE_API_KEY = "your API key";     // Replace with your TimeZoneDB API key
```

---

### **Observer Location**  
To ensure accurate calculations and visualizations, update the geographic location (latitude, longitude, and altitude in meters) for the observer.

```cpp
// Observer location
const double OBSERVER_LATITUDE = 0.0;     // Replace with your latitude
const double OBSERVER_LONGITUDE = 0.0;   // Replace with your longitude
const double OBSERVER_ALTITUDE = 0.0;    // Replace with your altitude in meters
```

---

### **Example `config.h` File**  
Here’s an example of a completed `config.h` file for reference:

```cpp
// Wi-Fi configuration
const char* WIFI_SSID = "MyHomeNetwork";
const char* WIFI_PASSWORD = "SuperSecretPassword";

// Alternative AP
const char* WIFI_SSID_ALT = "OfficeNetwork";
const char* WIFI_PASSWORD_ALT = "OfficePassword123";

// API configuration
const char* TIMEZONE_API_KEY = "ABCD1234EFGH5678"; // Example API key

// Observer location
const double OBSERVER_LATITUDE = 46.4717;      // Latitude of your location
const double OBSERVER_LONGITUDE = 6.8768;     // Longitude of your location
const double OBSERVER_ALTITUDE = 400.0;       // Altitude of your location in meters
```

---

### **Important Notes**  
- Ensure the Wi-Fi credentials and API key are accurate and match your setup.  
- Latitude and longitude values should reflect your exact location for best results. Use [Google Maps](https://maps.google.com) or a GPS device to find these coordinates.  
- The altitude value must be in meters.

---

5. **Compile and Upload the Code**:  
   - Connect your ESP32 to your computer using a USB cable.  
   - In VS Code, open the **PlatformIO toolbar** (left sidebar).  
   - Click on the **"Build"** button (checkmark icon) to compile the code.  
     - If the compilation succeeds, proceed to the next step.  
   - Click on the **"Upload"** button (arrow icon) to upload the code to your ESP32.

6. **Run the Tracker**:  
   Once the code is uploaded successfully:  
   - The TFT screen will display a splash image.  
   - The ESP32 will connect to Wi-Fi and start retrieving ISS tracking data.  
   - The screen will update with real-time information about the ISS.

---

## Contributing

Contributions are welcome! If you’d like to improve this project, please:
- Fork the repository.
- Create a new branch for your feature or bugfix.
- Submit a pull request with a detailed description.

## Acknowledgments

A heartfelt thank you to the authors and contributors of the libraries used in this project. Your work has made it possible to bring this project to life. Each library brings a unique capability, and we deeply appreciate the time, effort, and expertise invested in creating and maintaining them. 🙏

## License

This project is licensed under the [MIT License](LICENSE).

## Author

**HB9IIU - Daniel**  
*Amateur Radio Enthusiast & Developer*  
Contact: daniel at hb9iiu.com

### A Personal Note

This is my very first project on GitHub, so please excuse any rough edges, chaotic code, or moments of "what was this person thinking?" I'm still figuring things out as I go! Your feedback, suggestions, or even just your silent nod of approval would mean a lot to me—and might just save future me from cringing too hard. Thanks for stopping by and checking out my little corner of GitHub! 🙌
![ESP32-1](https://raw.githubusercontent.com/HB9IIU/ESP32-ISS-Tracker/main/doc/Misc/ESP32-1.png)
