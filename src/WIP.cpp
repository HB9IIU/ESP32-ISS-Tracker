#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Sgp4.h>
#include <Ticker.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <Preferences.h> // For flash storage of TLE data
#include <PNGdec.h>      // Include the PNG decoder library

// TFT Setup
TFT_eSPI tft = TFT_eSPI();
// PNG decoder instance
PNG png;

#include "ISSsplashImage.h" // Image is stored here in an 8-bit array
#include "worldMap.h"       // Image is stored here in an 8-bit array
// #include "blueMap.h"        // Image is stored here in an 8-bit array
#include "patreon.h" // Image is stored here in an 8-bit array
#include "expedition72.h"
// TLE data URL
const char *tleUrlMain = "https://tle.ivanstanojevic.me/api/tle/25544";
const char *tleUrlFallback = "https://live.ariss.org/iss.txt";

// TLE Storage in Flash
Preferences preferences;
const char *TLE_PREF_NAMESPACE = "TLE_data";
const char *TLE_LINE1_KEY = "tle_line1";
const char *TLE_LINE2_KEY = "tle_line2";
const char *TLE_TIMESTAMP_KEY = "tle_timestamp";
char TLE_age[9]; // HH:MM:SS + null-terminator
bool refresh = false;
// Flag to track if time was successfully updated at least once
bool timeInitialized = false;
// NTP Client for UTC time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "129.6.15.28", 0, 60000);

// for the touchscreen (CLEANUP LATER)
int touchTFT = 0;
int touchCounter = 1;
unsigned long lastTouchTime = 0;
unsigned long debounceDelay = 200; // debounce delay in milliseconds
// Flags to track if each page has been displayed
bool page1Displayed = false;
bool page2Displayed = false;
bool page3Displayed = false;
bool page4Displayed = false;
bool page5Displayed = false;
bool page6Displayed = false;

Sgp4 sat;
Ticker tkSecond;

char tleLine1[70];
char tleLine2[70];

unsigned long unixtime;
// Variables to store offset data
int timezoneOffset; // Offset in seconds, will be updated during setup for your location
int dstOffset;      // DST offset in seconds, will be updated during setup for your location

// Global variables for orbit calculation
float meanMotion = 0.0;     // Mean motion (revolutions per day)
unsigned long tleEpoch = 0; // TLE epoch time (in seconds since Unix epoch)

// Next pass information
unsigned long nextPassStart = 0;
unsigned long nextPassEnd = 0;
double nextPassAOSAzimuth = 0;
double nextPassLOSAzimuth = 0;
double nextPassMaxTCA = 0;
double nextPassPerigee = 0;
unsigned long nextPassCulminationTime = 0;
float culminationAzimuth = 0.0; // Variable to store azimuth at culmination
unsigned long passDuration = 0; // Duration of the pass in seconds
unsigned long passMinutes = 0;  // Pass duration in minutes
unsigned long passSeconds = 0;  // Remaining seconds after minutes
// Retry and error-checking settings
const int MAX_NTP_RETRIES = 5; // Maximum number of attempts to get time from NTP server
int orbitNumber;

// Function declarations
void displayRawNumberRightAligned(int rightEdgeX, int y, int number, int color);
void displayFormattedNumberRightAligned(int rightEdgeX, int y, int number, int color);
void displayTimeRightAligned(int rightEdgeX, int y, int seconds, int color);
void displayMainPage();
void initializeTFT()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK); // Clears the screen to black
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
}
void TFTprint(const String &text, uint16_t color = TFT_WHITE)
{
  // Constants for font and screen dimensions
  const int FONT_HEIGHT = 28;    // Approximate height of FONT4; adjust if necessary
  const int SCREEN_HEIGHT = 320; // Screen height for 480x320 TFT
  static int currentLine = 0;    // Track the current line position

  // Initialize TFT settings
  tft.setTextFont(4);      // Use FONT4
  tft.setTextColor(color); // Set text color with a transparent background

  // Calculate the y-position for the current line
  int yPos = currentLine * FONT_HEIGHT;

  // If we've reached the bottom of the screen, reset to the top
  if (yPos + FONT_HEIGHT > SCREEN_HEIGHT)
  {
    tft.fillScreen(TFT_BLACK); // Clear the screen
    currentLine = 0;           // Reset line counter
    yPos = 0;
  }

  // Set cursor to the beginning of the line and print text
  tft.setCursor(0, yPos); // Start at x = 0 for each line
  tft.print(text);

  // Move to the next line for subsequent calls
  currentLine++;
}
void printWelcomeMessage()
{

  for (int i = 0; i < 30; i++)
  { // Adjust as needed
    Serial.println();
  }
  String title = "Welcome to HB9IIU ISS Life Tracker";
  String version = String("Version: ") + VERSION_NUMBER + " (" + VERSION_DATE + ")";
  String disclaimer = "Please use at your own risk!";

  TFTprint(title);
  TFTprint(version);
  TFTprint(disclaimer);
  int width = max(max(title.length(), version.length()), disclaimer.length()) + 4; // Adjust frame width

  // Print top border
  Serial.println();
  Serial.print("+");
  for (int i = 0; i < width; i++)
    Serial.print("-");
  Serial.println("+");

  // Print empty line for padding
  Serial.print("|");
  for (int i = 0; i < width; i++)
    Serial.print(" ");
  Serial.println("|");

  // Function to print a line with centered text
  auto printCenteredLine = [&](String text)
  {
    int padding = (width - text.length()) / 2;
    Serial.print("|");
    for (int i = 0; i < padding; i++)
      Serial.print(" ");
    Serial.print(text);
    for (int i = 0; i < width - text.length() - padding; i++)
      Serial.print(" ");
    Serial.println("|");
  };

  // Print title, version, and disclaimer with centered alignment
  printCenteredLine(title);
  printCenteredLine(version);
  printCenteredLine(disclaimer);

  // Print empty line for padding
  Serial.print("|");
  for (int i = 0; i < width; i++)
    Serial.print(" ");
  Serial.println("|");

  // Print bottom border
  Serial.print("+");
  for (int i = 0; i < width; i++)
    Serial.print("-");
  Serial.println("+");
  Serial.println();
}
void clearPreferences()
{
  Preferences preferences;
  preferences.begin(TLE_PREF_NAMESPACE, false); // Open in write mode
  preferences.clear();                          // Clears all keys in the namespace
  preferences.end();
  Serial.println("All preferences cleared.");
}
void connectToWiFi()
{
  int attempt = 0;
  const int maxAttempts = 5; // Maximum number of attempts for each network
  bool connected = false;

  while (!connected)
  {
    if (attempt < maxAttempts)
    {
      Serial.print("Connecting to primary Wi-Fi: ");
      Serial.println(WIFI_SSID);

      String message = "Connecting to primary Wi-Fi: " + String(WIFI_SSID);
      TFTprint(message, TFT_YELLOW);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    else
    {
      Serial.print("Connecting to alternative Wi-Fi: ");
      Serial.println(WIFI_SSID_ALT);
      String message = "Connecting to alternative Wi-Fi: " + String(WIFI_SSID_ALT);
      TFTprint(message);
      WiFi.begin(WIFI_SSID_ALT, WIFI_PASSWORD_ALT);
    }

    // Check Wi-Fi connection status
    for (int i = 0; i < 10; i++)
    { // Wait up to 5 seconds
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println("\nConnected to Wi-Fi.");
        TFTprint("Connected to Wi-Fi", TFT_GREEN);

        connected = true;
        break;
      }
      delay(500);
      Serial.print(".");
    }

    // If not connected, try again or switch to the alternative network
    if (!connected)
    {
      WiFi.disconnect();
      attempt++;
      if (attempt == maxAttempts * 2)
      {
        Serial.println("\nFailed to connect to both networks. Retrying...");
        TFTprint("Failed to connect to both networks. Retrying...", TFT_RED);

        attempt = 0; // Reset attempts to retry both networks
      }
      else if (attempt == maxAttempts)
      {
        Serial.println("\nSwitching to alternative network...");
        TFTprint("Switching to alternative network...", TFT_YELLOW);
      }
    }
  }
}
// Function to retrieve timezone data from TimeZoneDB API
void getTimezoneData()
{
  // Inform the user that the process of getting timezone data is starting
  Serial.println("Retrieving timezone data...");
  TFTprint("Retrieving timezone data...", TFT_YELLOW);

  // Time Zone Database URL with placeholders
  String timezoneUrl = "http://api.timezonedb.com/v2.1/get-time-zone?key=" + String(TIMEZONE_API_KEY) + "&format=json&by=position&lat=" + String(OBSERVER_LATITUDE) + "&lng=" + String(OBSERVER_LONGITUDE);

  Serial.println("");
  Serial.println(timezoneUrl); // Optional debug print for URL

  int maxRetries = 3; // Number of retry attempts
  int attempt = 0;    // Track the current attempt
  String timeZoneName;

  while (attempt < maxRetries)
  {
    attempt++;
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      http.begin(timezoneUrl);
      int httpCode = http.GET();

      if (httpCode == 200)
      {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error)
        {
          timezoneOffset = doc["gmtOffset"];        // Offset in seconds
          dstOffset = (doc["dst"] == 1) ? 3600 : 0; // 1 hour if DST is active
          timeZoneName = doc["zoneName"].as<String>();
          http.end(); // End the HTTP request

          Serial.println("Timezone data retrieved successfully!");
          Serial.print("Timezone Name: ");
          Serial.println(timeZoneName);
          Serial.print("Timezone Offset (seconds): ");
          Serial.println(timezoneOffset);
          Serial.print("DST Offset (seconds): ");
          Serial.println(dstOffset);

          // Print to TFT
          TFTprint("Timezone data retrieved successfully!", TFT_GREEN);
          TFTprint("Timezone Name: " + String(timeZoneName), TFT_GREEN);
          TFTprint("Timezone Offset (seconds): " + String(timezoneOffset), TFT_GREEN);
          TFTprint("DST Offset (seconds): " + String(dstOffset), TFT_GREEN);

          return; // Exit function on success
        }
        else
        {
          Serial.println("JSON deserialization error.");
          TFTprint("JSON deserialization error.", TFT_RED);
        }
      }
      else
      {
        Serial.print("HTTP error: ");
        Serial.println(httpCode);
        TFTprint("HTTP error: " + String(httpCode), TFT_YELLOW);
      }
      http.end(); // End the HTTP request
    }
    else
    {
      Serial.println("Not connected to WiFi.");
      TFTprint("Not connected to WiFi.", TFT_RED);
    }

    // Retry delay
    delay(2000); // Wait 2 seconds before the next attempt
  }

  // If we exit the loop, all retries failed, reboot the ESP32
  Serial.println("Failed to retrieve timezone data after multiple attempts. Rebooting...");
  TFTprint("Failed to retrieve timezone data. Rebooting...", TFT_RED);
  delay(1000);   // Brief delay to display the message before reboot
  ESP.restart(); // Reboot the ESP32
}
// Function to synchronize time from NTP servers
void syncTimeFromNTP()
{
  // List of NTP server IPs with corresponding server names
  Serial.println();
  // Print a message indicating the start of the time synchronization process
  Serial.println("Synchronizing time from NTP servers...");
  TFTprint("Synchronizing time from NTP servers...", TFT_YELLOW);
  const char *ntpServers[] = {
      "216.239.35.0",    // Google NTP
      "132.163.96.1",    // NIST NTP
      "162.159.200.123", // Cloudflare NTP
      "129.6.15.28",     // Pool server
      "193.67.79.202"    // Time server (Europe)
  };

  // Corresponding server names for debugging
  const char *serverNames[] = {
      "time.google.com",     // Google NTP
      "time.nist.gov",       // NIST NTP
      "time.cloudflare.com", // Cloudflare NTP
      "time.windows.com",    // Pool server
      "time.europe.com"      // Time server (Europe)
  };

  const int numServers = sizeof(ntpServers) / sizeof(ntpServers[0]);

  int ntpRetries = 0;
  bool success = false;

  // Loop through the list of NTP servers
  for (int serverIndex = 0; serverIndex < numServers; serverIndex++)
  {
    // Print the server name being contacted
    Serial.print("Trying NTP server: ");
    Serial.println(serverNames[serverIndex]);
    TFTprint("Trying NTP server: " + String(serverNames[serverIndex]), TFT_CYAN);

    // Set NTP client to use the current server
    timeClient.end();                                      // Stop the current client
    timeClient.setPoolServerName(ntpServers[serverIndex]); // Set the new NTP server to use
    timeClient.begin();                                    // Re-initialize the NTP client with the new server

    ntpRetries = 0; // Reset retry counter for this server

    // Retry logic for NTP update
    while (ntpRetries < MAX_NTP_RETRIES)
    {
      timeClient.update();                  // Attempt to update the time from the NTP server
      unixtime = timeClient.getEpochTime(); // Get the current UNIX time

      // If the time is valid, break the retry loop
      if (unixtime > 1000000000)
      {
        if (!timeInitialized)
        {
          Serial.println("NTP time updated successfully.");
          TFTprint("NTP time updated successfully.", TFT_GREEN);
          timeInitialized = true; // Mark time as initialized
        }
        success = true; // Time sync was successful
        break;          // Exit the retry loop
      }
      else
      {
        Serial.println("NTP update failed, retrying...");
        TFTprint("NTP update failed, retrying...", TFT_YELLOW);
        ntpRetries++; // Increment retry count
        delay(1000);  // Wait before retrying
      }
    }

    // If time sync was successful, exit the server loop
    if (success)
    {
      break;
    }
  }

  // If all servers fail, reboot the device
  if (!success)
  {
    Serial.println("All NTP servers failed after multiple attempts. Rebooting...");
    TFTprint("All NTP servers failed. Rebooting...", TFT_RED);
    delay(1000);   // Brief delay before reboot
    ESP.restart(); // Reboot the ESP32
  }
}

