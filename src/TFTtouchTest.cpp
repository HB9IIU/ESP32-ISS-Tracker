#include <Arduino.h>
#include <TFT_eSPI.h> // Include the TFT_eSPI library for TFT display functionality

// Create an instance of the TFT screen
TFT_eSPI tft = TFT_eSPI(); // Use default settings defined in platformio.ini

void setup() {
  // Initialize Serial communication for debugging
  Serial.begin(115200);
  delay(1000); // Give time for the Serial monitor to initialize

  Serial.println();
  Serial.println("ESP32 Chip Information:");

  // Print ESP32 chip information
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  Serial.printf("Chip model: %s\n", (chip_info.model == CHIP_ESP32) ? "ESP32" : "Unknown");
  Serial.printf("Number of cores: %d\n", chip_info.cores);
  Serial.printf("Chip revision: %d\n", chip_info.revision);
  Serial.printf("Flash size: %dMB\n", spi_flash_get_chip_size() / (1024 * 1024));

  // Initialize the TFT screen
  tft.init();                  // Initialize the TFT display
  tft.setRotation(1);          // Set screen orientation (0-3)
  pinMode(TFT_BLP, OUTPUT);    // Configure the backlight pin as output
  digitalWrite(TFT_BLP, HIGH); // Turn on the backlight

  // Clear the screen and display a welcome message
  tft.fillScreen(TFT_BLACK);   // Clear the screen to black
  tft.setTextColor(TFT_WHITE); // Set text color to white
  tft.setTextSize(2);          // Set text size multiplier
  tft.setCursor(10, 10);       // Position the cursor
  tft.print("TFT Test");       // Display "TFT Test" on the screen

  // Draw some basic shapes on the screen
  tft.fillRect(50, 50, 100, 50, TFT_BLUE); // Draw a filled blue rectangle
  tft.drawCircle(150, 150, 50, TFT_RED);   // Draw a red circle outline
  tft.fillCircle(150, 150, 25, TFT_GREEN); // Draw a filled green circle

  // Configure text font and size for use in the main loop
  tft.setTextFont(7); // Set font to Font7 (customizable)
  tft.setTextSize(1); // Set text size multiplier to default size (1)
}

void loop() {
  int touchTFT = 0; // Variable to store the touch pressure value

  // Get the current touch pressure value
  touchTFT = tft.getTouchRawZ();

  // Check if the touch pressure exceeds the threshold
  if (touchTFT > 500) {
    // Clear the area where the text is displayed
    tft.fillRect(300, 200, 200, 80, TFT_BLACK); // Clear previous text area (adjust size as needed)

    // Display the touch pressure value on the screen
    tft.setCursor(300, 200);          // Set cursor position for the text
    tft.setTextColor(TFT_WHITE);      // Set text color to white
    tft.print(touchTFT);              // Print the touch pressure value

    delay(100); // Small delay to avoid rapid updates

    // Overwrite previous value with a placeholder
    tft.setCursor(300, 200);          // Reset cursor position
    tft.setTextColor(TFT_BLACK);      // Set text color to black
    tft.print(8888);                  // Display placeholder value to clear previous text
  }
}
