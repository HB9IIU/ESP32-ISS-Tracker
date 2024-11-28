# HB9IIU ISS Life Tracker
### **Efficient Real-Time Calculations on an ESP32**

This project is an **ESP32-based tracking system for the International Space Station (ISS)** that demonstrates the impressive functionality and versatility of this microcontroller. Unlike other applications that rely on external APIs to fetch the ISS's current position, this system retrieves only the **Two-Line Elements (TLEs)** and current time from online sources. All orbital calculations are performed in real time using the **SGDP4 library**, making the solution self-contained and dynamic.

The system provides detailed information about ISS passes, including **Acquisition of Signal (AOS)**, **Time of Closest Approach (TCA)**, and **Loss of Signal (LOS)**. The data is displayed on a **480x320 TFT screen** with a touchscreen interface, offering **clear and informative visualizations** such as polar plots, azimuth/elevation graphs, and satellite footprint maps. 

This project highlights how much capability can be packed into an ESP32, handling computationally intensive tasks while remaining compact and efficient.

---
### Demo
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
   Update the following parameters in the `config.h` file to match your Wi-Fi and location:  
   ```cpp
   #define WIFI_SSID "your-ssid"
   #define WIFI_PASSWORD "your-password"
   #define OBSERVER_LATITUDE 46.4717
   #define OBSERVER_LONGITUDE 6.8768
   #define OBSERVER_ALTITUDE 400
   ```

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

## Acknowledgments

A heartfelt thank you to the authors and contributors of the libraries used in this project. Your work has made it possible to bring this project to life. Each library brings a unique capability, and we deeply appreciate the time, effort, and expertise invested in creating and maintaining them. 🙏

## License

This project is licensed under the [MIT License](LICENSE).

## Author

**HB9IIU - Daniel**  
*Amateur Radio Enthusiast & Developer*  
[Contact Me](mailto:daniel@hb9iiu.com)

### A Personal Note

This is my very first project published on GitHub, and I sincerely apologize if you find any parts of it incomplete, messy, or lacking clarity. I'm still learning and growing, and I appreciate your patience and understanding. Your feedback or suggestions for improvement are most welcome and will help me refine my skills. Thank you for checking out my project! 🙏