bool getTLEelementsFallback(char *tleLine1, char *tleLine2)
{
  Serial.println("");
  Serial.println("Attempting to fetch TLE data (Fallback Method)...");
  TFTprint("Fetching TLE data (Fallback)...", TFT_YELLOW);

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(tleUrlFallback);
    int httpCode = http.GET();

    if (httpCode == 200)
    {
      String payload = http.getString();
      int line1Start = payload.indexOf('\n') + 1;
      int line2Start = payload.indexOf('\n', line1Start) + 1;

      if (line1Start > 0 && line2Start > line1Start)
      {
        payload.substring(line1Start, line1Start + 69).toCharArray(tleLine1, 70);
        payload.substring(line2Start, line2Start + 69).toCharArray(tleLine2, 70);

        // Extract TLE epoch from Line 1
        int epochYear = String(tleLine1).substring(18, 20).toInt() + 2000;
        float epochDay = String(tleLine1).substring(20, 32).toFloat();
        struct tm epochTime = {0};
        epochTime.tm_year = epochYear - 1900;
        epochTime.tm_mday = 1;
        epochTime.tm_mon = 0;
        time_t tleEpochUnix = mktime(&epochTime) + (unsigned long)((epochDay - 1) * 86400);

        // Validate unixtime and tleEpochUnix
        Serial.print("unixtime: ");
        Serial.println(unixtime);
        Serial.print("tleEpochUnix: ");
        Serial.println(tleEpochUnix);

        if (unixtime <= tleEpochUnix)
        {
          Serial.println("Error: Invalid unixtime or TLE epoch.");
          TFTprint("Invalid time data.", TFT_RED);
          return false;
        }

        // Calculate TLE age
        unsigned long ageInSeconds = unixtime - tleEpochUnix;

        unsigned long ageInDays = ageInSeconds / 86400;
        unsigned long remainingSeconds = ageInSeconds % 86400;
        unsigned long ageInHours = remainingSeconds / 3600;
        unsigned long ageInMinutes = (remainingSeconds % 3600) / 60;

        // Format TLE age as DD:HH:MM
        char ageFormatted[30];
        sprintf(ageFormatted, "%lu days, %lu hours, %lu minutes", ageInDays, ageInHours, ageInMinutes);

        // Format TLE age as HH:MM and assign it to TLE_age
        sprintf(TLE_age, "%02lu:%02lu", ageInHours, ageInMinutes); // Format as HH:MM

        // Store TLE data in preferences
        preferences.begin(TLE_PREF_NAMESPACE, false);
        preferences.putString(TLE_LINE1_KEY, tleLine1);
        preferences.putString(TLE_LINE2_KEY, tleLine2);
        preferences.putULong(TLE_TIMESTAMP_KEY, unixtime);
        preferences.end();

        Serial.println("TLE data fetched and stored successfully (Fallback Method).");
        Serial.println("TLE Line 1: " + String(tleLine1));
        Serial.println("TLE Line 2: " + String(tleLine2));
        Serial.println("TLE age: " + String(ageFormatted));

        TFTprint("TLE fetched and stored.", TFT_GREEN);
        TFTprint("TLE Line 1: ", TFT_GREEN);
        TFTprint(String(tleLine1), TFT_CYAN);
        TFTprint("", TFT_BLACK);
        TFTprint("TLE Line 2: ", TFT_GREEN);
        TFTprint(String(tleLine2), TFT_CYAN);
        TFTprint("", TFT_BLACK);
        TFTprint("TLE Age: " + String(ageFormatted), TFT_WHITE);
        return true;
      }
      else
      {
        Serial.println("Error parsing TLE data.");
        TFTprint("Error parsing TLE data.", TFT_RED);
      }
    }
    else
    {
      Serial.print("HTTP error while fetching TLE: ");
      Serial.println(httpCode);
      TFTprint("HTTP error: " + String(httpCode), TFT_RED);
    }
    http.end();
  }
  else
  {
    Serial.println("Not connected to WiFi. Cannot fetch TLE data.");
    TFTprint("WiFi not connected.", TFT_RED);
  }

  // If fetching TLE data fails
  Serial.println("Failed to fetch TLE data. Will reboot in 3 seconds...");
  TFTprint("TLE fetch failed. Rebooting in 3s...", TFT_RED);
  delay(3000); // Wait for 3 seconds before reboot
  ESP.restart();
  return false; // This line won't actually execute due to the reboot
}

bool getTLEelementsMain(char *tleLine1, char *tleLine2)
{
  bool tleLoaded = false;
  bool isFirstDownload = false;
  bool isDataUpdated = false;

  Serial.println("Attempting to fetch TLE data (Main Method)...");
  TFTprint("Fetching TLE data (Main)...", TFT_YELLOW);

  // Try to load TLE from flash
  preferences.begin(TLE_PREF_NAMESPACE, true);
  String storedLine1 = preferences.getString(TLE_LINE1_KEY, "");
  String storedLine2 = preferences.getString(TLE_LINE2_KEY, "");

  preferences.end();

  if (storedLine1.length() == 69 && storedLine2.length() == 69)
  {
    // Extract TLE epoch from storedLine1
    int epochYear = storedLine1.substring(18, 20).toInt() + 2000; // Year (2-digit to full year)
    float epochDay = storedLine1.substring(20, 32).toFloat();     // Day of the year
    struct tm epochTime = {0};
    epochTime.tm_year = epochYear - 1900; // tm_year is years since 1900
    epochTime.tm_mday = 1;                // Start from January 1st
    epochTime.tm_mon = 0;
    time_t tleEpochUnix = mktime(&epochTime) + (unsigned long)((epochDay - 1) * 86400);

    // Calculate age of TLE
    unsigned long ageInSeconds = unixtime - tleEpochUnix;
    unsigned long ageInDays = ageInSeconds / 86400;
    unsigned long remainingSeconds = ageInSeconds % 86400;
    unsigned long ageInHours = remainingSeconds / 3600;
    unsigned long ageInMinutes = (remainingSeconds % 3600) / 60;

    // Format TLE age as DD:HH:MM
    char ageFormatted[30];
    sprintf(ageFormatted, "%lu days, %lu hours, %lu minutes", ageInDays, ageInHours, ageInMinutes);

    // Format TLE age as HH:MM and assign it to TLE_age
    sprintf(TLE_age, "%02lu:%02lu", ageInHours, ageInMinutes); // Format as HH:MM

    // Print and TFT display the TLE data
    TFTprint("TLE Age: " + String(ageFormatted), TFT_WHITE);

    Serial.println("TLE lines are valid:");
    Serial.println("TLE Line 1: " + storedLine1);
    Serial.println("TLE Line 2: " + storedLine2);
    Serial.println("TLE Age: " + String(ageFormatted));

    TFTprint("TLE lines are valid.", TFT_GREEN);
    TFTprint("TLE Line 1: ", TFT_GREEN);
    TFTprint(storedLine1, TFT_CYAN);
    TFTprint("", TFT_BLACK);
    TFTprint("TLE Line 2: ", TFT_GREEN);
    TFTprint(storedLine2, TFT_CYAN);
    TFTprint("", TFT_BLACK);

    if (ageInSeconds < TLE_UPDATE_INTERVAL)
    {
      // Data is valid and within update interval
      storedLine1.toCharArray(tleLine1, 70);
      storedLine2.toCharArray(tleLine2, 70);
      tleLoaded = true;
    }
    else
    {
      // Data is too old, fetch new data
      Serial.println("Stored TLE is outdated. Fetching new TLE data...");
      TFTprint("TLE outdated. Fetching new...", TFT_YELLOW);
      isDataUpdated = true;
    }
  }
  else
  {
    // No valid TLE data found in preferences
    Serial.println("No TLE data found. This is the first download.");
    TFTprint("No TLE found. First download.", TFT_YELLOW);
    isFirstDownload = true;
  }

  // Fetch new TLE data from the internet if necessary
  if (!tleLoaded && WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(tleUrlMain); // Use the main TLE URL
    int httpCode = http.GET();

    if (httpCode == 200)
    {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);

      DeserializationError error = deserializeJson(doc, payload);
      if (!error)
      {
        // Extract TLE data
        String line1 = doc["line1"];
        String line2 = doc["line2"];
        String tleDate = doc["date"]; // ISO 8601 format

        // Parse ISO 8601 date into Unix timestamp
        struct tm tleTime = {0};
        sscanf(tleDate.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
               &tleTime.tm_year, &tleTime.tm_mon, &tleTime.tm_mday,
               &tleTime.tm_hour, &tleTime.tm_min, &tleTime.tm_sec);
        tleTime.tm_year -= 1900; // Adjust year
        tleTime.tm_mon -= 1;     // Adjust month (0-11)
        time_t tleEpochUnix = mktime(&tleTime);

        // Calculate TLE age
        unsigned long ageInSeconds = unixtime - tleEpochUnix;
        unsigned long ageInDays = ageInSeconds / 86400;
        unsigned long remainingSeconds = ageInSeconds % 86400;
        unsigned long ageInHours = remainingSeconds / 3600;
        unsigned long ageInMinutes = (remainingSeconds % 3600) / 60;

        // Format TLE age
        // Format TLE age
        char ageFormatted[50];
        sprintf(ageFormatted, "%lu days, %lu hours, %lu minutes", ageInDays, ageInHours, ageInMinutes);

        // Format TLE age as HH:MM and assign it to TLE_age
        sprintf(TLE_age, "%02lu:%02lu", ageInHours, ageInMinutes); // Format as HH:MM

        // Print the results
        printf("Formatted TLE age (DD:HH:MM): %s\n", ageFormatted);
        printf("Formatted TLE age (HH:MM:SS): %s\n", TLE_age);

        line1.toCharArray(tleLine1, 70);
        line2.toCharArray(tleLine2, 70);
        tleLoaded = true;

        // Store TLE data in preferences
        preferences.begin(TLE_PREF_NAMESPACE, false);
        preferences.putString(TLE_LINE1_KEY, tleLine1);
        preferences.putString(TLE_LINE2_KEY, tleLine2);
        preferences.putULong(TLE_TIMESTAMP_KEY, unixtime);
        preferences.end();

        // Print success message
        Serial.println("TLE data fetched and stored successfully (Main Method).");
        Serial.println("TLE Line 1: " + String(tleLine1));
        Serial.println("TLE Line 2: " + String(tleLine2));
        Serial.println("TLE Age: " + String(ageFormatted));

        TFTprint("TLE fetched and stored.", TFT_GREEN);
        TFTprint("TLE Line 1: ", TFT_GREEN);
        TFTprint(String(tleLine1), TFT_CYAN);
        TFTprint("", TFT_BLACK);
        TFTprint("TLE Line 2: ", TFT_GREEN);
        TFTprint(String(tleLine2), TFT_CYAN);
        TFTprint("", TFT_BLACK);
        TFTprint("TLE Age: " + String(ageFormatted), TFT_WHITE);
      }
      else
      {
        Serial.println("JSON parsing error while fetching TLE data.");
        TFTprint("JSON parsing error.", TFT_RED);
      }
    }
    else
    {
      Serial.print("HTTP error while fetching TLE: ");
      Serial.println(httpCode);
      TFTprint("HTTP error: " + String(httpCode), TFT_RED);
    }
    http.end();
  }
  else if (!tleLoaded)
  {
    Serial.println("WiFi not connected. Cannot fetch TLE data.");
    TFTprint("WiFi not connected.", TFT_RED);
  }

  // Use fallback if TLE data couldn't be fetched or loaded
  if (!tleLoaded)
  {
    Serial.println("Main method failed. Falling back...");
    TFTprint("Main method failed. Falling back...", TFT_YELLOW);
    return getTLEelementsFallback(tleLine1, tleLine2);
  }

  // Final messages for first download or update
  if (tleLoaded && isFirstDownload)
  {
    Serial.println("First TLE download complete.");
    TFTprint("First TLE download complete.", TFT_GREEN);
  }
  else if (tleLoaded && isDataUpdated)
  {
    Serial.println("TLE data updated successfully.");
    TFTprint("TLE updated successfully.", TFT_GREEN);
  }

  return tleLoaded;
}

