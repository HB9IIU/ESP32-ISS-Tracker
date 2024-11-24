#include <TFT_eSPI.h>
#include <abc48pt7b.h> // Include your custom font

TFT_eSPI tft = TFT_eSPI();

void setup() {
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // Set custom font
    tft.setFreeFont(&abc48pt7b);

    // Set text color
    tft.setTextColor(TFT_GREEN, TFT_BLACK);

    // Print text
    tft.setCursor(10, 80);
    tft.print("123");
}

void loop() {
    // Nothing here
}
