
#ifndef CONFIG_H
#define CONFIG_H
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


// TLE update interval
bool DEBUG_ON_TFT = false; // provides a bit more time to read messages at start-up 


// Observer location
const double OBSERVER_LATITUDE = 46.4666463;
const double OBSERVER_LONGITUDE = 6.8615008;
const double OBSERVER_ALTITUDE = 500.0;

// Display configuration
const double MIN_ELEVATION = 1.0;

#endif // CONFIG_H