// Main Function: Calculate and update the global orbitNumber
void getOrbitNumber(time_t t)
{
  char tempstr[13]; // Temporary string for parsing
  int baselineOrbitNumber;
  int year;
  float epochDay;
  float meanMotion;

  // Extract Revolution Number (Baseline Orbit Number) from TLE Line 2
  char revNumStr[6] = {0};              // Maximum 5 characters + null terminator
  strncpy(revNumStr, &tleLine2[63], 5); // Extract columns 63–67
  revNumStr[5] = '\0';                  // Null-terminate
  baselineOrbitNumber = atoi(revNumStr);
  if (baselineOrbitNumber <= 0)
  {
    Serial.println("Error: Invalid baseline orbit number.");
    return;
  }

  // Extract Epoch Year (columns 19-20 in Line 1)
  strncpy(tempstr, &tleLine1[18], 2); // Extract columns 19-20
  tempstr[2] = '\0';
  int epochYear = atoi(tempstr);
  year = (epochYear < 57) ? (epochYear + 2000) : (epochYear + 1900);

  // Extract Epoch Day (columns 21-32 in Line 1)
  strncpy(tempstr, &tleLine1[20], 12); // Extract columns 21-32
  tempstr[12] = '\0';
  epochDay = atof(tempstr);

  // Extract Mean Motion (columns 53-63 in Line 2)
  strncpy(tempstr, &tleLine2[52], 10); // Extract columns 53-63
  tempstr[10] = '\0';
  meanMotion = atof(tempstr);

  // Convert TLE Epoch to Unix Time
  struct tm tmEpoch = {};
  tmEpoch.tm_year = year - 1900; // tm_year is years since 1900
  tmEpoch.tm_mday = 1;           // Start of the year
  tmEpoch.tm_hour = 0;
  tmEpoch.tm_min = 0;
  tmEpoch.tm_sec = 0;
  time_t tleEpochStart = mktime(&tmEpoch);                  // Start of the year in Unix time
  time_t tleEpoch = tleEpochStart + (epochDay - 1) * 86400; // Add fractional days as seconds

  // Validate input time
  if (t <= tleEpoch)
  {
    Serial.println("Error: Input time is before or equal to TLE Epoch. Check the time data.");
    return;
  }

  // Time Since TLE Epoch
  unsigned long timeSinceEpoch = t - tleEpoch;

  // Convert Time Since Epoch to Days
  float timeSinceEpochDays = timeSinceEpoch / 86400.0;

  // Calculate Orbits Since TLE Epoch
  float orbitsSinceEpoch = timeSinceEpochDays * meanMotion;

  // Update the Global Orbit Number
  orbitNumber = baselineOrbitNumber + (int)floor(orbitsSinceEpoch);

  // Print the updated Orbit Number
  Serial.print("Updated Orbit Number: ");
  Serial.println(orbitNumber);
}

String formatTime(unsigned long epochTime, bool isLocal = false)
{
  // Adjust the epoch time by adding timezone offset and DST offset if applicable
  time_t t = isLocal ? epochTime + timezoneOffset + dstOffset : epochTime;
  struct tm *tmInfo = gmtime(&t);
  char buffer[20];
  strftime(buffer, 20, "%d.%m.%y @ %H:%M:%S", tmInfo);
  return String(buffer);
}

String formatDate(unsigned long epochTime, bool isLocal = false)
{
  // Adjust the epoch time by adding timezone offset and DST offset if applicable
  time_t t = isLocal ? epochTime + timezoneOffset + dstOffset : epochTime;
  struct tm *tmInfo = gmtime(&t);
  char buffer[10];
  strftime(buffer, 10, "%d.%m.%y", tmInfo);
  return String(buffer);
}

String formatTimeOnly(unsigned long epochTime, bool isLocal = false)
{
  // Adjust the epoch time by adding timezone offset and DST offset if applicable
  time_t t = isLocal ? epochTime + timezoneOffset + dstOffset : epochTime;
  struct tm *tmInfo = gmtime(&t);
  char buffer[10];
  strftime(buffer, 10, "%H:%M:%S", tmInfo);
  return String(buffer);
}

String formatWithSeparator(unsigned long number)
{
  String result = "";
  int count = 0;

  // Process each digit from right to left
  while (number > 0)
  {
    if (count > 0 && count % 3 == 0)
    {
      result = "'" + result; // Add separator
    }
    result = String(number % 10) + result;
    number /= 10;
    count++;
  }

  return result;
}

void calculateNextPass()
{
  Serial.println("Calculating Next Pass...");

  // Reset global variables for the next pass
  nextPassStart = 0;
  nextPassEnd = 0;
  nextPassMaxTCA = 0;
  nextPassCulminationTime = 0;
  nextPassAOSAzimuth = 0;
  nextPassLOSAzimuth = 0;
  culminationAzimuth = 0;
  nextPassPerigee = 0;
  passDuration = 0;
  passMinutes = 0;
  passSeconds = 0;

  passinfo overpass;

  // Initialize prediction with the current time and minimum elevation
  sat.initpredpoint(unixtime, MIN_ELEVATION);

  // Find the next pass using up to 100 iterations
  bool passFound = sat.nextpass(&overpass, 100);

  if (passFound)
  {
    int year, month, day, hour, minute;
    double second;
    bool daylightSaving = false;

    // Adjust time by adding timezoneOffset and dstOffset
    long adjustedTimezoneOffset = timezoneOffset + dstOffset;
    adjustedTimezoneOffset = 0;

    // Convert AOS: Acquisition of Signal
    invjday(overpass.jdstart, adjustedTimezoneOffset / 3600, daylightSaving, year, month, day, hour, minute, second);
    struct tm aosTm = {0};
    aosTm.tm_year = year - 1900;
    aosTm.tm_mon = month - 1;
    aosTm.tm_mday = day;
    aosTm.tm_hour = hour;
    aosTm.tm_min = minute;
    aosTm.tm_sec = (int)second;
    nextPassStart = mktime(&aosTm);

    nextPassAOSAzimuth = overpass.azstart;

    // Convert TCA: Time of Closest Approach
    invjday(overpass.jdmax, adjustedTimezoneOffset / 3600, daylightSaving, year, month, day, hour, minute, second);
    struct tm tcaTm = {0};
    tcaTm.tm_year = year - 1900;
    tcaTm.tm_mon = month - 1;
    tcaTm.tm_mday = day;
    tcaTm.tm_hour = hour;
    tcaTm.tm_min = minute;
    tcaTm.tm_sec = (int)second;
    nextPassCulminationTime = mktime(&tcaTm);

    nextPassMaxTCA = overpass.maxelevation;
    culminationAzimuth = overpass.azmax;

    // Convert LOS: Loss of Signal
    invjday(overpass.jdstop, adjustedTimezoneOffset / 3600, daylightSaving, year, month, day, hour, minute, second);
    struct tm losTm = {0};
    losTm.tm_year = year - 1900;
    losTm.tm_mon = month - 1;
    losTm.tm_mday = day;
    losTm.tm_hour = hour;
    losTm.tm_min = minute;
    losTm.tm_sec = (int)second;
    nextPassEnd = mktime(&losTm);

    nextPassLOSAzimuth = overpass.azstop;

    // Calculate Pass Duration
    passDuration = nextPassEnd - nextPassStart;
    passMinutes = passDuration / 60;
    passSeconds = passDuration % 60;

    // Debug Output UTC
    Serial.println();
    Serial.println("UTC Times");
    Serial.println("Next Pass Details:");
    Serial.printf("AOS: %s @ Azimuth: %.2f°\n", formatTime(nextPassStart, false).c_str(), nextPassAOSAzimuth);
    Serial.printf("TCA (Max Elevation): %s @ Elevation: %.2f°\n", formatTime(nextPassCulminationTime, false).c_str(), nextPassMaxTCA);
    Serial.printf("LOS: %s @ Azimuth: %.2f°\n", formatTime(nextPassEnd, false).c_str(), nextPassLOSAzimuth);
    Serial.printf("Pass Duration: %02lu minutes, %02lu seconds\n", passMinutes, passSeconds);
    Serial.println();
    // Debug Output LOCAL
    Serial.println("Local Times");
    Serial.println("Next Pass Details:");
    Serial.printf("AOS: %s @ Azimuth: %.2f°\n", formatTime(nextPassStart, true).c_str(), nextPassAOSAzimuth);
    Serial.printf("TCA (Max Elevation): %s @ Elevation: %.2f°\n", formatTime(nextPassCulminationTime, true).c_str(), nextPassMaxTCA);
    Serial.printf("LOS: %s @ Azimuth: %.2f°\n", formatTime(nextPassEnd, true).c_str(), nextPassLOSAzimuth);
    Serial.printf("Pass Duration: %02lu minutes, %02lu seconds\n", passMinutes, passSeconds);
    Serial.println();
  }
  else
  {
    Serial.println("No pass found within specified parameters.");
  }
}

void displayAzElPlotPage()
{
  const int numPoints = 100; // Number of points for the plot
  float azimuth[numPoints];
  float elevation[numPoints];
  unsigned long timestamps[numPoints];

  unsigned long startTime = nextPassStart; // Start time (e.g., AOS)
  unsigned long endTime = nextPassEnd;     // End time (e.g., LOS)
  unsigned long timeStep = (endTime - startTime) / numPoints;

  // Calculate azimuth and elevation over time
  for (int i = 0; i < numPoints; i++)
  {
    unsigned long currentTime = startTime + i * timeStep;

    sat.findsat(currentTime); // Update satellite position
    azimuth[i] = sat.satAz;   // Store azimuth
    elevation[i] = sat.satEl; // Store elevation
    timestamps[i] = currentTime;

    // Debug output
    Serial.print("Time: ");
    Serial.print(currentTime);
    Serial.print(" | Azimuth: ");
    Serial.print(azimuth[i]);
    Serial.print("° | Elevation: ");
    Serial.print(elevation[i]);
    Serial.println("°");
  }

  // Plot Az/El graph
  const int PLOT_X = 40;       // Left margin
  const int PLOT_Y = 5;        // Top margin
  const int PLOT_WIDTH = 380;  // Plot width
  const int PLOT_HEIGHT = 240; // Plot height

  const int SCREEN_WIDTH = 480;
  const int SCREEN_HEIGHT = 320;

  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_BLACK);

  // Draw Axes
  tft.drawRect(PLOT_X, PLOT_Y, PLOT_WIDTH, PLOT_HEIGHT, TFT_WHITE);

  // Draw grid and labels
  int azGridInterval = 30; // Azimuth grid every 30 degrees
  int elGridInterval = 15; // Elevation grid every 15 degrees

  // Azimuth (left y-axis)
  for (int az = 0; az <= 360; az += azGridInterval)
  {
    int y = PLOT_Y + PLOT_HEIGHT - map(az, 0, 360, 0, PLOT_HEIGHT);
    tft.drawLine(PLOT_X, y, PLOT_X + PLOT_WIDTH, y, TFT_DARKGREY);
    if (az % 90 == 0)
    { // Label key points (0°, 90°, 180°, 270°, 360°)
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.setCursor(PLOT_X - 30, y - 5);
      tft.printf("%d°", az);
    }
  }

  // Elevation (right y-axis)
  for (int el = 0; el <= 90; el += elGridInterval)
  {
    int y = PLOT_Y + PLOT_HEIGHT - map(el, 0, 90, 0, PLOT_HEIGHT);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(PLOT_X + PLOT_WIDTH + 10, y - 5);
    tft.printf("%d°", el);
  }

  // Vertical Gridlines for Each Minute
  for (unsigned long time = startTime; time <= endTime; time += 60)
  { // Increment by 1 minute (60 seconds)
    int x = PLOT_X + map(time, startTime, endTime, 0, PLOT_WIDTH);
    tft.drawLine(x, PLOT_Y, x, PLOT_Y + PLOT_HEIGHT, TFT_DARKGREY); // Draw vertical gridline
  }

  // Time (x-axis)
  for (int i = 0; i <= 5; i++)
  {
    int x = PLOT_X + map(i, 0, 5, 0, PLOT_WIDTH);
    unsigned long time = startTime + i * (endTime - startTime) / 5;

    // Convert time to HH:MM format
    time_t t = time;
    struct tm *timeInfo = gmtime(&t); // Use UTC for time conversion
    char buffer[6];                   // Buffer for "HH:MM"
    snprintf(buffer, sizeof(buffer), "%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(x - 15, PLOT_Y + PLOT_HEIGHT + 5);
    tft.print(buffer); // Print HH:MM
  }

  // Plot Azimuth and Elevation
  int lastAzX = -1, lastAzY = -1;
  int lastElX = -1, lastElY = -1;

  for (int i = 0; i < numPoints; i++)
  {
    int x = PLOT_X + map(timestamps[i], startTime, endTime, 0, PLOT_WIDTH); // Correct scaling
    int azY = PLOT_Y + PLOT_HEIGHT - map(azimuth[i], 0, 360, 0, PLOT_HEIGHT);
    int elY = PLOT_Y + PLOT_HEIGHT - map(elevation[i], 0, 90, 0, PLOT_HEIGHT);

    // Handle Azimuth Wraparound
    if (lastAzX != -1 && lastAzY != -1)
    {
      if (abs(azimuth[i] - azimuth[i - 1]) > 180)
      {
        if (azimuth[i] > azimuth[i - 1])
        {
          tft.drawLine(lastAzX, lastAzY, x, PLOT_Y + PLOT_HEIGHT - map(0, 0, 360, 0, PLOT_HEIGHT), TFT_GREENYELLOW);
          tft.drawLine(x, PLOT_Y + PLOT_HEIGHT - map(360, 0, 360, 0, PLOT_HEIGHT), x, azY, TFT_GREENYELLOW);
        }
        else
        {
          tft.drawLine(lastAzX, lastAzY, x, PLOT_Y + PLOT_HEIGHT - map(360, 0, 360, 0, PLOT_HEIGHT), TFT_GREENYELLOW);
          tft.drawLine(x, PLOT_Y + PLOT_HEIGHT - map(0, 0, 360, 0, PLOT_HEIGHT), x, azY, TFT_GREENYELLOW);
        }
      }
      else
      {
        tft.drawLine(lastAzX, lastAzY, x, azY, TFT_GREENYELLOW);
      }
    }
    lastAzX = x;
    lastAzY = azY;

    // Draw Elevation Line (no wraparound needed)
    if (lastElX != -1 && lastElY != -1)
    {
      tft.drawLine(lastElX, lastElY, x, elY, TFT_CYAN);
    }
    lastElX = x;
    lastElY = elY;

    // Optional: Fill under elevation curve
    tft.drawLine(x, elY, x, PLOT_Y + PLOT_HEIGHT, TFT_CYAN);
  }

  // Display extra pass details
  tft.setFreeFont(&FreeMono9pt7b);
  tft.setTextFont(4);
  tft.setCursor(45, 270); // Position for additional info below the plot
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.print("AOS: ");
  tft.print(formatTimeOnly(nextPassStart, true));//xxx
  tft.print(" | LOS: ");
  tft.println(formatTimeOnly(nextPassEnd, true));
    tft.setCursor(90, 296); // Position for additional info below the plot

  tft.print("Pass Duration: ");
  unsigned long duration = nextPassEnd - nextPassStart;
  tft.print(duration / 60);
  tft.print("m ");
  tft.print(duration % 60);
  tft.println("s");
}



