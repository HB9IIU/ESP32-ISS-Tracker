
#ifndef CONFIG_H
#define CONFIG_H
// Version Info
const char* VERSION_NUMBER = "Beta0";
const char* VERSION_DATE = "14.11.24";


// Wi-Fi configuration
const char* WIFI_SSID = "MESH";
const char* WIFI_PASSWORD = "Nestle2010Nestle";

// Alternative AP
const char* WIFI_SSID_ALT = "NO WIFI FOR YOU!!!";
const char* WIFI_PASSWORD_ALT  = "Nestle2010Nestle";

// API configuration
const char* TIMEZONE_API_KEY = "EH7POYI19YHB";


// TLE update interval
const unsigned long TLE_UPDATE_INTERVAL = 24 * 3600; // 8 hours


// Observer location
const double OBSERVER_LATITUDE = 46.4666463;
const double OBSERVER_LONGITUDE = 6.8615008;
const double OBSERVER_ALTITUDE = 500.0;

// Display configuration
const double MIN_ELEVATION = 1.0;

#endif // CONFIG_H
