
#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h> // Include this for uint16_t and other fixed-width integer types

// Wi-Fi configuration
const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

// Alternative AP
const char* WIFI_SSID_ALT = "your-alternative-ssid";
const char* WIFI_PASSWORD_ALT  = "your-password-for-alternative-ssid";


// Satellite Catalogue Numbers
// Uncomment the satellite you are interested in (only one!!!)

int satelliteCatalogueNumber = 25544; // ISS (International Space Station)
// int satelliteCatalogueNumber = 25338; // NOAA 15 (Polar Orbiting Weather Satellite)
// int satelliteCatalogueNumber = 28654; // NOAA 18 (Polar Orbiting Weather Satellite)
// int satelliteCatalogueNumber = 33591; // NOAA 19 (Polar Orbiting Weather Satellite)
// int satelliteCatalogueNumber = 29155; // METOP-A (EUMETSAT Weather Satellite)
// int satelliteCatalogueNumber = 38771; // METOP-B (EUMETSAT Weather Satellite)
// int satelliteCatalogueNumber = 43689; // METOP-C (EUMETSAT Weather Satellite)
// int satelliteCatalogueNumber = 40940; // Fengyun 3D (Chinese Weather Satellite)
// int satelliteCatalogueNumber = 43014; // Himawari 8 (Geostationary Weather Satellite)
// int satelliteCatalogueNumber = 41866; // GOES 16 (Geostationary Weather Satellite)
// int satelliteCatalogueNumber = 43226; // GOES 17 (Geostationary Weather Satellite)
// int satelliteCatalogueNumber = 37849; // Suomi NPP (Weather and Climate Satellite)

// int satelliteCatalogueNumber = 43017; // AO-91 (AMSAT Fox-1B, LEO HAM Satellite)
// int satelliteCatalogueNumber = 43137; // AO-92 (AMSAT Fox-1D, LEO HAM Satellite)
// int satelliteCatalogueNumber = 7530;  // OSCAR 7 (AO-7, HAM Satellite)
// int satelliteCatalogueNumber = 43770; // CAS-4A (LeoSat-2, HAM Satellite)
// int satelliteCatalogueNumber = 43771; // CAS-4B (LeoSat-3, HAM Satellite)
// int satelliteCatalogueNumber = 39444; // FUNCUBE-1 (AO-73, HAM Satellite)
// int satelliteCatalogueNumber = 44832; // QO-100 (Es'hail-2, GEO HAM Satellite)
// int satelliteCatalogueNumber = 27607; // SO-50 (SaudiSat-1C, HAM Satellite)
// int satelliteCatalogueNumber = 40908; // LilacSat-2 (CAS-3H, HAM Satellite)
//int satelliteCatalogueNumber = 24278; // FO-29 (JAS-2, Fuji OSCAR 29)
// int satelliteCatalogueNumber = 39445; // EO-79 (Nayif-1, HAM Satellite)
// int satelliteCatalogueNumber = 41168; // AO-85 (AMSAT Fox-1A, LEO HAM Satellite)
// int satelliteCatalogueNumber = 25544; // ISS (International Space Station, HAM Radio)
// int satelliteCatalogueNumber = 43678; // CAS-6 (XW-4, HAM Satellite)
// int satelliteCatalogueNumber = 43743; // Diwata-2 (Philippine Microsatellite, HAM Radio)
// int satelliteCatalogueNumber = 28650; // HAMSAT (VO-52, ISRO Satellite)
// int satelliteCatalogueNumber = 43786; // Tevel-1 (Israeli HAM Satellite)
// int satelliteCatalogueNumber = 43803; // Tevel-2 (Israeli HAM Satellite)

// int satelliteCatalogueNumber = 37849; // Suomi NPP (Weather and Earth Observation)
// int satelliteCatalogueNumber = 20580; // Hubble Space Telescope (HST)
// int satelliteCatalogueNumber = 46984; // Sentinel-6 Michael Freilich (Earth Observation)




// API configuration
// TimeZoneDB is a free service that provides a comprehensive time zone database for cities worldwide. 
// get yours here https://timezonedb.com/
const char* TIMEZONE_API_KEY = "your-timezone-API-key";


// Observer location
const double OBSERVER_LATITUDE = 46.2044;
const double OBSERVER_LONGITUDE = 6.1432;
const double OBSERVER_ALTITUDE = 500.0;


// Buzzer notifications; set seconds before next pass, or 0 for none
const int beepsNotificationBeforeAOSandLOS=15;
bool notificationAtTCA=true;


// Digital clock style
constexpr bool display7DigisStyleClock = true;
constexpr uint16_t clockDigitsColor = 0xFEA0;

// Display option
const bool DISPLAY_ISS_CREW = true;

// duration of booting messages in ms
int bootingMessagePause = 1000; // for TFT messages at boot

bool DEBUG_ON_TFT = false; // provides a bit more time to read messages at start-up 



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

#endif 