void displayPolarPlotPage()
{
  calculateNextPass();
  getOrbitNumber(nextPassStart);
  // Clear the area to redraw
  tft.fillRect(0, 0, 480, 320, TFT_BLACK);
  // Display AOS on TFT screen
  int margin = 3;
  int newline = 8;
  tft.setCursor(margin, 10);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.setTextFont(4);                            // Set the desired font
  tft.print("ISS Orbit ");                       // Label for Orbit number
  tft.println(formatWithSeparator(orbitNumber)); // Label for Orbit number
  tft.setCursor(margin, tft.getCursorY() + 5);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  tft.print("AOS  "); // Label for AOS

  tft.setTextFont(2);                           // Set the desired font
  tft.println(formatDate(nextPassStart, true)); // Prints just the date
  tft.setCursor(margin, tft.getCursorY() + 8);  // Move to next line at x=5

  tft.setTextFont(4);                               // Set the desired font
  tft.setCursor(margin, tft.getCursorY());          // Move to next line at x=5
  tft.println(formatTimeOnly(nextPassStart, true)); // Prints just the time

  tft.setCursor(margin, tft.getCursorY());          // Move to next line at x=5
  int azimuthInt = (int)(nextPassAOSAzimuth + 0.5); // Rounds to nearest integer
  tft.print(azimuthInt);
  tft.println(" deg.");
  tft.setCursor(10, tft.getCursorY() + newline);

  Serial.print("Azimuth at AOS: ");
  Serial.print(nextPassAOSAzimuth);
  Serial.println("°");

  Serial.print("Max Elevation TCA: ");
  Serial.print(nextPassMaxTCA);
  Serial.println("° at time ");
  Serial.println(formatTime(nextPassCulminationTime));
  Serial.print("Azimuth at max elevation:");
  Serial.print(culminationAzimuth);
  Serial.println("°");

  // Display TCA
  tft.setTextFont(4); // Set the desired font
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(margin, tft.getCursorY()); // Move to next line at x=5

  tft.print("TCA "); // Label for TCA

  int maxel = (int)(nextPassMaxTCA + 0.5);
  tft.print(maxel);
  tft.println(" deg.");
  // Set the desired font
  tft.setCursor(margin, tft.getCursorY());                    // Move to next line at x=5
  tft.println(formatTimeOnly(nextPassCulminationTime, true)); // Prints just the time

  tft.setCursor(margin, tft.getCursorY());      // Move to next line at x=5
  azimuthInt = (int)(culminationAzimuth + 0.5); // Rounds to nearest integer
  tft.print(azimuthInt);
  tft.println(" deg.");
  tft.setCursor(10, tft.getCursorY() + newline);

  // Display LOS
  tft.setTextFont(4); // Set the desired font
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(margin, tft.getCursorY()); // Move to next line at x=5

  tft.println("LOS"); // Label for TCA

  tft.setTextFont(2);                             // Set the desired font
  tft.setCursor(margin, tft.getCursorY());        // Move to next line at x=5
  tft.setTextFont(4);                             // Set the desired font
  tft.setCursor(margin, tft.getCursorY());        // Move to next line at x=5
  tft.println(formatTimeOnly(nextPassEnd, true)); // Prints just the time

  tft.setCursor(margin, tft.getCursorY());      // Move to next line at x=5
  azimuthInt = (int)(nextPassLOSAzimuth + 0.5); // Rounds to nearest integer
  tft.print(azimuthInt);
  tft.println(" deg.");
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.setCursor(margin, tft.getCursorY() + newline);
  tft.print("Pass Duration: ");
  unsigned long duration = nextPassEnd - nextPassStart;

  tft.print(duration / 60);
  tft.print(":");

  tft.print(duration % 60);

  Serial.print("Pass Duration: ");
  Serial.print(duration / 60); // minutes
  Serial.print(" min ");
  Serial.print(duration % 60); // seconds
  Serial.println(" sec");

  Serial.print("LOS (End): ");
  Serial.println(formatTime(nextPassEnd));
  Serial.print("Azimuth at LOS: ");
  Serial.print(nextPassLOSAzimuth);
  Serial.println("°");

#define POLAR_CENTER_X 320 // Center of the polar chart
#define POLAR_CENTER_Y 160 // Center of the polar chart
#define POLAR_RADIUS 140   // Adjusted radius for more space

  // Draw concentric circles for elevation markers
  for (int i = 75; i >= 15; i -= 15) // Start from 75 and go down to 15
  {
    int radius = map(i, 0, 75, 0, POLAR_RADIUS);
    tft.drawCircle(POLAR_CENTER_X, POLAR_CENTER_Y, radius, TFT_LIGHTGREY);

    // Label elevation markers inside each circle
    int x = POLAR_CENTER_X + radius + 5; // Offset to place labels inside each circle
    int y = POLAR_CENTER_Y - 10;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    tft.setCursor(x, y);
    tft.print(i);
  }

  // Add degree labels around the outer circle at 30° intervals, with 0° at North
  for (int angle = 0; angle < 360; angle += 30)
  {
    if (angle == 0 || angle == 90 || angle == 180 || angle == 270)
      continue;

    float radianAngle = radians(angle); // Angle adjusted to start 0° at North
    int x = POLAR_CENTER_X + (POLAR_RADIUS + 10) * sin(radianAngle);
    int y = POLAR_CENTER_Y - (POLAR_RADIUS + 10) * cos(radianAngle);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    tft.setCursor(x - 5, y - 5);
    tft.print(angle);
  }

  // Draw radial lines every 30° for compass directions
  for (int angle = 0; angle < 360; angle += 30)
  {
    float radianAngle = radians(angle); // Corrected orientation to match 0° at North
    int xEnd = POLAR_CENTER_X + POLAR_RADIUS * sin(radianAngle);
    int yEnd = POLAR_CENTER_Y - POLAR_RADIUS * cos(radianAngle);
    tft.drawLine(POLAR_CENTER_X, POLAR_CENTER_Y, xEnd, yEnd, TFT_DARKGREY);
  }

  // Draw compass labels with correct orientation
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("N", POLAR_CENTER_X, POLAR_CENTER_Y - POLAR_RADIUS - 18, 2);
  tft.drawCentreString("S", POLAR_CENTER_X, POLAR_CENTER_Y + POLAR_RADIUS + 5, 2);
  tft.drawCentreString("E", POLAR_CENTER_X + POLAR_RADIUS + 13, POLAR_CENTER_Y, 2);
  tft.drawCentreString("W", POLAR_CENTER_X - POLAR_RADIUS - 10, POLAR_CENTER_Y, 2);

  // Plot the satellite pass path with color dots for AOS, max elevation, and LOS
  int lastX = -1, lastY = -1;
  // Time step for pass prediction
  int timeStep = 2;
  for (unsigned long t = nextPassStart; t <= nextPassEnd; t += timeStep)
  {
    sat.findsat(t);
    float azimuth = sat.satAz;
    float elevation = sat.satEl;

    // Serial.printf("Time: %lu | Azimuth: %.2f | Elevation: %.2f\n", t, azimuth, elevation);

    if (elevation >= 0)
    {
      int radius = map(90 - elevation, 0, 90, 0, POLAR_RADIUS);
      float radianAzimuth = radians(azimuth);
      int x = POLAR_CENTER_X + radius * sin(radianAzimuth);
      int y = POLAR_CENTER_Y - radius * cos(radianAzimuth);

      // Serial.printf("Plotted Point -> x: %d, y: %d\n", x, y);

      if (t == nextPassStart)
      {
        tft.fillCircle(x, y, 3, TFT_GREEN); // Green dot for AOS
        // Serial.println("Plotted AOS (Green)");
      }
      else if (t == nextPassCulminationTime)
      {
        tft.fillCircle(x, y, 3, TFT_YELLOW); // Yellow dot for max elevation
        // Serial.println("Plotted TCA (Yellow)");
      }
      else if (t == nextPassEnd)
      {
        tft.fillCircle(x, y, 3, TFT_RED); // Red dot for LOS
        // Serial.println("Plotted LOS (Red)");
      }

      if (lastX != -1 && lastY != -1)
      {
        tft.drawLine(lastX, lastY, x, y, TFT_GOLD); // Path
        // Serial.println("Drew Line Segment");
      }
      lastX = x;
      lastY = y;
    }
  }
}

/*
void calculateNextPassOlMethodNotUsed()
{
  Serial.println("Calculating Next Pass...");

  // Reset global variables for the next pass
  nextPassStart = 0;
  nextPassEnd = 0;
  nextPassMaxTCA = 0;
  nextPassCulminationTime = 0;
  nextPassAOSAzimuth = 0;
  nextPassLOSAzimuth = 0;
  culminationAzimuth = 0;
  nextPassPerigee = 0;
  passDuration = 0;
  passMinutes = 0;
  passSeconds = 0;

  bool passFound = false;

  while (!passFound)
  {
    bool passInProgress = false;

    // Check if the satellite is already above the horizon at the current time
    sat.findsat(unixtime);
    if (sat.satEl > MIN_ELEVATION)
    {
      nextPassStart = unixtime;
      nextPassAOSAzimuth = sat.satAz;
      passInProgress = true;
    }

    // Iterate through the time to find the next pass
    for (unsigned long t = unixtime + timeStep; t < unixtime + 86400; t += timeStep)
    {
      sat.findsat(t);

      if (sat.satEl > MIN_ELEVATION)
      {
        if (!passInProgress)
        {
          passInProgress = true;
          nextPassStart = t;
          nextPassAOSAzimuth = sat.satAz;
          nextPassMaxTCA = sat.satEl;
          nextPassCulminationTime = t;
          nextPassPerigee = sat.satDist;
        }
        else
        {
          if (sat.satEl > nextPassMaxTCA)
          {
            nextPassMaxTCA = sat.satEl;
            nextPassCulminationTime = t;
            culminationAzimuth = sat.satAz; // Update culmination azimuth
          }
          if (sat.satDist < nextPassPerigee)
          {
            nextPassPerigee = sat.satDist;
          }
        }
        nextPassEnd = t;
        nextPassLOSAzimuth = sat.satAz;
      }
      else if (passInProgress)
      {
        // Pass has ended
        break;
      }
    }

    // Check if the pass meets the minimum elevation requirement
    if (nextPassMaxTCA >= MIN_ELEVATION)
    {
      passFound = true;
    }
    else
    {
      Serial.println("Ignored pass due to low elevation.");
      unixtime = nextPassEnd + timeStep; // Skip to after the current pass
    }
  }

  // Calculate pass duration in minutes and seconds
  passDuration = nextPassEnd - nextPassStart;
  passMinutes = passDuration / 60;
  passSeconds = passDuration % 60;

  // Print pass details to Serial
  Serial.println("Next Pass Details:");
  Serial.println("------------------");
  Serial.println("AOS: " + formatTime(nextPassStart, true) + " @ Azimuth: " + String(nextPassAOSAzimuth, 2) + "°");
  Serial.println("TCA: " + formatTime(nextPassCulminationTime, true) + " @ Max Elevation: " + String(nextPassMaxTCA, 2) + "°");
  Serial.println("LOS: " + formatTime(nextPassEnd, true) + " @ Azimuth: " + String(nextPassLOSAzimuth, 2) + "°");
  Serial.println("Pass Duration: " + String(passMinutes) + " minutes " + String(passSeconds) + " seconds");
  Serial.println("------------------");
}
*/

