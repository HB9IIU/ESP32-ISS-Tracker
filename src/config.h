
#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h> // Include this for uint16_t and other fixed-width integer types

// Version Info
const char* VERSION_NUMBER = "Beta 0";
const char* VERSION_DATE = "01.12.24";


// Wi-Fi configuration
const char* WIFI_SSID = "MESH";
const char* WIFI_PASSWORD = "Nestle2010Nestle";

// Alternative AP
const char* WIFI_SSID_ALT = "NO WIFI FOR YOU!!!";
const char* WIFI_PASSWORD_ALT  = "Nestle2010Nestle";

// API configuration
// TimeZoneDB is a free service that provides a comprehensive time zone database for cities worldwide. 
// get yours here https://timezonedb.com/
const char* TIMEZONE_API_KEY = "EH7POYI19YHB";


// Observer location
const double OBSERVER_LATITUDE = 46.4666463;
const double OBSERVER_LONGITUDE = 6.8615008;
const double OBSERVER_ALTITUDE = 500.0;

// Digital clock style
constexpr bool SEVEN_DIGIT_STYLE = true;
constexpr uint16_t SEVEN_DIGIT_COLOR = 0xFEA0;

// Display option
const bool DISPLAY_ISS_CREW = true;

/* Predefined Colors in RGB565 format
 TFT_BLACK       0x0000  // Black
 TFT_NAVY        0x000F  // Dark Blue
 TFT_DARKGREEN   0x03E0  // Dark Green
 TFT_DARKCYAN    0x03EF  // Dark Cyan
 TFT_MAROON      0x7800  // Dark Red
 TFT_GOLD        0xFEA0  // Gold 
 TFT_PURPLE      0x780F  // Purple
 TFT_OLIVE       0x7BE0  // Olive
 TFT_LIGHTGREY   0xC618  // Light Grey
 TFT_DARKGREY    0x7BEF  // Dark Grey
 TFT_BLUE        0x001F  // Blue
 TFT_GREEN       0x07E0  // Green
 TFT_CYAN        0x07FF  // Cyan
 TFT_RED         0xF800  // Red
 TFT_MAGENTA     0xF81F  // Magenta
 TFT_YELLOW      0xFFE0  // Yellow
 TFT_WHITE       0xFFFF  // White
 TFT_ORANGE      0xFD20  // Orange
 TFT_GREENYELLOW 0xAFE5  // Green-Yellow
 TFT_PINK        0xF81F  // Pink (alias for Magenta)
*/

bool DEBUG_ON_TFT = false; // provides a bit more time to read messages at start-up 

// Display configuration
const double MIN_ELEVATION = 0;

#endif // CONFIG_H
