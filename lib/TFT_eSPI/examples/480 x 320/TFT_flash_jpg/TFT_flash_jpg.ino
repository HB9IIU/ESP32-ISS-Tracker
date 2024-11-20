#include <SPI.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>

TFT_eSPI tft = TFT_eSPI();

// Define the minimum macro
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

// Include your JPEG images
#include "jpeg1.h"
#include "jpeg2.h"
#include "jpeg3.h"
#include "jpeg4.h"

// Function prototypes
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos);
void jpegInfo();
void renderJPEG(int xpos, int ypos);

// Global counter for image draws
uint32_t icount = 0;

// Setup function
void setup() {
  Serial.begin(115200);
  tft.begin();
}

void loop() {
  tft.setRotation(2);  // Set orientation to portrait
  tft.fillScreen(random(0xFFFF));

  // Calculate position for centering the image
  int x = (tft.width()  - 300) / 2 - 1;
  int y = (tft.height() - 300) / 2 - 1;

  // Draw the first JPEG image
  drawArrayJpeg(EagleEye, sizeof(EagleEye), x, y);
  delay(2000);

  // Draw the second JPEG image
  tft.fillScreen(random(0xFFFF));
  drawArrayJpeg(Baboon40, sizeof(Baboon40), 0, 0);
  delay(2000);

  // Draw the third JPEG image
  tft.fillScreen(random(0xFFFF));
  drawArrayJpeg(lena20k, sizeof(lena20k), 0, 0);
  delay(2000);

  // Draw the fourth JPEG image, testing cropping
  tft.setRotation(1);  // Set orientation to landscape
  tft.fillScreen(random(0xFFFF));
  drawArrayJpeg(Mouse480, sizeof(Mouse480), 100, 100);
  delay(2000);
}

void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {
  JpegDec.decodeArray(arrayname, array_size);
  jpegInfo(); // Print JPEG information
  renderJPEG(xpos, ypos); // Render the JPEG on the TFT
  Serial.println("#########################");
}

void renderJPEG(int xpos, int ypos) {
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width + xpos;
  uint32_t max_y = JpegDec.height + ypos;

  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  uint32_t drawTime = millis();

  while (JpegDec.read()) {
    pImg = JpegDec.pImage;

    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    uint32_t mcu_pixels = win_w * win_h;

    tft.startWrite();
    if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height()) {
      tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);
      while (mcu_pixels--) {
        tft.pushColor(*pImg++);
      }
    } else if ((mcu_y + win_h) >= tft.height()) JpegDec.abort();
    tft.endWrite();
  }

  drawTime = millis() - drawTime;
  Serial.print(F("Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
}

void jpegInfo() {
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F("Width      :")); Serial.println(JpegDec.width);
  Serial.print(F("Height     :")); Serial.println(JpegDec.height);
  Serial.print(F("Components :")); Serial.println(JpegDec.comps);
  Serial.print(F("MCU / row  :")); Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F("MCU / col  :")); Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F("Scan type  :")); Serial.println(JpegDec.scanType);
  Serial.print(F("MCU width  :")); Serial.println(JpegDec.MCUWidth);
  Serial.print(F("MCU height :")); Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}

void showTime(uint32_t msTime) {
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}