void updateBigClock(bool refresh = false)
{
  int y = 0;
  int color = TFT_GOLD;
  tft.setTextFont(8);
  tft.setTextSize(1);
  static String previousTime = ""; // Track previous time to update only changed characters

  static bool isPositionCalculated = false;
  static int clockXPosition; // Calculated once to center the clock text
  static int clockWidth;     // Width of the time string in pixels

  // If refresh is true, reset the static variables to their initial state
  if (refresh)
  {
    previousTime = "";
    isPositionCalculated = false;
  }

  // Perform initial calculation of clock width and position if not already done
  if (!isPositionCalculated)
  {
    String sampleTime = "23:59:59"; // Sample time format for clock width calculation
    clockWidth = tft.textWidth(sampleTime.c_str());
    clockXPosition = (tft.width() - clockWidth) / 2; // Center x position for the clock
    isPositionCalculated = true;                     // Mark as calculated
  }

  // Get current UTC time in seconds
  unsigned long utcTime = timeClient.getEpochTime();

  // Apply timezone and DST offsets
  unsigned long localTime = utcTime + timezoneOffset + dstOffset;

  // Convert to human-readable format
  struct tm *timeinfo = gmtime((time_t *)&localTime); // Use gmtime for seconds since epoch
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

  Serial.print("Local Time: ");
  Serial.println(timeStr);

  String currentTime = String(timeStr);

  // Only update characters that have changed
  int xPosition = clockXPosition; // Start at the pre-calculated center position
  for (int i = 0; i < currentTime.length(); i++)
  {
    // If character has changed, update it
    if (i >= previousTime.length() || currentTime[i] != previousTime[i])
    {
      // Clear the previous character area by printing a black background
      tft.setCursor(xPosition, y);
      tft.setTextColor(TFT_BLACK, TFT_BLACK); // Black on black to clear
      tft.print(previousTime[i]);

      // Print the new character with the specified color
      tft.setCursor(xPosition, y);
      tft.setTextColor(color, TFT_BLACK); // Color on black background
      tft.print(currentTime[i]);
    }
    // Increment xPosition by width of the current character
    xPosition += tft.textWidth(String(currentTime[i]).c_str());
  }

  // Update previousTime to the new time
  previousTime = currentTime;
}

void displayElevation(float number, int x, int y, uint16_t color, bool refresh)
{
  tft.setTextSize(1);
  tft.setTextFont(7);

  static char previousOutput[6] = "     ";   // Previous state (5 characters + null terminator) 999.9
  static uint16_t previousColor = TFT_GREEN; // Track the last color used
  static bool isInitiated = false;           // Track initialization of static elements
  char output[6] = "     ";                  // Current output (5 characters + null terminator) 999.9

  // Format the number with sign and one decimal place (e.g., "+123.4" or "-123.4")
  char tempBuffer[7];                                       // Temporary buffer to hold the formatted string
  snprintf(tempBuffer, sizeof(tempBuffer), "%.1f", number); // Format number without the sign

  // Dynamically fill the `output` array from the right
  int len = strlen(tempBuffer); // Length of the formatted number
  int startIndex = 5 - len;     // Calculate starting index for right alignment
  for (int i = 0; i < len; i++)
  {
    output[startIndex + i] = tempBuffer[i]; // Copy characters from tempBuffer to output
  }

  // Positions for characters
  int xPos[] = {0, 32, 64, 96, 117, 124}; // Adjust character width dynamically
  for (int i = 0; i < 6; i++)
    xPos[i] += x;

  // Handle color change or refresh logic
  if (color != previousColor)
  {
    // If the color has changed, trigger a full redraw
    refresh = true;
    previousColor = color; // Update the last used color
  }

  // Handle refresh logic for static elements (decimal point)
  if (!isInitiated || refresh)
  {

    // Margins around the text
    int horiMargin = 12;
    int topMargin = 14;
    int bottomMargin = 22;
    // Calculate text dimensions
    int textLength = xPos[5] - x + tft.textWidth("8") + 14;
    ;
    // Total width and height of the rectangle
    int rectWidth = textLength + 2 * horiMargin;
    int rectHeight = tft.fontHeight() + topMargin + bottomMargin;
    // X and Y positions for the rectangle
    int rectX = xPos[0] - horiMargin;
    int rectY = y - topMargin;
    // Draw the rectangle with rounded corners
    tft.drawRoundRect(rectX, rectY, rectWidth, rectHeight, 10, TFT_LIGHTGREY);
    // Draw Elevation label
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.drawString("Elevation", rectX + rectWidth / 2 - tft.textWidth("Elevation") / 2, rectY + rectHeight - 12); // Decimal point is fixed at xPos[4]

    tft.setTextFont(7);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(".", xPos[4], y); // Decimal point is fixed at xPos[4]
    tft.setFreeFont(&FreeMonoBold12pt7b);
    tft.drawString("o", xPos[4] + 40, y - 5); // Decimal point is fixed at xPos[4]
    tft.setTextFont(7);

    isInitiated = true;
  }

  // Update only changed characters or redraw all if refresh is triggered
  for (int i = 0; i < 6; i++)
  { // Loop through each character of the formatted number
    if (output[i] != previousOutput[i] || refresh)
    {
      // Print the previous character in black to erase it
      if (previousOutput[i] != ' ')
      {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawChar(previousOutput[i], xPos[i], y);
      }

      // Draw the new character
      if (output[i] != ' ')
      {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawChar(output[i], xPos[i], y);
      }

      // Update previous state
      previousOutput[i] = output[i];
    }
  }
}

void displayAzimuth(float number, int x, int y, uint16_t color, bool refresh)
{
  tft.setTextSize(1);
  tft.setTextFont(7);

  static char previousOutput[6] = "     ";   // Previous state (5 characters + null terminator) 999.9
  static uint16_t previousColor = TFT_GREEN; // Track the last color used
  static bool isInitiated = false;           // Track initialization of static elements
  char output[6] = "     ";                  // Current output (5 characters + null terminator) 999.9

  // Format the number with sign and one decimal place (e.g., "+123.4" or "-123.4")
  char tempBuffer[7];                                       // Temporary buffer to hold the formatted string
  snprintf(tempBuffer, sizeof(tempBuffer), "%.1f", number); // Format number without the sign

  // Dynamically fill the `output` array from the right
  int len = strlen(tempBuffer); // Length of the formatted number
  int startIndex = 5 - len;     // Calculate starting index for right alignment
  for (int i = 0; i < len; i++)
  {
    output[startIndex + i] = tempBuffer[i]; // Copy characters from tempBuffer to output
  }

  // Positions for characters
  int xPos[] = {0, 32, 64, 96, 117, 124}; // Adjust character width dynamically
  for (int i = 0; i < 6; i++)
    xPos[i] += x;

  // Handle color change or refresh logic
  if (color != previousColor)
  {
    // If the color has changed, trigger a full redraw
    refresh = true;
    previousColor = color; // Update the last used color
  }

  // Handle refresh logic for static elements (decimal point)
  if (!isInitiated || refresh)
  {

    // Margins around the text
    int horiMargin = 12;
    int topMargin = 14;
    int bottomMargin = 22;
    // Calculate text dimensions
    int textLength = xPos[5] - x + tft.textWidth("8") + 14;
    ;
    // Total width and height of the rectangle
    int rectWidth = textLength + 2 * horiMargin;
    int rectHeight = tft.fontHeight() + topMargin + bottomMargin;
    // X and Y positions for the rectangle
    int rectX = xPos[0] - horiMargin;
    int rectY = y - topMargin;
    // Draw the rectangle with rounded corners
    tft.drawRoundRect(rectX, rectY, rectWidth, rectHeight, 10, TFT_LIGHTGREY);
    // Draw Azimuth label
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.drawString("Azimuth", rectX + rectWidth / 2 - tft.textWidth("Azimuth") / 2, rectY + rectHeight - 12); // Decimal point is fixed at xPos[4]

    tft.setTextFont(7);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(".", xPos[4], y); // Decimal point is fixed at xPos[4]
    tft.setFreeFont(&FreeMonoBold12pt7b);
    tft.drawString("o", xPos[4] + 40, y - 5); // Decimal point is fixed at xPos[4]
    tft.setTextFont(7);

    isInitiated = true;
  }

  // Update only changed characters or redraw all if refresh is triggered
  for (int i = 0; i < 6; i++)
  { // Loop through each character of the formatted number
    if (output[i] != previousOutput[i] || refresh)
    {
      // Print the previous character in black to erase it
      if (previousOutput[i] != ' ')
      {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawChar(previousOutput[i], xPos[i], y);
      }

      // Draw the new character
      if (output[i] != ' ')
      {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawChar(output[i], xPos[i], y);
      }

      // Update previous state
      previousOutput[i] = output[i];
    }
  }
}
/*
void displayLatLon(int x, int y, float val)
{
  static float prevVal = -1.0;
  static int digitPositions[5]; // Store x-positions of each digit for "359.9" format
  static int decimalPos;
  static bool isInitialized = false;

  // Initialize digit positions, decimal point position, and draw rectangle and label once
  tft.setTextFont(4);
  tft.setTextSize(1);

  if (!isInitialized)
  {
    String sample = "-90.0";
    tft.setTextColor(TFT_BLACK, TFT_BLACK); // Print in black to capture digit positions
    tft.setCursor(x, y);

    // Capture x-coordinates for each character in the sample, with special handling for the decimal point
    for (int i = 0; i < sample.length(); i++)
    {
      digitPositions[i] = tft.getCursorX(); // Capture position for each character
      if (sample[i] == '.')
      {
        decimalPos = digitPositions[i]; // Save the specific position for the decimal point
      }
      tft.print(sample[i]);

      // Draw the decimal point once at its fixed position
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(decimalPos, y);
      tft.print(".");
      isInitialized = true;
    }

    // Determine the color based on elevation value

    int color = TFT_GOLD;

    // Format val with exactly one decimal place
    char valStr[8];
    sprintf(valStr, "%5.1f", val);
    char prevValStr[8];
    sprintf(prevValStr, "%5.1f", prevVal);

    // Update only digits that have changed, ensuring minus sign changes color too
    for (int i = 0; i < 5; i++)
    {
      if (valStr[i] != prevValStr[i] || (valStr[i] == '-' && color != TFT_RED))
      {
        tft.setCursor(digitPositions[i], y);
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.print(prevValStr[i]);

        tft.setCursor(digitPositions[i], y);
        tft.setTextColor(color, TFT_BLACK);
        tft.print(valStr[i]);
      }
    }

    // Update decimal point color to match digits
    tft.setCursor(decimalPos, y);
    tft.setTextColor(color, TFT_BLACK);
    tft.print(".");

    // Update prevVal to the current value
    prevVal = val;
  }
}
*/
void displayLatitude(float number, int x, int y, uint16_t color, bool refresh)
{
  tft.setTextSize(1);
  tft.setTextFont(4);

  static char previousOutput[7] = "      ";  // Previous state (6 characters + null terminator)
  static uint16_t previousColor = TFT_GREEN; // Track the last color used
  static bool isInitiated = false;           // Track initialization of static elements
  char output[7] = "      ";                 // Current output (6 characters + null terminator)

  // Format the number with sign and one decimal place (e.g., "+123.4" or "-123.4")
  char tempBuffer[8];                                        // Temporary buffer to hold the formatted string
  snprintf(tempBuffer, sizeof(tempBuffer), "%+.1f", number); // Format number

  // Dynamically fill the `output` array from the right
  int len = strlen(tempBuffer); // Length of the formatted number
  int startIndex = 6 - len;     // Calculate starting index for right alignment
  for (int i = 0; i < len; i++)
  {
    output[startIndex + i] = tempBuffer[i]; // Copy characters from tempBuffer to output
  }

  // Positions for characters
  int xPos[] = {0, 14, 28, 42, 56, 63};

  for (int i = 0; i < 6; i++)
    xPos[i] += x;

  // Handle color change or refresh logic
  if (color != previousColor)
  {
    // If the color has changed, trigger a full redraw
    refresh = true;
    previousColor = color; // Update the last used color
  }

  // Handle refresh logic for static elements (decimal point)
  if (!isInitiated || refresh)
  {
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(".", xPos[4], y);         // Decimal point is fixed at xPos[4]
    tft.drawString("deg.", xPos[5] + 20, y); // Unit
    tft.drawString("Lat.", xPos[0] - 45, y); // Unit

    isInitiated = true;
  }

  // Update only changed characters or redraw all if refresh is triggered
  for (int i = 0; i < 6; i++)
  { // Loop through each character of the formatted number
    if (output[i] != previousOutput[i] || refresh)
    {
      // Print the previous character in black to erase it
      if (previousOutput[i] != ' ')
      {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawChar(previousOutput[i], xPos[i], y);
      }

      // Draw the new character
      if (output[i] != ' ')
      {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawChar(output[i], xPos[i], y);
      }

      // Update previous state
      previousOutput[i] = output[i];
    }
  }
}

void displayLongitude(float number, int x, int y, uint16_t color, bool refresh)
{
  tft.setTextSize(1);
  tft.setTextFont(4);

  static char previousOutput[7] = "      ";  // Previous state (6 characters + null terminator)
  static uint16_t previousColor = TFT_GREEN; // Track the last color used
  static bool isInitiated = false;           // Track initialization of static elements
  char output[7] = "      ";                 // Current output (6 characters + null terminator)

  // Format the number with sign and one decimal place (e.g., "+123.4" or "-123.4")
  char tempBuffer[8];                                        // Temporary buffer to hold the formatted string
  snprintf(tempBuffer, sizeof(tempBuffer), "%+.1f", number); // Format number

  // Dynamically fill the `output` array from the right
  int len = strlen(tempBuffer); // Length of the formatted number
  int startIndex = 6 - len;     // Calculate starting index for right alignment
  for (int i = 0; i < len; i++)
  {
    output[startIndex + i] = tempBuffer[i]; // Copy characters from tempBuffer to output
  }

  // Positions for characters
  int xPos[] = {0, 14, 28, 42, 56, 63};

  for (int i = 0; i < 6; i++)
    xPos[i] += x;

  // Handle color change or refresh logic
  if (color != previousColor)
  {
    // If the color has changed, trigger a full redraw
    refresh = true;
    previousColor = color; // Update the last used color
  }

  // Handle refresh logic for static elements (decimal point)
  if (!isInitiated || refresh)
  {
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(".", xPos[4], y);         // Decimal point is fixed at xPos[4]
    tft.drawString("deg.", xPos[5] + 20, y); // Unit
    tft.drawString("Lon.", xPos[0] - 45, y); // Unit
    isInitiated = true;
  }

  // Update only changed characters or redraw all if refresh is triggered
  for (int i = 0; i < 6; i++)
  { // Loop through each character of the formatted number
    if (output[i] != previousOutput[i] || refresh)
    {
      // Print the previous character in black to erase it
      if (previousOutput[i] != ' ')
      {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawChar(previousOutput[i], xPos[i], y);
      }

      // Draw the new character
      if (output[i] != ' ')
      {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawChar(output[i], xPos[i], y);
      }

      // Update previous state
      previousOutput[i] = output[i];
    }
  }
}

void displayAltitude()
{
  // Static variable to store the previous altitude, initialized only once
  static long previousAlt = -1;

  // Only update if the altitude has changed
  if (sat.satAlt != previousAlt)
  {
    // Set cursor for the label text
    tft.setCursor(20, 250);
    tft.setTextColor(TFT_GOLD);
    tft.setTextFont(4);

    // Draw "Altitude:" label
    tft.print("Altitude: ");

    // Clear the previous altitude value by filling the specific area with black
    tft.fillRect(130, 250, 80, 30, TFT_BLACK); // Adjust width and height based on text area

    // Set cursor for the altitude value and print updated altitude
    tft.setCursor(130, 250);
    tft.print(formatWithSeparator(sat.satAlt));

    // Draw the "km" label after the altitude value
    tft.print(" km");

    // Update previousAlt to the current altitude
    previousAlt = sat.satAlt;
  }
}

void displayLTLEage(int y, bool refresh)
{
  static bool isInitiated = false;

  if (!isInitiated || refresh)
  {
    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.setTextColor(TFT_GOLD);
    tft.setCursor(276, y);
    tft.print("TLE age:");
    tft.setCursor(388, y);
    tft.print(String(TLE_age));
    isInitiated = true;
  }
}

void pngDraw(PNGDRAW *pDraw)
{
  uint16_t lineBuffer[480];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(0, 0 + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}
void displayISSimage(int duration)
{
  // https://notisrac.github.io/FileToCArray/
  int16_t rc = png.openFLASH((uint8_t *)ISSsplashImage, sizeof(ISSsplashImage), pngDraw);

  if (rc == PNG_SUCCESS)
  {
    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt);
    Serial.println("ms");
    tft.endWrite();
  }

  delay(duration);
}
void displayEquirectangularWorlsMap()
{
  // https://notisrac.github.io/FileToCArray/
  int16_t rc = png.openFLASH((uint8_t *)worldMap, sizeof(worldMap), pngDraw);
  if (rc == PNG_SUCCESS)
  {
    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt);
    Serial.println("ms");
    tft.endWrite();
  }

  String text = "ISS (ZARYA) Next 3 Passes";

  // Set the text font to FONT4
  tft.setTextFont(4);

  // Calculate the width of the text to center it
  int textWidth = tft.textWidth(text);

  // Calculate the x position to center the text on the screen
  int xPosition = (tft.width() - textWidth) / 2;

  // Set the cursor position at the center (x) and y = 5
  tft.setCursor(xPosition, 5);

  // Set text color (adjust to your preference)
  tft.setTextColor(TFT_GOLD, TFT_TRANSPARENT); // White text on black background

  // Print the text
  tft.print(text);

  int yPassColor = 30;
  text = "Green";

  // Set the text font to FONT4
  tft.setTextFont(2);

  // Calculate the width of the text to center it
  textWidth = tft.textWidth(text);

  // Calculate the x position to center the text on the screen
  xPosition = (tft.width() - textWidth) / 2 - 50;

  // Set the cursor position at the center (x) and y = 5
  tft.setCursor(xPosition, yPassColor);

  // Set text color (adjust to your preference)
  tft.setTextColor(TFT_GREEN, TFT_TRANSPARENT); // White text on black background

  // Print the text
  tft.print(text);

  text = "- Yellow -";

  // Set the text font to FONT4
  tft.setTextFont(2);

  // Calculate the width of the text to center it
  textWidth = tft.textWidth(text);

  // Calculate the x position to center the text on the screen
  xPosition = (tft.width() - textWidth) / 2;

  // Set the cursor position at the center (x) and y = 5
  tft.setCursor(xPosition, yPassColor);

  // Set text color (adjust to your preference)
  tft.setTextColor(TFT_GOLD, TFT_TRANSPARENT); // White text on black background

  // Print the text
  tft.print(text);

  text = "Red";

  // Set the text font to FONT4
  tft.setTextFont(2);

  // Calculate the width of the text to center it
  textWidth = tft.textWidth(text);

  // Calculate the x position to center the text on the screen
  xPosition = (tft.width() - textWidth) / 2 + 50;

  // Set the cursor position at the center (x) and y = 5
  tft.setCursor(xPosition, yPassColor);

  // Set text color (adjust to your preference)
  tft.setTextColor(TFT_RED, TFT_TRANSPARENT); // White text on black background

  // Print the text
  tft.print(text);
}

/*
void displayBlueMapimage()
{
  // https://notisrac.github.io/FileToCArray/
  int16_t rc = png.openFLASH((uint8_t *)blueMap, sizeof(blueMap), pngDraw);
  if (rc == PNG_SUCCESS)
  {
    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt);
    Serial.println("ms");
    tft.endWrite();
  }
}
*/

void displayPatreonimage()
{
  // https://notisrac.github.io/FileToCArray/
  int16_t rc = png.openFLASH((uint8_t *)patreon, sizeof(patreon), pngDraw);
  if (rc == PNG_SUCCESS)
  {
    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt);
    Serial.println("ms");
    tft.endWrite();
  }
}

void displayPExpedition72image()
{
  // https://notisrac.github.io/FileToCArray/
  int16_t rc = png.openFLASH((uint8_t *)expedition72, sizeof(expedition72), pngDraw);
  if (rc == PNG_SUCCESS)
  {
    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt);
    Serial.println("ms");
    tft.endWrite();
  }
}

void drawFootprint(int centerX, int centerY, float altitude)
{
  float earthRadius = 6371.0; // Earth's radius in kilometers
  float footprintRadiusKm = earthRadius * acos(earthRadius / (earthRadius + altitude));

  // Debug: Print calculated footprint radius in km
  Serial.print("Footprint radius (km): ");
  Serial.println(footprintRadiusKm);

  // Map scaling factors
  float pixelsPerDegreeLat = 320.0 / 180.0; // 320 pixels for 180 degrees latitude (-90 to +90)
  float pixelsPerDegreeLon = 480.0 / 360.0; // 480 pixels for 360 degrees longitude (-180 to +180)

  // Convert footprint radius in kilometers to map pixels
  // Assume 1° latitude ≈ 111 km and scale accordingly
  float degreesLat = footprintRadiusKm / 111.0;           // Approximate conversion to degrees
  float degreesLon = degreesLat;                          // Same for longitude assuming spherical geometry
  int footprintRadiusY = degreesLat * pixelsPerDegreeLat; // Convert degrees to pixels
  int footprintRadiusX = degreesLon * pixelsPerDegreeLon; // Convert degrees to pixels

  /*Debug: Print scaled footprint radii in pixels
  Serial.print("Footprint radius X (pixels): ");
  Serial.println(footprintRadiusX);
  Serial.print("Footprint radius Y (pixels): ");
  Serial.println(footprintRadiusY);
  */

  // Draw an ellipse to represent the footprint
  for (int angle = 0; angle < 360; angle++)
  {
    float rad = angle * DEG_TO_RAD; // Convert to radians
    int x = centerX + cos(rad) * footprintRadiusX;
    int y = centerY + sin(rad) * footprintRadiusY;

    // Check bounds before drawing
    if (x >= 0 && x < 480 && y >= 0 && y < 320)
    {
      tft.fillCircle(x, y + 40, 1, TFT_GOLD); // Radius of 1 pixel makes a 2-pixel diameter dot
    }
  }
}

void displayMapWithMultiPasses()
{
  const int timeStep = 15; // Time step for plotting points
  int startX;
  int currentXpos;
  int currentYpos;

  tft.fillScreen(TFT_BLACK);
  displayEquirectangularWorlsMap();

  // Get the starting position
  sat.findsat(unixtime);
  float startLon = sat.satLon;
  startX = map(startLon, -180, 180, 0, 480); // Calculate the starting x position based on longitude
  currentXpos = startX;
  currentYpos = map(sat.satLat, 90, -90, 0, 242);

  unsigned long t = unixtime;
  bool hasLeftStartX = false;
  int passageCount = 0; // Track the number of passages across startX

  // Plot points until three passages are completed
  while (passageCount < 3)
  {
    sat.findsat(t);
    float lat = sat.satLat;
    float lon = sat.satLon;

    // Convert latitude and longitude to map coordinates
    int x = map(lon, -180, 180, 0, 480); // Longitude to x (0 to 480)
    int y = map(lat, 90, -90, 0, 242);   // Latitude to y (0 to 242)
    y = y + 40;                          // to bypass black banner on top
    // Choose color based on passage count
    uint16_t color;
    if (passageCount == 0)
      color = TFT_GREEN; // First segment
    else if (passageCount == 1)
      color = TFT_YELLOW; // Second segment
    else
      color = TFT_RED; // Third segment

    tft.fillCircle(x, y, 1, color); // Draw a 1-pixel filled dot

    // Check if the satellite has moved away from the starting x-coordinate
    if (!hasLeftStartX && abs(x - startX) > 20) // Moved far enough away
    {
      hasLeftStartX = true;
    }

    // Check if the satellite has returned close to the starting x-coordinate after leaving it
    if (hasLeftStartX && abs(x - startX) < 5) // Adjust tolerance as needed
    {
      passageCount++;        // Increment passage count
      hasLeftStartX = false; // Reset for next passage
    }

    // Increment time
    t += timeStep;
  }

  tft.fillCircle(currentXpos, currentYpos + 40, 5, TFT_YELLOW); // Mark starting point
  tft.drawCircle(currentXpos, currentYpos + 40, 6, TFT_RED);
  tft.drawCircle(currentXpos, currentYpos + 40, 7, TFT_RED);

  // Call the drawFootprint function
  drawFootprint(currentXpos, currentYpos, sat.satAlt);
}

void displayTableNext10Passes()
{
  passinfo overpass;

  // Initialize prediction start point
  sat.initpredpoint(unixtime, MIN_ELEVATION);

  Serial.println("Next 10 Passes:");
  Serial.println("--------------------");

  int year, month, day, hour, minute;
  double second;
  bool daylightSaving = true;

  // Clear the TFT and set up the screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);

  // Draw headers
  tft.setCursor(0, 0);
  tft.print("DATE");
  tft.setCursor(85, 0);
  tft.print("AOS");
  tft.setCursor(164, 0);
  tft.print("TCA");
  tft.setCursor(244, 0);
  tft.print("LOS");
  tft.setCursor(324, 0);
  tft.print("DUR");
  tft.setCursor(400, 0);
  tft.print("MEL");

  // Adjust timezone and DST offset
  long adjustedTimezoneOffset = timezoneOffset + dstOffset; // Correctly adjust for DST and timezone offset

  // Iterate through the next 10 passes
  for (int i = 1; i <= 12; i++) // Loop for next 12 passes (as per your original code)
  {
    bool passFound = sat.nextpass(&overpass, 100);
    if (passFound)
    {
      // Prepare persistent `struct tm` objects for AOS, TCA, and LOS
      struct tm aosTm = {0}, tcaTm = {0}, losTm = {0};

      // Convert AOS: Acquisition of Signal
      invjday(overpass.jdstart, adjustedTimezoneOffset / 3600, daylightSaving, year, month, day, hour, minute, second);
      aosTm.tm_year = year - 1900;
      aosTm.tm_mon = month - 1;
      aosTm.tm_mday = day;
      aosTm.tm_hour = hour;
      aosTm.tm_min = minute;

      // Convert TCA: Time of Closest Approach
      invjday(overpass.jdmax, adjustedTimezoneOffset / 3600, daylightSaving, year, month, day, hour, minute, second);
      tcaTm.tm_year = year - 1900;
      tcaTm.tm_mon = month - 1;
      tcaTm.tm_mday = day;
      tcaTm.tm_hour = hour;
      tcaTm.tm_min = minute;

      // Convert LOS: Loss of Signal
      invjday(overpass.jdstop, adjustedTimezoneOffset / 3600, daylightSaving, year, month, day, hour, minute, second);
      losTm.tm_year = year - 1900;
      losTm.tm_mon = month - 1;
      losTm.tm_mday = day;
      losTm.tm_hour = hour;
      losTm.tm_min = minute;

      // Calculate pass duration (in seconds)
      unsigned long passDuration = (unsigned long)((overpass.jdstop - overpass.jdstart) * 86400);
      int passMinutes = passDuration / 60;
      int passSeconds = passDuration % 60;

      // Format pass duration as HH:MM
      char durationFormatted[10];
      sprintf(durationFormatted, "%02d:%02d", passMinutes, passSeconds);

      // Maximum elevation (rounded to the nearest integer)
      int maxElevation = (int)round(overpass.maxelevation);

      // Format date without year (just day and month)
      char passDate[6];
      sprintf(passDate, "%02d.%02d", day, month);

      // Format AOS, TCA, and LOS times in HH:MM format
      char aosTime[6], tcaTime[6], losTime[6];
      sprintf(aosTime, "%02d:%02d", aosTm.tm_hour, aosTm.tm_min);
      sprintf(tcaTime, "%02d:%02d", tcaTm.tm_hour, tcaTm.tm_min);
      sprintf(losTime, "%02d:%02d", losTm.tm_hour, losTm.tm_min);

      // Serial output for debugging
      Serial.printf("%02d %s | AOS %s TCA %s LOS %s DUR %s MEL: %d\n",
                    i, passDate, aosTime, tcaTime, losTime, durationFormatted, maxElevation);

      // Highlight if elevation is above 30° for radio ham contact
      if (maxElevation > 35)
      {
        tft.setTextColor(TFT_GREEN, TFT_BLACK); // Highlight in green
      }
      else
      {
        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK); // Normal color
      }

      // TFT output: Display pass information on screen
      int yPosition = i * 23 + 5; // Adjust vertical spacing for each row
      tft.setCursor(0, yPosition);
      tft.printf("%s", passDate);
      tft.setCursor(80, yPosition);
      tft.printf("%s", aosTime);
      tft.setCursor(160, yPosition);
      tft.printf("%s", tcaTime);
      tft.setCursor(240, yPosition);
      tft.printf("%s", losTime);
      tft.setCursor(320, yPosition);
      tft.printf("%s", durationFormatted);
      tft.setCursor(400, yPosition);
      tft.printf("%.1f", overpass.maxelevation);
    }
    else
    {
      // If no more passes are found, exit the loop
      Serial.println("No more passes found.");
      break;
    }
  }
}

// Helper function to format numbers with a thousands separator
String formatWithSeparators(int number)
{
  tft.setTextFont(4); // Set font to 4 (ensure it's enabled in TFT_eSPI setup)
  tft.setTextSize(1); // Set default text size
  // Convert the number to a string
  String result = String(number);
  int len = result.length();

  // Insert thousands separators ('), starting from the right
  for (int i = len - 3; i > 0; i -= 3)
  {
    result = result.substring(0, i) + "'" + result.substring(i);
  }
  return result;
}

// Function to display a raw number (no separators), right-aligned
// Only updates digits that change
void displayRawNumberRightAligned(int rightEdgeX, int y, int number, int color)
{
  tft.setTextFont(4);                  // Set font to 4 (ensure it's enabled in TFT_eSPI setup)
  tft.setTextSize(1);                  // Set default text size
  static String previousRawValue = ""; // Store the previously displayed value
  String rawValue = String(number);    // Convert the current number to a string

  // Calculate the total width of the raw number
  int totalWidth = tft.textWidth(rawValue);
  int xStart = rightEdgeX - totalWidth; // Calculate the starting x position

  // Update only changed characters
  for (int i = 0; i < rawValue.length(); i++)
  {
    if (i >= previousRawValue.length() || rawValue[i] != previousRawValue[i])
    {
      // Erase old digit by printing a black character
      tft.setCursor(xStart, y);
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.print(i < previousRawValue.length() ? previousRawValue[i] : ' ');

      // Draw the new digit
      tft.setCursor(xStart, y);
      tft.setTextColor(color, TFT_BLACK);
      tft.print(rawValue[i]);
    }
    xStart += tft.textWidth("0"); // Move cursor forward by the width of one digit
  }

  // Store the current value for future comparisons
  previousRawValue = rawValue;
}

/*
// Function to display latitude/longitude with one decimal, right-aligned
// Only updates digits that change (i.e., decimal places or sign changes)
void displayLatLonRightAligned(int rightEdgeX, int y, float latLon, int color)
{
  tft.setTextFont(4);                  // Set font to 4 (ensure it's enabled in TFT_eSPI setup)
  tft.setTextSize(1);                  // Set default text size
  static String previousRawValue = ""; // Store the previously displayed value

  // Format the number to one decimal place
  String rawValue = String(latLon, 1); // Convert to string with one decimal place

  // Check if the value has a negative sign, handle accordingly
  if (latLon < 0)
  {
    rawValue = "-" + rawValue.substring(1); // Keep the minus sign for negative values
  }

  // Calculate the total width of the formatted number
  int totalWidth = tft.textWidth(rawValue);
  int xStart = rightEdgeX - totalWidth; // Calculate the starting x position

  // Update only changed characters
  for (int i = 0; i < rawValue.length(); i++)
  {
    if (i >= previousRawValue.length() || rawValue[i] != previousRawValue[i])
    {
      // Erase old character by printing a black space (or black character)
      tft.setCursor(xStart, y);
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.print(i < previousRawValue.length() ? previousRawValue[i] : ' ');

      // Draw the new character
      tft.setCursor(xStart, y);
      tft.setTextColor(color, TFT_BLACK);
      tft.print(rawValue[i]);
    }
    xStart += tft.textWidth("0"); // Move cursor forward by the width of one digit
  }

  // Store the current value for future comparisons
  previousRawValue = rawValue;
}
*/
// Function to display a formatted number with separators, right-aligned
// Only updates digits or separators that change
void displayFormattedNumberRightAligned(int rightEdgeX, int y, int number, int color)
{
  tft.setTextFont(4);                                   // Set font to 4 (ensure it's enabled in TFT_eSPI setup)
  tft.setTextSize(1);                                   // Set default text size
  static String previousFormattedValue = "";            // Store the previously displayed value
  String formattedValue = formatWithSeparators(number); // Format the current number

  // Calculate the total width of the formatted number
  int totalWidth = 0;
  for (int i = 0; i < formattedValue.length(); i++)
  {
    totalWidth += (formattedValue[i] == '\'') ? tft.textWidth("'") : tft.textWidth("0");
  }
  int xStart = rightEdgeX - totalWidth; // Calculate the starting x position

  // Update only changed characters
  for (int i = 0; i < formattedValue.length(); i++)
  {
    if (i >= previousFormattedValue.length() || formattedValue[i] != previousFormattedValue[i])
    {
      // Erase old character by printing a black character
      tft.setCursor(xStart, y);
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.print(i < previousFormattedValue.length() ? previousFormattedValue[i] : ' ');

      // Draw the new character
      tft.setCursor(xStart, y);
      tft.setTextColor(color, TFT_BLACK);
      tft.print(formattedValue[i]);
    }
    xStart += (formattedValue[i] == '\'') ? tft.textWidth("'") : tft.textWidth("0"); // Advance cursor
  }

  // Store the current value for future comparisons
  previousFormattedValue = formattedValue;
}

// Function to display a timer in HH:MM:SS format, right-aligned
// Only updates digits or colons that change
void displayTimeRightAligned(int rightEdgeX, int y, int seconds, int color, bool refresh)
{
  tft.setTextFont(4); // Set font to 4 (ensure it's enabled in TFT_eSPI setup)
  tft.setTextSize(1); // Set default text size
  // Convert seconds into hours, minutes, and seconds
  int hours = seconds / 3600;
  seconds %= 3600;
  int minutes = seconds / 60;
  seconds %= 60;
  static bool isInitiated;
  // Format time as HH:MM:SS with leading zeros
  String timeString = "";
  timeString += (hours < 10 ? "0" : "") + String(hours) + ":";
  timeString += (minutes < 10 ? "0" : "") + String(minutes) + ":";
  timeString += (seconds < 10 ? "0" : "") + String(seconds);

  static String previousTimeString = ""; // Store the previously displayed value
  if (!isInitiated || refresh)
  {
    previousTimeString = "";
    isInitiated = true;
  }

  // Calculate the total width of the time string
  int totalWidth = 0;
  for (int i = 0; i < timeString.length(); i++)
  {
    totalWidth += (timeString[i] == ':') ? tft.textWidth(":") : tft.textWidth("0");
  }
  int xStart = rightEdgeX - totalWidth; // Calculate the starting x position

  // Update only changed characters
  for (int i = 0; i < timeString.length(); i++)
  {
    if (i >= previousTimeString.length() || timeString[i] != previousTimeString[i])
    {
      // Erase old character by printing a black character
      tft.setCursor(xStart, y);
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.print(i < previousTimeString.length() ? previousTimeString[i] : ' ');

      // Draw the new character
      tft.setCursor(xStart, y);
      tft.setTextColor(color, TFT_BLACK);
      tft.print(timeString[i]);
    }
    xStart += (timeString[i] == ':') ? tft.textWidth(":") : tft.textWidth("0"); // Advance cursor
  }

  // Store the current value for future comparisons
  previousTimeString = timeString;
}
void displayDistance(int number, int x, int y, uint16_t color, bool refresh)
{
  tft.setTextSize(1);
  tft.setTextFont(4);

  static char previousOutput[7] = "      ";  // Previous state (6 characters + null terminator) 12'456
  static uint16_t previousColor = TFT_GREEN; // Track the last color used
  static bool isInitiated = false;           // Track initialization of static elements
  char output[7] = "      ";                 // Current output (6 characters + null terminator)

  char tempBuffer[7] = ""; // Temporary buffer for formatted number
  int len = 0;             // Length of the formatted number
  int index = 5;           // Start filling from the right (5 characters + null terminator)

  // Format the number with thousands separator
  while (number > 0 || len == 0)
  { // Ensure '0' is handled correctly
    if (len > 0 && len % 3 == 0)
    {
      tempBuffer[index--] = '\''; // Insert the separator
    }
    tempBuffer[index--] = '0' + (number % 10);
    number /= 10;
    len++;
  }

  // Fill the output buffer with spaces for right alignment
  for (int i = 0; i <= index; i++)
  {
    output[i] = ' ';
  }
  // Copy the formatted number into the output buffer
  for (int i = index + 1; i < 6; i++)
  {
    output[i] = tempBuffer[i];
  }

  output[6] = '\0'; // Ensure null termination

  // Positions for characters
  int shift = 8;
  int xPos[] = {0, 14, 28, 42 - shift, 56 - shift, 70 - shift};
  int leftmargin = 110;

  for (int i = 0; i < 6; i++)
    xPos[i] += x + leftmargin;

  // Handle color change or refresh logic
  if (color != previousColor)
  {
    // If the color has changed, trigger a full redraw
    refresh = true;
    previousColor = color; // Update the last used color
  }

  // Handle refresh logic for static elements (decimal point)
  if (!isInitiated || refresh)
  {
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString("km", xPos[5] + 24, y);
    tft.drawString("Distance:", x, y);
    isInitiated = true;
  }

  // Update only changed characters or redraw all if refresh is triggered
  for (int i = 0; i < 6; i++)
  { // Loop through each character of the formatted number
    Serial.print(output[i]);
    Serial.print(" ");
    if (output[i] != previousOutput[i] || refresh)
    {
      // Print the previous character in black to erase it
      if (previousOutput[i] != ' ')
      {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawChar(previousOutput[i], xPos[i], y);
      }

      // Draw the new character
      if (output[i] != ' ')
      {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawChar(output[i], xPos[i], y);
      }

      // Update previous state
      previousOutput[i] = output[i];
    }
  }
  Serial.println();
}
void displayAltitude(int number, int x, int y, uint16_t color, bool refresh)
{
  tft.setTextSize(1);
  tft.setTextFont(4);

  static char previousOutput[7] = "      ";  // Previous state (6 characters + null terminator) 12'456
  static uint16_t previousColor = TFT_GREEN; // Track the last color used
  static bool isInitiated = false;           // Track initialization of static elements
  char output[7] = "      ";                 // Current output (6 characters + null terminator)

  char tempBuffer[7] = ""; // Temporary buffer for formatted number
  int len = 0;             // Length of the formatted number
  int index = 5;           // Start filling from the right (5 characters + null terminator)

  // Format the number with thousands separator
  while (number > 0 || len == 0)
  { // Ensure '0' is handled correctly
    if (len > 0 && len % 3 == 0)
    {
      tempBuffer[index--] = '\''; // Insert the separator
    }
    tempBuffer[index--] = '0' + (number % 10);
    number /= 10;
    len++;
  }

  // Fill the output buffer with spaces for right alignment
  for (int i = 0; i <= index; i++)
  {
    output[i] = ' ';
  }
  // Copy the formatted number into the output buffer
  for (int i = index + 1; i < 6; i++)
  {
    output[i] = tempBuffer[i];
  }

  output[6] = '\0'; // Ensure null termination

  // Positions for characters
  int shift = 8;
  int xPos[] = {0, 14, 28, 42 - shift, 56 - shift, 70 - shift};
  int leftmargin = 110;

  for (int i = 0; i < 6; i++)
    xPos[i] += x + leftmargin;

  // Handle color change or refresh logic
  if (color != previousColor)
  {
    // If the color has changed, trigger a full redraw
    refresh = true;
    previousColor = color; // Update the last used color
  }

  // Handle refresh logic for static elements (decimal point)
  if (!isInitiated || refresh)
  {
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString("km", xPos[5] + 24, y);
    tft.drawString("Altitude:", x, y);
    isInitiated = true;
  }

  // Update only changed characters or redraw all if refresh is triggered
  for (int i = 0; i < 6; i++)
  { // Loop through each character of the formatted number
    Serial.print(output[i]);
    Serial.print(" ");
    if (output[i] != previousOutput[i] || refresh)
    {
      // Print the previous character in black to erase it
      if (previousOutput[i] != ' ')
      {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawChar(previousOutput[i], xPos[i], y);
      }

      // Draw the new character
      if (output[i] != ' ')
      {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawChar(output[i], xPos[i], y);
      }

      // Update previous state
      previousOutput[i] = output[i];
    }
  }
  Serial.println();
}
void displayOrbitNumber(int number, int x, int y, uint16_t color, bool refresh)
{
  tft.setTextSize(1);
  tft.setTextFont(4);

  static char previousOutput[7] = "      ";  // Previous state (6 characters + null terminator) 12'456
  static uint16_t previousColor = TFT_GREEN; // Track the last color used
  static bool isInitiated = false;           // Track initialization of static elements
  char output[7] = "      ";                 // Current output (6 characters + null terminator)

  char tempBuffer[7] = ""; // Temporary buffer for formatted number
  int len = 0;             // Length of the formatted number
  int index = 5;           // Start filling from the right (5 characters + null terminator)

  // Format the number with thousands separator
  while (number > 0 || len == 0)
  { // Ensure '0' is handled correctly
    if (len > 0 && len % 3 == 0)
    {
      tempBuffer[index--] = '\''; // Insert the separator
    }
    tempBuffer[index--] = '0' + (number % 10);
    number /= 10;
    len++;
  }

  // Fill the output buffer with spaces for right alignment
  for (int i = 0; i <= index; i++)
  {
    output[i] = ' ';
  }
  // Copy the formatted number into the output buffer
  for (int i = index + 1; i < 6; i++)
  {
    output[i] = tempBuffer[i];
  }

  output[6] = '\0'; // Ensure null termination

  // Positions for characters
  int shift = 8;
  int xPos[] = {0, 14, 28, 42 - shift, 56 - shift, 70 - shift};
  int leftmargin = 110;

  for (int i = 0; i < 6; i++)
    xPos[i] += x + leftmargin;

  // Handle color change or refresh logic
  if (color != previousColor)
  {
    // If the color has changed, trigger a full redraw
    refresh = true;
    previousColor = color; // Update the last used color
  }

  // Handle refresh logic for static elements (decimal point)
  if (!isInitiated || refresh)
  {
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString("Orbit #:", x, y);
    isInitiated = true;
  }

  // Update only changed characters or redraw all if refresh is triggered
  for (int i = 0; i < 6; i++)
  { // Loop through each character of the formatted number
    Serial.print(output[i]);
    Serial.print(" ");
    if (output[i] != previousOutput[i] || refresh)
    {
      // Print the previous character in black to erase it
      if (previousOutput[i] != ' ')
      {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawChar(previousOutput[i], xPos[i], y);
      }

      // Draw the new character
      if (output[i] != ' ')
      {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawChar(output[i], xPos[i], y);
      }

      // Update previous state
      previousOutput[i] = output[i];
    }
  }
  Serial.println();
}

void setup()
{
  // clearPreferences();// uncomment for testing
  Serial.begin(115200);
  pinMode(TFT_BLP, OUTPUT);
  digitalWrite(TFT_BLP, HIGH);

  initializeTFT();
  // displayBlueMapimage(); Alternative Mapd

  displayISSimage(1000);
  printWelcomeMessage();
  int pause = 0;
  connectToWiFi();
  delay(pause);
  getTimezoneData();
  delay(pause);
  syncTimeFromNTP();
  delay(pause);

  getTLEelementsMain(tleLine1, tleLine2);

  sat.init("ISS (ZARYA)", tleLine1, tleLine2);
  sat.site(OBSERVER_LATITUDE, OBSERVER_LONGITUDE, OBSERVER_ALTITUDE);

  displayTableNext10Passes();

  delay(pause);
  displayMapWithMultiPasses();
  displayPolarPlotPage();
  displayAzElPlotPage();
  displayPExpedition72image();
  delay(3000);
  displayPatreonimage();
  delay(3000);

  tft.fillScreen(TFT_BLACK); // clear the screen
  // tkSecond.attach(1, Second_Tick);
}

void loop()
{

  // Get the current touch pressure
  touchTFT = tft.getTouchRawZ();

  // Check if the touch pressure exceeds the threshold and debounce
  if (touchTFT > 1000)
  {
    // Only increment the counter if enough time has passed since the last touch
    if (millis() - lastTouchTime > debounceDelay)
    {
      touchCounter++; // Increment the counter

      // If counter exceeds 5, reset it to 1
      if (touchCounter > 6)
      {
        touchCounter = 1;

        // Reset page display flags so that pages can be displayed again
        page1Displayed = false;
        page2Displayed = false;
        page3Displayed = false;
        page4Displayed = false;
        page5Displayed = false;
        page6Displayed = false;
      }

      // Print the counter
      Serial.print("Counter: ");
      Serial.println(touchCounter);

      // Call the respective functions based on the counter value
      switch (touchCounter)
      {
      case 1:
        if (!page1Displayed)
        {
          tft.fillScreen(TFT_BLACK);
          updateBigClock(true);
          refresh = true;
          page1Displayed = true; // Set the flag to prevent re-display
        }

        displayMainPage(); // Show page 1

        break;
      case 2:
        if (!page2Displayed)
        {
          displayPolarPlotPage();
          page2Displayed = true; // Set the flag to prevent re-display
        }
        break;
      case 3:
        if (!page3Displayed)
        {
          displayTableNext10Passes(); // Show page 3
          page3Displayed = true;      // Set the flag to prevent re-display
        }
        break;
      case 4:
        if (!page4Displayed)
        {
          displayMapWithMultiPasses(); // Show page 4
          page4Displayed = true;       // Set the flag to prevent re-display
        }
        break;
      case 5:
        if (!page5Displayed)
        {
          displayAzElPlotPage();
          page5Displayed = true; // Set the flag to prevent re-display
        }
        break;

      case 6:
        if (!page5Displayed)
        {

          displayPExpedition72image();
          page6Displayed = true; // Set the flag to prevent re-display
        }
        break;
      default:
        break;
      }

      lastTouchTime = millis(); // Update the time of the last touch
    }
  }
  // Execute every second
  static unsigned long lastLoopTime = millis();

  if (millis() - lastLoopTime >= 1000 && touchCounter == 1)
  {

    // Calculate Range Rate
    static double previousDistance = 0.0;  // Store the previous distance
    static unsigned long previousTime = 0; // Store the previous time (milliseconds)
    double currentDistance = sat.satDist;  // Current satellite distance
    unsigned long currentTime = millis();  // Current time in milliseconds

    // Calculate time difference
    double deltaTime = (currentTime - previousTime) / 1000.0;

    if (previousDistance > 0 && deltaTime > 0)
    {
      // Calculate range rate
      double rangeRate = (currentDistance - previousDistance) / deltaTime;
    }

    // Update previous values
    previousDistance = currentDistance;
    previousTime = currentTime;
    displayMainPage();
    // Update the last loop time
    lastLoopTime = millis();
  }
}

void displayMainPage()
{
  // Update the time from NTP
  timeClient.update();

  getOrbitNumber(unixtime);
  unixtime = timeClient.getEpochTime(); // Get the current UNIX timestamp
  unsigned long nextpassInSec = nextPassStart - unixtime;
  // Update the satellite data
  sat.findsat(unixtime);
  Serial.println(unixtime);

  updateBigClock();
  int AZELcolor;
  // Determine AZELcolor based on nextpassInSec
  if (nextpassInSec < 30)
  {
    AZELcolor = TFT_YELLOW;
  }
  else if (nextpassInSec < 60)
  {
    AZELcolor = TFT_GREEN;
  }
  else
  {
    AZELcolor = TFT_RED; // Default or another color if required
  }
  displayElevation(sat.satEl, 5 + 30, 108, AZELcolor, refresh);
  displayAzimuth(sat.satAz, 303 - 30, 108, AZELcolor, refresh);

  int startXmain = 30;
  int startYmain = 200;
  int deltaY = 30;

  displayAltitude(sat.satAlt, 25, startYmain, TFT_GOLD, refresh);
  displayDistance(sat.satDist, 25, startYmain + 1 * deltaY, TFT_GOLD, refresh);
  displayOrbitNumber(orbitNumber, 25, startYmain + 2 * deltaY, TFT_GOLD, refresh);
  displayLatitude(sat.satLat, 320, startYmain, TFT_GOLD, refresh);
  displayLongitude(sat.satLon, 320, startYmain + deltaY, TFT_GOLD, refresh);
  displayLTLEage(startYmain + 2 * deltaY, refresh);

  int shifting = 40;
  int nextPass = 295;
  tft.setCursor(shifting, nextPass);
  tft.setTextColor(TFT_CYAN);
  tft.print("Next Pass in ");
  displayTimeRightAligned(tft.textWidth("Next pass in 00:00:00 ") + shifting, nextPass, nextpassInSec, TFT_CYAN, refresh);
  tft.setCursor(tft.textWidth("Next pass in 00:00:00  ") + shifting, nextPass);
  tft.print(" at ");
  tft.print(formatTimeOnly(nextPassStart, true));

  Serial.print("nextPassStart:  ");
  Serial.println(nextPassStart);
  Serial.print("unixtime:  ");
  Serial.println(unixtime);
  Serial.print("delta:  ");
  Serial.println(nextPassStart - unixtime);

  Serial.println(formatTime(unixtime));
  Serial.print("Azimuth = ");
  Serial.print(sat.satAz, 2);
  Serial.print("°, Elevation = ");
  Serial.print(sat.satEl, 2);
  Serial.print("°, Distance = ");
  Serial.print(sat.satDist, 2);
  Serial.println(" km");

  Serial.print("Latitude = ");
  Serial.print(sat.satLat, 2);
  Serial.print("°, Longitude = ");
  Serial.print(sat.satLon, 2);
  Serial.print("°, Altitude = ");
  Serial.print(sat.satAlt, 2);
  Serial.println(" km");

  refresh = false;
}
