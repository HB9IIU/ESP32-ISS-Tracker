#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Sgp4.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <Preferences.h> // For flash storage of TLE data
#include <PNGdec.h>      // Include the PNG decoder library
#include <SolarCalculator.h>
#include <HB9IIU7segFonts.h> //  https://rop.nl/truetype2gfx/   https://fontforge.org/en-US/

// TFT Setup
TFT_eSPI tft = TFT_eSPI();
// PNG decoder instance
PNG png;
// https://notisrac.github.io/FileToCArray/
#include "ISSsplashImage.h" // Image is stored here in an 8-bit array
#include "worldMap.h"       // Image is stored here in an 8-bit array
// #include "blueMap.h"        // Image is stored here in an 8-bit array
// #include "patreon.h" // Image is stored here in an 8-bit array
#include "fancySplashImage.h"
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
char TLEageHHMM[6];          // Buffer to store the formatted age (HH:MM)
int bootingMessagePause = 0; // for TFT messages at boot
bool refresh = false;
unsigned long multipassMaplastRefreshTime = 0;
unsigned long PolarPlotlastRefreshTime = 0;
unsigned long AzElPlotlastRefreshTime = 0;

unsigned long lastTLEUpdate = 0;              // Track the last update time
const unsigned long updateInterval = 3600000; // 1 hour in milliseconds

// Flag to track if time was successfully updated at least once
bool timeInitialized = false;
// NTP Client for UTC time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "129.6.15.28", 0, 60000);

// for the touchscreen
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
bool first_time_below = true;
bool first_time_above = true;
// Function declarations
void computeSun();
void display7segmentClock(unsigned long unixTime, int xOffset, int yOffset, uint16_t textColor, bool refresh);
String displayRemainingVisibleTimeinMMSS(int delta) ;
void displayRemainingPassTime(unsigned long durationInSec, int x, int y, uint16_t color, bool refresh);
void displayNextPassTime(unsigned long durationInSec, int x, int y, uint16_t color, bool refresh);
void displayRawNumberRightAligned(int rightEdgeX, int y, int number, int color);
void displayFormattedNumberRightAligned(int rightEdgeX, int y, int number, int color);
String formatTime(unsigned long epochTime, bool isLocal);
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
void displayWelcomeMessage()
{
    for (int i = 0; i < 30; i++)
    { // Adjust as needed
        Serial.println();
    }
    String title = "Welcome to HB9IIU ISS Life Tracker";
    String version = String("Version: ") + VERSION_NUMBER + " (" + VERSION_DATE + ")";
    String disclaimer = "Please use at your own risk!";
    TFTprint("       ");
    TFTprint("       " + title), TFT_GOLD;
    TFTprint("       ");
    TFTprint("               " + version, TFT_GOLD);
    TFTprint("       ");
    TFTprint("             " + disclaimer, TFT_GOLD);
    TFTprint("       ");
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
    TFTprint("");
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
            TFTprint("");

            String message = "Connecting to alternative Wi-Fi: " + String(WIFI_SSID_ALT);
            TFTprint(message, TFT_YELLOW);
            WiFi.begin(WIFI_SSID_ALT, WIFI_PASSWORD_ALT);
        }

        // Check Wi-Fi connection status
        for (int i = 0; i < 10; i++)
        { // Wait up to 5 seconds
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("\nConnected to Wi-Fi.");
                TFTprint("Wi-Fi Connection Successful", TFT_GREEN);

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
                TFTprint("Failed to connect to both networks", TFT_RED);
                delay(1000);
                TFTprint("Retrying...", TFT_RED);

                attempt = 0; // Reset attempts to retry both networks
            }
            else if (attempt == maxAttempts)
            {
                Serial.println("\nSwitching to alternative network...");
                TFTprint("Switching to alternative network...", TFT_YELLOW);
            }
        }
    }

    // Get Wi-Fi signal strength (RSSI)
    int32_t rssi = WiFi.RSSI();
    Serial.print("Signal Strength: ");
    Serial.print(rssi);
    Serial.println(" dBm");
    TFTprint("Signal Strength: " + String(rssi) + " dBm", TFT_GOLD);
}

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
    TFTprint("Failed to retrieve timezone data.", TFT_RED);
    TFTprint("Check your API key.", TFT_YELLOW);
    TFTprint("");
    TFTprint("Rebooting...", TFT_RED);
    delay(1500);   // Brief delay to display the message before reboot
    ESP.restart(); // Reboot the ESP32
}

void syncTimeFromNTP()
{
    // List of NTP server IPs with corresponding server names
    Serial.println();
    TFTprint("       ");

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
                    TFTprint(formatTime(unixtime, true), TFT_GREEN);
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
bool getTLEelements(char *tleLine1, char *tleLine2, bool displayTLEinfoOnTFT)
{
    Serial.println("\n--- Retrieving TLE Data from Preferences ---");

    // Open preferences in read-only mode
    preferences.begin(TLE_PREF_NAMESPACE, true);
    String storedLine1 = preferences.getString(TLE_LINE1_KEY, "");
    String storedLine2 = preferences.getString(TLE_LINE2_KEY, "");
    unsigned long storedTLETime = preferences.getULong(TLE_TIMESTAMP_KEY, 0);
    preferences.end();

    // Validate stored TLE data
    if (storedLine1.isEmpty() || storedLine2.isEmpty() || storedTLETime == 0)
    {
        Serial.println("No valid TLE data found in preferences.");
        return false;
    }

    // Calculate TLE age
    unsigned long ageInSeconds = unixtime - storedTLETime;
    unsigned long ageInDays = ageInSeconds / 86400;
    unsigned long remainingSeconds = ageInSeconds % 86400;
    unsigned long ageInHours = remainingSeconds / 3600;
    unsigned long ageInMinutes = (remainingSeconds % 3600) / 60;

    // Format TLE age for display
    char ageFormatted[50];
    sprintf(ageFormatted, "%lu days, %lu hours, %lu minutes", ageInDays, ageInHours, ageInMinutes);

    // Format age for HH:MM variable
    sprintf(TLEageHHMM, "%02lu:%02lu", ageInHours, ageInMinutes);

    // Debug outputs
    Serial.println("TLE data retrieved successfully:");
    Serial.println("  TLE Line 1: " + storedLine1);
    Serial.println("  TLE Line 2: " + storedLine2);
    Serial.println("  TLE Age: " + String(ageFormatted));

    if (displayTLEinfoOnTFT)
        TFTprint("TLE Line 1:", TFT_GREEN);
    TFTprint(storedLine1, TFT_YELLOW);
    TFTprint("");
    TFTprint("TLE Line 2:", TFT_GREEN);
    TFTprint(storedLine2, TFT_YELLOW);
    TFTprint("");
    TFTprint("TLE Age: " + String(ageFormatted), TFT_GREEN);

    // Copy stored TLE lines to output parameters
    storedLine1.toCharArray(tleLine1, 70);
    storedLine2.toCharArray(tleLine2, 70);

    return true;
}

void processTLE(String line1, String line2, String tleDate, time_t &youngestTLETime, String &youngestTLELine1, String &youngestTLELine2);

bool refreshTLEelements(bool displayTLEinfoOnTFT)
{
    const char *servers[] = {tleUrlMain, tleUrlFallback}; // List of TLE servers
    time_t youngestTLETime = 0;                           // Timestamp of the youngest TLE found
    String youngestTLELine1 = "";                         // Line 1 of the youngest TLE
    String youngestTLELine2 = "";                         // Line 2 of the youngest TLE
    bool retryNeeded = false;

    Serial.println("\n--- Starting TLE Refresh Process ---");
    if (displayTLEinfoOnTFT)
        TFTprint("Starting TLE Refresh Process... ");
    // Iterate through both servers
    for (int i = 0; i < sizeof(servers) / sizeof(servers[0]); ++i)
    {
        Serial.println("\nContacting server: " + String(servers[i]));

        HTTPClient http;
        http.begin(servers[i]);
        int httpCode = http.GET();

        if (httpCode == 200)
        {
            Serial.println("Server responded successfully.");
            String payload = http.getString();
            Serial.println("Raw response from server:");
            Serial.println(payload);

            String line1, line2, tleDate;

            // Check if the response is JSON or plain text
            if (payload.startsWith("{"))
            {
                // Process JSON response
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload);
                if (!error)
                {
                    line1 = String(doc["line1"].as<const char *>());
                    line2 = String(doc["line2"].as<const char *>());
                    tleDate = String(doc["date"].as<const char *>());
                }
                else
                {
                    Serial.println("JSON parsing error: " + String(error.c_str()));
                    retryNeeded = true;
                    continue;
                }
            }
            else
            {
                // Process plain text response
                int line1Start = payload.indexOf("\n1 ");
                int line2Start = payload.indexOf("\n2 ", line1Start);
                if (line1Start != -1 && line2Start != -1)
                {
                    line1 = payload.substring(line1Start + 1, line2Start);
                    line2 = payload.substring(line2Start + 1);
                    tleDate = ""; // No explicit date in plain text
                }
                else
                {
                    Serial.println("Failed to extract TLE data from plain text response.");
                    retryNeeded = true;
                    continue;
                }
            }

            // Process TLE data
            time_t tleEpochUnix = 0;
            if (tleDate != "")
            {
                // Parse ISO 8601 date
                struct tm tleTime = {0};
                sscanf(tleDate.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
                       &tleTime.tm_year, &tleTime.tm_mon, &tleTime.tm_mday,
                       &tleTime.tm_hour, &tleTime.tm_min, &tleTime.tm_sec);
                tleTime.tm_year -= 1900;
                tleTime.tm_mon -= 1;
                tleEpochUnix = mktime(&tleTime);
            }
            else
            {
                // Extract epoch from Line 1 for plain text
                int epochYear = line1.substring(18, 20).toInt() + 2000;
                float epochDay = line1.substring(20, 32).toFloat();

                struct tm tleTime = {0};
                tleTime.tm_year = epochYear - 1900;
                tleTime.tm_mday = 1;
                tleTime.tm_mon = 0;
                tleEpochUnix = mktime(&tleTime) + (unsigned long)((epochDay - 1) * 86400);
            }

            // Calculate TLE age
            unsigned long ageInSeconds = unixtime - tleEpochUnix;
            unsigned long ageInDays = ageInSeconds / 86400;
            unsigned long remainingSeconds = ageInSeconds % 86400;
            unsigned long ageInHours = remainingSeconds / 3600;
            unsigned long ageInMinutes = (remainingSeconds % 3600) / 60;

            char ageFormatted[50];
            sprintf(ageFormatted, "%lu days, %lu hours, %lu minutes", ageInDays, ageInHours, ageInMinutes);
            Serial.println("TLE Age from server: " + String(ageFormatted));

            // Compare with the current youngest TLE
            if (youngestTLETime == 0 || tleEpochUnix > youngestTLETime)
            {
                youngestTLETime = tleEpochUnix;
                youngestTLELine1 = line1;
                youngestTLELine2 = line2;
                Serial.println("This TLE is retained as the youngest so far.");
            }
        }
        else
        {
            Serial.println("HTTP error for server: " + String(servers[i]));
            Serial.println("HTTP Code: " + String(httpCode));
            retryNeeded = true;
        }
        http.end();
    }

    // If no valid TLE was found, return without updating
    if (youngestTLETime == 0)
    {
        Serial.println("No valid TLE data found from any server.");
        return retryNeeded;
    }

    // Open preferences and print stored TLE age
    preferences.begin(TLE_PREF_NAMESPACE, true);
    unsigned long storedTLETime = preferences.getULong(TLE_TIMESTAMP_KEY, 0);
    preferences.end();

    if (storedTLETime > 0)
    {
        unsigned long storedAgeInSeconds = unixtime - storedTLETime;
        unsigned long storedAgeInDays = storedAgeInSeconds / 86400;
        unsigned long remainingSeconds = storedAgeInSeconds % 86400;
        unsigned long storedAgeInHours = remainingSeconds / 3600;
        unsigned long storedAgeInMinutes = (remainingSeconds % 3600) / 60;

        char storedAgeFormatted[50];
        sprintf(storedAgeFormatted, "%lu days, %lu hours, %lu minutes", storedAgeInDays, storedAgeInHours, storedAgeInMinutes);

        Serial.println("\nCurrently Stored TLE Age:");
        Serial.println("  Age: " + String(storedAgeFormatted));
    }
    else
    {
        Serial.println("\nNo TLE data currently stored. This is the first update.");
        if (displayTLEinfoOnTFT)
            TFTprint("Very first TLE Update");
    }

    // Decide whether to update preferences
    if (storedTLETime == 0 || youngestTLETime > storedTLETime)
    {
        Serial.println("\nDecision: Updating stored TLE data with the youngest TLE.");
        preferences.begin(TLE_PREF_NAMESPACE, false);
        preferences.putString(TLE_LINE1_KEY, youngestTLELine1);
        preferences.putString(TLE_LINE2_KEY, youngestTLELine2);
        preferences.putULong(TLE_TIMESTAMP_KEY, youngestTLETime);
        preferences.end();

        Serial.println("New TLE data stored successfully:");
        Serial.println("  Line 1: " + youngestTLELine1);
        Serial.println("  Line 2: " + youngestTLELine2);
    }
    else
    {
        Serial.println("\nDecision: No update needed. Stored TLE is newer or the same.");
        if (displayTLEinfoOnTFT)
            TFTprint("No TLE update (stored are newer or same)");
    }

    return retryNeeded;
}

// Helper function to process and compare TLE
void processTLE(String line1, String line2, String tleDate, time_t &youngestTLETime, String &youngestTLELine1, String &youngestTLELine2)
{
    time_t tleEpochUnix = 0;

    if (tleDate != "")
    {
        // Parse ISO 8601 date from JSON into Unix timestamp
        struct tm tleTime = {0};
        sscanf(tleDate.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
               &tleTime.tm_year, &tleTime.tm_mon, &tleTime.tm_mday,
               &tleTime.tm_hour, &tleTime.tm_min, &tleTime.tm_sec);
        tleTime.tm_year -= 1900; // Adjust year
        tleTime.tm_mon -= 1;     // Adjust month (0-11)
        tleEpochUnix = mktime(&tleTime);
    }
    else
    {
        // Extract timestamp from Line 1 (Plain Text)
        int epochYear = line1.substring(18, 20).toInt() + 2000; // Convert 2-digit year to full year
        float epochDay = line1.substring(20, 32).toFloat();     // Extract fractional day of the year

        // Convert to Unix timestamp
        struct tm tleTime = {0};
        tleTime.tm_year = epochYear - 1900; // tm_year is years since 1900
        tleTime.tm_mday = 1;                // Start from January 1st
        tleTime.tm_mon = 0;
        tleEpochUnix = mktime(&tleTime) + (unsigned long)((epochDay - 1) * 86400); // Add fractional days
    }

    // Calculate TLE age
    unsigned long ageInSeconds = unixtime - tleEpochUnix;
    unsigned long ageInDays = ageInSeconds / 86400;
    unsigned long remainingSeconds = ageInSeconds % 86400;
    unsigned long ageInHours = remainingSeconds / 3600;
    unsigned long ageInMinutes = (remainingSeconds % 3600) / 60;

    char ageFormatted[50];
    sprintf(ageFormatted, "%lu days, %lu hours, %lu minutes", ageInDays, ageInHours, ageInMinutes);
    Serial.println("  TLE Age: " + String(ageFormatted));

    // Compare age with the current youngest TLE
    if (tleEpochUnix > youngestTLETime)
    {
        Serial.println("  This is the youngest TLE found so far.");
        youngestTLETime = tleEpochUnix;
        youngestTLELine1 = line1;
        youngestTLELine2 = line2;
    }
    else
    {
        Serial.println("  TLE from this server is older than the current youngest.");
    }
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
    orbitNumber = baselineOrbitNumber + (int)ceil(orbitsSinceEpoch);

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
    unixtime = timeClient.getEpochTime(); // Get the current UNIX time

    sat.initpredpoint(unixtime+10*60, MIN_ELEVATION);// adding 10 minutes to ensure that next pass is not in the past (experimental) 

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
    const int stepsInSeconds = 1; // Step size in seconds
    const int PLOT_X = 38;        // Left margin
    const int PLOT_Y = 20;        // Top margin
    const int PLOT_WIDTH = 410;   // Plot width
    const int PLOT_HEIGHT = 240;  // Plot height
    const int SCREEN_WIDTH = 480;
    const int SCREEN_HEIGHT = 320;
    unixtime = timeClient.getEpochTime(); // Get the current UNIX time

    calculateNextPass();

    // Clear Screen
    tft.fillScreen(TFT_BLACK);

    // Draw Axes
    tft.drawRect(PLOT_X, PLOT_Y, PLOT_WIDTH, PLOT_HEIGHT, TFT_WHITE);

    // Draw Azimuth Gridlines and Labels
    int azGridInterval = 30;
    for (int az = 0; az <= 360; az += azGridInterval)
    {
        int y = PLOT_Y + PLOT_HEIGHT - map(az, 0, 360, 0, PLOT_HEIGHT);
        tft.drawLine(PLOT_X, y, PLOT_X + PLOT_WIDTH, y, TFT_DARKGREY);
        if (az % 90 == 0) // Label key azimuths
        {
            tft.setFreeFont(&FreeMono9pt7b);
            tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
            tft.setCursor(2, y - 5);
            tft.printf("%d°", az);
        }
    }

    // Draw Elevation Gridlines and Labels
    int elGridInterval = 15;
    for (int el = 0; el <= 90; el += elGridInterval)
    {
        int y = PLOT_Y + PLOT_HEIGHT - map(el, 0, 90, 0, PLOT_HEIGHT);
        // Draw vertical gridline
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setCursor(PLOT_X + PLOT_WIDTH + 10, y - 5);
        tft.printf("%d°", el);
    }

    // Draw Time Gridlines and Labels
    for (int i = 0; i <= 5; i++)
    {
        int x = PLOT_X + map(i, 0, 5, 0, PLOT_WIDTH);
        unsigned long time = nextPassStart + i * (nextPassEnd - nextPassStart) / 5;
        tft.drawLine(x, PLOT_Y, x, PLOT_Y + PLOT_HEIGHT, TFT_DARKGREY);

        String timeStr = formatTimeOnly(time, true).substring(0, 5);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(x - 28, PLOT_Y + PLOT_HEIGHT + 18);
        tft.print(timeStr);
    }

    // Start plotting Azimuth and Elevation
    unsigned long currentTime = nextPassStart;
    int lastAzX = -1, lastAzY = -1, lastElX = -1, lastElY = -1;
    float lastAzimuth = -1;
    Serial.print("currentTime: ");
    Serial.println(currentTime);
    Serial.print("nextPassStart: ");
    Serial.println(nextPassStart);
    Serial.print("nextPassEnd: ");
    Serial.println(nextPassEnd);
    Serial.print("nextPassEnd - nextPassStart: ");
    Serial.println(nextPassEnd - nextPassStart);
    Serial.print("nextPassEnd - currentTime: ");
    Serial.println(nextPassEnd - currentTime);

    while (currentTime <= nextPassEnd)
    {
        sat.findsat(currentTime); // Update satellite position

        // Stop plotting if elevation goes below 0
        // if (sat.satEl < 0)
        // break;

        // Calculate x position based on time
        int x = PLOT_X + map(currentTime, nextPassStart, nextPassEnd, 0, PLOT_WIDTH);
        int azY = PLOT_Y + PLOT_HEIGHT - map(sat.satAz, 0, 360, 0, PLOT_HEIGHT);
        int elY = PLOT_Y + PLOT_HEIGHT - map(sat.satEl, 0, 90, 0, PLOT_HEIGHT);
        /*
        Serial.print("sat.satAz: ");
        Serial.print(sat.satAz);
        Serial.print("   sat.satEl: ");
        Serial.println(sat.satEl);
    */
        if (lastAzX != -1)
        {
            // Handle azimuth wraparound
            if (lastAzimuth != -1 && abs(sat.satAz - lastAzimuth) > 180)
            {
                if (sat.satAz > lastAzimuth)
                {
                    tft.drawLine(lastAzX, lastAzY, x, PLOT_Y + PLOT_HEIGHT - map(0, 0, 360, 0, PLOT_HEIGHT), TFT_CYAN);
                    tft.drawLine(x, PLOT_Y + PLOT_HEIGHT - map(360, 0, 360, 0, PLOT_HEIGHT), x, azY, TFT_CYAN);
                }
                else
                {
                    tft.drawLine(lastAzX, lastAzY, x, PLOT_Y + PLOT_HEIGHT - map(360, 0, 360, 0, PLOT_HEIGHT), TFT_CYAN);
                    tft.drawLine(x, PLOT_Y + PLOT_HEIGHT - map(0, 0, 360, 0, PLOT_HEIGHT), x, azY, TFT_CYAN);
                }
            }
            else
            {
                tft.drawLine(lastAzX, lastAzY, x, azY, TFT_GREENYELLOW);
            }

            // Draw elevation line (no wraparound needed)
            tft.drawLine(lastElX, lastElY, x, elY, TFT_CYAN);
        }

        // Update for the next iteration
        lastAzimuth = sat.satAz;
        lastAzX = x;
        lastAzY = azY;
        lastElX = x;
        lastElY = elY;

        // Increment time by step size
        currentTime += stepsInSeconds;
    }

    // Display TCA Time
    int tcaX = PLOT_X + map(nextPassCulminationTime, nextPassStart, nextPassEnd, 0, PLOT_WIDTH);
    int tcaY = PLOT_Y + PLOT_HEIGHT - map(nextPassMaxTCA, 0, 90, 0, PLOT_HEIGHT);
    String tcaTimeStr = formatTimeOnly(nextPassCulminationTime, true).substring(0, 5);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(tcaX - 35, tcaY - 8);
    tft.setFreeFont(&FreeMonoBold12pt7b);
    tft.print(tcaTimeStr);
    tft.fillCircle(tcaX, tcaY, 4, TFT_GREEN);

    // Display Pass Duration
    unsigned long duration = nextPassEnd - nextPassStart;
    tft.setCursor(55, 300);
    tft.print("Pass Duration: ");
    tft.print(duration / 60);
    tft.print("m ");
    tft.print(duration % 60);
    tft.println("s");

    // display current position if visible
    unixtime = timeClient.getEpochTime(); // Get the current UNIX time
    sat.findsat(unixtime);
    if (sat.satEl > 0)
    {
        int x = PLOT_X + map(unixtime, nextPassStart, nextPassEnd, 0, PLOT_WIDTH);
        int elY1 = PLOT_Y + PLOT_HEIGHT - map(0, 0, 90, 0, PLOT_HEIGHT);
        int elY2 = PLOT_Y + PLOT_HEIGHT - map(sat.satEl, 0, 90, 0, PLOT_HEIGHT);
        tft.drawLine(x - 1, elY1, x - 1, elY2, TFT_RED);
        tft.drawLine(x, elY1, x, elY2, TFT_RED);
        tft.drawLine(x + 1, elY1, x + 1, elY2, TFT_RED);
        tft.fillCircle(x, elY2, 3, TFT_CYAN);
        tft.drawCircle(x, elY2, 4, TFT_RED);
    }
}

void displayPolarPlotPage()
{
    calculateNextPass();
    getOrbitNumber(nextPassStart);
    // Clear the area to redraw
    tft.fillScreen(TFT_BLACK);
    // Display AOS on TFT screen
    int margin = 5;
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
    tft.print("TCA ");                       // Label for TCA
    tft.print(nextPassMaxTCA, 1);            // 1 specifies the number of decimal places
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
#define POLAR_RADIUS 140   // Maximum radius for the outermost circle

    // Elevations to label and draw circles for
    int elevations[] = {0, 15, 30, 45, 60, 75};

    // Draw concentric circles for elevation markers
    for (int i = 0; i < 6; i++) // Loop through elevations array
    {
        int elevation = elevations[i];

        // Map the elevation to the corresponding radius
        int radius = map(elevation, 0, 90, POLAR_RADIUS, 0); // Corrected mapping

        // Draw the circle for this elevation
        tft.drawCircle(POLAR_CENTER_X, POLAR_CENTER_Y, radius, TFT_LIGHTGREY);

        // Label the elevation on the circle
        int x = POLAR_CENTER_X + radius + 5; // Offset to place labels inside each circle
        int y = POLAR_CENTER_Y - 10;
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(1);
        tft.setCursor(x, y);
        tft.print(elevation); // Display the elevation value
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
    // Adding the "90" label in the center
    // tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Set text color
    tft.setCursor(POLAR_CENTER_X - 5, POLAR_CENTER_Y - 10); // Slightly offset for readability
    tft.print("90");                                        // Print "90" in the center
    // Draw compass labels with correct orientation
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("N", POLAR_CENTER_X, POLAR_CENTER_Y - POLAR_RADIUS - 18, 2);
    tft.drawCentreString("S", POLAR_CENTER_X, POLAR_CENTER_Y + POLAR_RADIUS + 5, 2);
    tft.drawCentreString("E", POLAR_CENTER_X + POLAR_RADIUS + 13, POLAR_CENTER_Y, 2);
    tft.drawCentreString("W", POLAR_CENTER_X - POLAR_RADIUS - 10, POLAR_CENTER_Y, 2);

    // Plot the satellite pass path with color dots for AOS, max elevation, and LOS
    int lastX = -1, lastY = -1;
    // Time step for pass prediction
    int timeStep = 1;
    bool AOSdrawm = false;
    int x = 0;
    int y = 0;
    for (unsigned long t = nextPassStart - 30; t <= nextPassEnd + 30; t += timeStep) // just adding some  second 'margin'
    {
        sat.findsat(t);
        float azimuth = sat.satAz;
        float elevation = sat.satEl;
        /*
        Serial.print("azimuth = ");
        Serial.print(azimuth);
        Serial.print("  elevation = ");
        Serial.println(elevation);
        */
        // Serial.printf("Time: %lu | Azimuth: %.2f | Elevation: %.2f\n", t, azimuth, elevation);

        if (elevation >= 0)
        {
            int radius = map(90 - elevation, 0, 90, 0, POLAR_RADIUS);
            float radianAzimuth = radians(azimuth);
            x = POLAR_CENTER_X + radius * sin(radianAzimuth);
            y = POLAR_CENTER_Y - radius * cos(radianAzimuth);

            // Serial.printf("Plotted Point -> x: %d, y: %d\n", x, y);

            if (elevation > 0 && AOSdrawm == false)
            {
                tft.fillCircle(x, y, 3, TFT_GREEN); // Green dot for AOS
                AOSdrawm = true;
            }
            if (t == nextPassCulminationTime)
            {
                tft.fillCircle(x, y, 3, TFT_YELLOW); // Yellow dot for max elevation
                                                     // Serial.println("Plotted TCA (Yellow)");
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
    tft.fillCircle(x, y, 3, TFT_RED); // Red dot for LOS

    // IF VISIBLE
    // display current position if visible
    unixtime = timeClient.getEpochTime(); // Get the current UNIX time
    sat.findsat(unixtime);
    if (sat.satEl > 0)
    {
        float azimuth = sat.satAz;
        float elevation = sat.satEl;
        int radius = map(90 - elevation, 0, 90, 0, POLAR_RADIUS);
        float radianAzimuth = radians(azimuth);

        x = POLAR_CENTER_X + radius * sin(radianAzimuth);
        y = POLAR_CENTER_Y - radius * cos(radianAzimuth);

        tft.fillCircle(x, y, 3, TFT_CYAN);
        tft.drawCircle(x, y, 4, TFT_RED);
        tft.drawLine(POLAR_CENTER_X, POLAR_CENTER_Y, x, y, TFT_CYAN);
    }
}

void updateBigClock(bool refresh = false)
{
    if (SEVEN_DIGIT_STYLE == true)
    {
        display7segmentClock(unixtime + timezoneOffset + dstOffset, 26, 92, SEVEN_DIGIT_COLOR, refresh);
        return;
    }
    int y = 0;
    int color = TFT_GOLD;
    tft.setTextFont(8);
    tft.setTextSize(1);
    static String previousTime = ""; // Track previous time to update only changed characters

    static bool isPositionCalculated = false;
    static int clockXPosition; // Calculated once to center the clock text
    static int clockWidth;     // Width of the time string in pixels

    // Perform initial calculation of clock width and position if not already done
    if (!isPositionCalculated || refresh == true)
    {
        String sampleTime = "00:00:00"; // Sample time format for clock width calculation
        clockWidth = tft.textWidth(sampleTime.c_str());
        clockXPosition = (tft.width() - clockWidth) / 2; // Center x position for the clock
        isPositionCalculated = true;                     // Mark as calculated
        previousTime = "";
    }

    // Get current UTC time in seconds
    unsigned long utcTime = timeClient.getEpochTime();

    // Apply timezone and DST offsets
    unsigned long localTime = utcTime + timezoneOffset + dstOffset; // NOT CLEAR YET XXXXX
    Serial.print(timezoneOffset);
    Serial.print("   ");
    Serial.print(dstOffset);

    // Convert to human-readable format
    struct tm *timeinfo = gmtime((time_t *)&localTime); // Use gmtime for seconds since epoch
    char timeStr[25];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

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

    // Dynamically fill the output array from the right
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

    // Dynamically fill the output array from the right
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

    // Dynamically fill the output array from the right
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

    // Dynamically fill the output array from the right
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
        tft.print(String(TLEageHHMM));
        isInitiated = true;
    }
}

void pngDraw(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[480];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    tft.pushImage(0, 0 + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

/*
void displaySplashScreen(int duration)
{
  digitalWrite(TFT_BLP, LOW);

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

  digitalWrite(TFT_BLP, HIGH);

  delay(duration);
}
*/

void displaySplashScreen(int duration)
{
    digitalWrite(TFT_BLP, LOW);

    // https://notisrac.github.io/FileToCArray/
    int16_t rc = png.openFLASH((uint8_t *)fancySplash, sizeof(fancySplash), pngDraw);

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

    digitalWrite(TFT_BLP, HIGH);

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
/*
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
*/
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

void displayMapWithMultiPasses()
{
    // Constants for map scaling and placement
    const int mapWidth = 480;  // Width of the map
    const int mapHeight = 242; // Height of the map
    const int mapOffsetY = 40; // Y-offset for the map (black banner)
    const int timeStep = 15;   // Time step for plotting points

    // Clear the screen and display the map image
    tft.fillScreen(TFT_BLACK);
    displayEquirectangularWorlsMap();

    // STEP 1: Get satellite position and draw the footprint
    sat.findsat(unixtime);
    float startLat = sat.satLat; // Satellite latitude
    float startLon = sat.satLon; // Satellite longitude
    float satAlt = sat.satAlt;   // Satellite altitude

    // Calculate footprint radius in kilometers
    float earthRadius = 6371.0; // Earth's radius in kilometers
    float footprintRadiusKm = earthRadius * acos(earthRadius / (earthRadius + satAlt));

    // Debug: Print footprint radius
    Serial.print("Footprint radius (km): ");
    Serial.println(footprintRadiusKm);

    // Draw the footprint as an ellipse
    for (int angle = 0; angle < 360; angle++)
    {
        float rad = angle * DEG_TO_RAD; // Convert angle to radians

        // Calculate latitude and longitude for each point on the footprint
        float deltaLat = footprintRadiusKm * cos(rad) / 111.0;                                // Latitude adjustment (1° ≈ 111 km)
        float deltaLon = footprintRadiusKm * sin(rad) / (111.0 * cos(startLat * DEG_TO_RAD)); // Longitude adjustment

        float footprintLat = startLat + deltaLat; // New latitude for the footprint point
        float footprintLon = startLon + deltaLon; // New longitude for the footprint point

        // Longitude wrapping to keep within -180° to 180°
        if (footprintLon > 180.0)
            footprintLon -= 360.0;
        if (footprintLon < -180.0)
            footprintLon += 360.0;

        // Map latitude and longitude to screen coordinates
        int x = map(footprintLon, -180, 180, 0, mapWidth);             // Longitude to X
        int y = map(footprintLat, 90, -90, 0, mapHeight) + mapOffsetY; // Latitude to Y with offset

        // Draw footprint point if within screen bounds
        if (x >= 0 && x < mapWidth && y >= mapOffsetY && y < mapHeight + mapOffsetY)
        {
            tft.fillCircle(x, y, 1, TFT_GOLD);
        }
    }

    // STEP 2: Plot the starting position
    int startX = map(startLon, -180, 180, 0, mapWidth);             // Longitude to X-coordinate
    int startY = map(startLat, 90, -90, 0, mapHeight) + mapOffsetY; // Latitude to Y-coordinate with offset

    // Mark the starting position
    tft.fillCircle(startX, startY, 3, TFT_YELLOW); // Starting point
    tft.drawCircle(startX, startY, 4, TFT_RED);
    tft.drawCircle(startX, startY, 5, TFT_RED);

    // Debug: Print starting position
    Serial.print("Starting Point (X, Y): ");
    Serial.print(startX);
    Serial.print(", ");
    Serial.println(startY);

    // STEP 3: Plot the satellite's path for three orbits
    unsigned long t = unixtime;
    int passageCount = 0;
    bool hasLeftStartX = false;

    while (passageCount < 3)
    {
        sat.findsat(t);
        float lat = sat.satLat;
        float lon = sat.satLon;

        // Map latitude and longitude to screen coordinates
        int x = map(lon, -180, 180, 0, mapWidth);             // Longitude to X
        int y = map(lat, 90, -90, 0, mapHeight) + mapOffsetY; // Latitude to Y with offset

        // Choose color based on passage count
        uint16_t color = (passageCount == 0) ? TFT_GREEN : (passageCount == 1) ? TFT_YELLOW
                                                                               : TFT_RED;

        // Draw the satellite's path
        tft.fillCircle(x, y, 1, color);

        // Check if the satellite moved away from the starting X position
        if (!hasLeftStartX && abs(x - startX) > 20)
        {
            hasLeftStartX = true;
        }

        // Check if the satellite returned close to the starting X position
        if (hasLeftStartX && abs(x - startX) < 5)
        {
            passageCount++;
            hasLeftStartX = false;
        }

        t += timeStep; // Increment time step
    }
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
    int margin = 12;
    // Draw headers
    tft.setCursor(margin, 0);
    tft.print("DATE");
    tft.setCursor(margin + 85, 0);
    tft.print("AOS");
    tft.setCursor(margin + 164, 0);
    tft.print("TCA");
    tft.setCursor(margin + 244, 0);
    tft.print("LOS");
    tft.setCursor(margin + 324, 0);
    tft.print("DUR");
    tft.setCursor(margin + 400, 0);
    tft.print("MEL");

    // Adjust timezone and DST offset
    long adjustedTimezoneOffset = timezoneOffset + dstOffset; // Correctly adjust for DST and timezone offset

    // Iterate through the next 10 passes
    for (int i = 1; i <= 12; i++) // Loop for next 12 passes (as per your original code)
    {
        bool passFound = sat.nextpass(&overpass, 100);
        if (passFound)
        {
            // Prepare persistent struct tm objects for AOS, TCA, and LOS
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
            tft.setCursor(margin, yPosition);
            tft.printf("%s", passDate);
            tft.setCursor(margin + 80, yPosition);
            tft.printf("%s", aosTime);
            tft.setCursor(margin + 160, yPosition);
            tft.printf("%s", tcaTime);
            tft.setCursor(margin + 240, yPosition);
            tft.printf("%s", losTime);
            tft.setCursor(margin + 320, yPosition);
            tft.printf("%s", durationFormatted);
            tft.setCursor(margin + 400, yPosition);
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
        // Serial.print(output[i]);
        // Serial.print(" ");
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
        // Serial.print(output[i]);
        // Serial.print(" ");
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
        // Serial.print(output[i]);
        // Serial.print(" ");
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
    pinMode(TFT_BLP, OUTPUT); // for TFT backlight
    digitalWrite(TFT_BLP, HIGH);

    // clearPreferences();// uncomment for testing
    Serial.begin(115200);
    if (DEBUG_ON_TFT == true)
    {
        bootingMessagePause = 3000;
    }
    Serial.println("Sketch Size: " + String(ESP.getSketchSize()) + " bytes");
    Serial.println("Free Sketch Space: " + String(ESP.getFreeSketchSpace()) + " bytes");
    Serial.println("Flash Chip Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
    Serial.println("Flash Frequency: " + String(ESP.getFlashChipSpeed() / 1000000) + " MHz");

    initializeTFT();

    displaySplashScreen(3000);
    // displayBlueMapimage(); Alternative Map
    delay(800); // image fully loaded

    displayWelcomeMessage();
    delay(bootingMessagePause);
    connectToWiFi();
    delay(bootingMessagePause);
    getTimezoneData();
    delay(bootingMessagePause);
    syncTimeFromNTP();
    delay(bootingMessagePause);

    computeSun();

    if (refreshTLEelements(true))
    {
        Serial.println("TLE refresh during setup completed.");
    }
    else
    {
        Serial.println("TLE refresh during setup failed or no update needed.");
    }

    delay(bootingMessagePause);
    getTLEelements(tleLine1, tleLine2, true);

    sat.init("ISS (ZARYA)", tleLine1, tleLine2);
    sat.site(OBSERVER_LATITUDE, OBSERVER_LONGITUDE, OBSERVER_ALTITUDE);

    delay(bootingMessagePause);

    digitalWrite(TFT_BLP, HIGH);
    /*
     displayTableNext10Passes();
     displayMapWithMultiPasses();
     displayPolarPlotPage();
     displayAzElPlotPage();
     displayPExpedition72image();
   */
    tft.fillScreen(TFT_BLACK);

    displayMainPage();
    lastTLEUpdate = millis(); // Record the time of the first update
}

void loop()
{

    // Check if it's time to update the TLE
    if (millis() - lastTLEUpdate >= updateInterval)
    {
        Serial.println("\nRunning hourly TLE refresh...");
        if (refreshTLEelements(false))
        {
            Serial.println("TLE refresh completed.");
            Serial.println("TLE refresh completed.");
            getTLEelements(tleLine1, tleLine2, false);

            sat.init("ISS (ZARYA)", tleLine1, tleLine2);
        }
        else
        {
            Serial.println("TLE refresh failed or no update needed.");
        }
        lastTLEUpdate = millis(); // Update the last refresh time
    }

    // Get the current touch pressure
    touchTFT = tft.getTouchRawZ();
    // Serial.println(touchCounter);

    if (touchCounter == 2) // AZel Plot
    {
        // Serial.println(sat.satEl);
        //  Check if 5 seconds (5000 ms) have passed since the last refresh
        if (millis() - AzElPlotlastRefreshTime >= 15000)
        {
            displayAzElPlotPage();              // Refresh the display
            AzElPlotlastRefreshTime = millis(); // Update the last refresh time
        }
    }

    if (touchCounter == 3) // Polar Plot
    {
        // Check if 5 seconds (5000 ms) have passed since the last refresh
        if (millis() - PolarPlotlastRefreshTime >= 15000)
        {
            displayPolarPlotPage();              // Refresh the display
            PolarPlotlastRefreshTime = millis(); // Update the last refresh time
        }
    }

    // Refresh logic for case 5 (displayMapWithMultiPasses) outside touch handling
    if (touchCounter == 5)
    {
        // Check if 5 seconds (5000 ms) have passed since the last refresh
        if (millis() - multipassMaplastRefreshTime >= 5000)
        {
            displayMapWithMultiPasses();            // Refresh the display
            multipassMaplastRefreshTime = millis(); // Update the last refresh time
        }
    }

    // Check if the touch pressure exceeds the threshold and debounce
    if (touchTFT > touchTreshold)
    {
        // Only increment the counter if enough time has passed since the last touch
        if (millis() - lastTouchTime > debounceDelay)
        {
            touchCounter++; // Increment the counter
            // If counter exceeds 6, reset it to 1

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
            // Serial.print("Counter: ");
            // Serial.println(touchCounter);

            // Call the respective functions based on the counter value
            switch (touchCounter)
            {
            case 1:
                if (!page1Displayed)
                {
                    tft.fillScreen(TFT_BLACK);
                    refresh = true;
                    updateBigClock(true);

                    page1Displayed = true; // Set the flag to prevent re-display
                }
                displayMainPage(); // Show page 1
                break;
            case 2:
                if (!page2Displayed)
                {
                    displayAzElPlotPage();
                    AzElPlotlastRefreshTime = millis();
                    page2Displayed = true; // Set the flag to prevent re-display
                }
                break;
            case 3:

                if (!page3Displayed)
                {
                    displayPolarPlotPage();
                    page3Displayed = true; // Set the flag to prevent re-display
                    PolarPlotlastRefreshTime = millis();
                }
                break;
            case 4:
                if (!page4Displayed)
                {
                    displayTableNext10Passes(); // Show page 3
                    page4Displayed = true;      // Set the flag to prevent re-display
                }
                break;
            case 5:
                if (!page5Displayed)
                {
                    displayMapWithMultiPasses(); // Show page 4
                    page5Displayed = true;       // Set the flag to prevent re-display
                    multipassMaplastRefreshTime = millis();
                }
                break;

            case 6:
                if (!page6Displayed)
                {
                    if (DISPLAY_ISS_CREW == true)
                    {
                        displayPExpedition72image();
                        page6Displayed = true; // Set the flag to prevent re-display
                    }
                    else
                    {
                        touchCounter = 7;
                        tft.fillScreen(TFT_BLACK);
                        refresh = true;
                        displayMainPage(); // Show page 1
                    }
                }
                break;
            }
            lastTouchTime = millis(); // Update the time of the last touch
        }
    }

    static unsigned long lastLoopTime = millis();
    if (millis() - lastLoopTime >= 1000 && touchCounter == 1)
    {
        displayMainPage();
        //  Update the last loop time
        lastLoopTime = millis();
    }
}

void displayMainPage()
{
    // Update the time from NTP
    timeClient.update();
    unixtime = timeClient.getEpochTime(); // Get the current UNIX timestamp
                                          // for debugging
    int deltaHour = 0;
    int deltaMin = 0;
    unixtime = unixtime + deltaHour * 3600 + deltaMin * 60;
    getOrbitNumber(unixtime);
    calculateNextPass();

    updateBigClock(refresh);

    // Update the satellite data

    sat.findsat(unixtime);

    int AZELcolor;
    if (sat.satEl > 3)
    {
        AZELcolor = TFT_GREEN; // Elevation greater than 3 -> Green
    }
    else if (sat.satEl < -3)
    {
        AZELcolor = TFT_RED; // Elevation less than -3 -> Red
    }
    else
    {
        AZELcolor = TFT_YELLOW; // Elevation between -3 and 3 -> Yellow
    }

    displayElevation(sat.satEl, 5 + 30, 116, AZELcolor, refresh);
    displayAzimuth(sat.satAz, 303 - 30, 116, AZELcolor, refresh);

    int startXmain = 30;
    int startYmain = 200;
    int deltaY = 30;

    displayAltitude(sat.satAlt, 25, startYmain, TFT_GOLD, refresh);
    displayDistance(sat.satDist, 25, startYmain + 1 * deltaY, TFT_GOLD, refresh);
    displayOrbitNumber(orbitNumber, 25, startYmain + 2 * deltaY, TFT_GOLD, refresh);
    displayLatitude(sat.satLat, 320, startYmain, TFT_GOLD, refresh);
    displayLongitude(sat.satLon, 320, startYmain + deltaY, TFT_GOLD, refresh);
    displayLTLEage(startYmain + 2 * deltaY, refresh);

    int lowerBannerY = 295;

    // Managing the bottom banner
    // XXXXXXX
    if (sat.satEl < 0)
    {
        int shifting = 50;
        if (first_time_below == true || refresh == true) //        first_time_above = true;
        {
            tft.fillRect(0, 295, 480, 50, TFT_BLACK); // clear entire area
            tft.setCursor(shifting, lowerBannerY);
            tft.setTextColor(TFT_CYAN);
            tft.print("Next Pass in ");
            tft.setCursor(tft.textWidth("Next pass in 00:00:00 ") + shifting, lowerBannerY);
            tft.print("at ");
            tft.print(formatTimeOnly(nextPassStart, true));
            displayNextPassTime(nextPassStart - unixtime, shifting, lowerBannerY, TFT_CYAN, refresh);
            first_time_below = false;
        }
        // tft.fillRect(0, 295, 480, 50, TFT_BLACK);
        tft.setCursor(shifting, lowerBannerY);
        // tft.fillRect(0, 295, 480, 50, TFT_BLACK);
        displayNextPassTime(nextPassStart - unixtime, shifting, lowerBannerY, TFT_CYAN, refresh);
        first_time_above = true;
        refresh = false;
    }

    if (sat.satEl > 0)
    {
        int shifting = 40;
        if (first_time_above == true || refresh == true) //        first_time_above = true;
        {
            tft.fillRect(0, 295, 480, 50, TFT_BLACK);
            tft.setCursor(shifting, lowerBannerY);
            tft.setTextColor(TFT_CYAN);
            tft.print("Satellite is above horizon for");
            tft.setCursor(shifting, lowerBannerY);
            // displayNextPassTime(nextPassEnd - unixtime, shifting, lowerBannerY, TFT_CYAN, refresh);
            first_time_above = false;
        }
        // tft.fillRect(0, 295, 480, 50, TFT_BLACK);
        // tft.setCursor(400, lowerBannerY);
        
        int tmpX=tft.textWidth("Satellite is above horizon for ")+shifting;
        tft.fillRect(tmpX, 295, 480-shifting, 50, TFT_BLACK);
        tft.setCursor(tmpX, lowerBannerY);
        tft.setTextColor(TFT_CYAN);
        tft.print(displayRemainingVisibleTimeinMMSS(nextPassEnd - unixtime));

        // tft.fillRect(0, 295, 480, 50, TFT_BLACK);
        // displayNextPassTime(nextPassEnd - unixtime, shifting, lowerBannerY, TFT_CYAN, refresh);
        first_time_below = true;
        refresh = false;
    }

  

    /*
            else
            {

                tft.setCursor(shifting, lowerBannerY);
                tft.fillRect(0, 295, 480, 50, TFT_BLACK);
                first_time_below = false;
                Serial.print("Do nothing");

                unsigned long timeRemaining = nextPassEnd - unixtime; // Calculate the difference
                displayRemainingPassTime(timeRemaining, 60, lowerBannerY, TFT_CYAN, refresh);
            }

            /*
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

        */
}

void displayRemainingPassTime(unsigned long durationInSec, int x, int y, uint16_t color, bool refresh)
{
    tft.setTextSize(1);
    tft.setTextFont(4);

    static char previousOutput[9] = "        "; // Previous state (8 characters + null terminator) "00:00:00"
    static uint16_t previousColor = TFT_GREEN;  // Track the last color used
    static bool isInitiated = false;            // Track initialization of static elements
    char output[9] = "        ";                // Current output (8 characters + null terminator)

    // Calculate hours, minutes, and seconds from durationInSec
    unsigned long hours = durationInSec / 3600;          // Get the number of full hours
    unsigned long minutes = (durationInSec % 3600) / 60; // Get the number of full minutes
    unsigned long seconds = durationInSec % 60;          // Get the remaining seconds

    // Fill the output array with the formatted time "HH:MM:SS"
    sprintf(output, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    Serial.println(output);
    // Positions for characters
    String preText = "ISS above horizon for ";
    int lPreText = preText.length();
    int shift = 6;
    int xPos[] = {0, 14, 28, 42 - shift, 56 - shift, 70 - shift, 84 - 2 * shift, 98 - 2 * shift};
    //  1   2   :    4          5          :         7            8

    int leftmargin = tft.textWidth(preText);

    for (int i = 0; i < 8; i++)
        xPos[i] += x + leftmargin;

    // Handle color change or refresh logic
    if (color != previousColor)
    {
        // If the color has changed, trigger a full redraw
        refresh = true;
        previousColor = color; // Update the last used color
    }

    // Handle refresh logic for static elements (if needed)
    if (!isInitiated || refresh)
    {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawString(preText, x, y);
        isInitiated = true;
    }

    // Update only changed characters or redraw all if refresh is triggered
    for (int i = 0; i < 8; i++)
    {
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

void displayNextPassTime(unsigned long durationInSec, int x, int y, uint16_t color, bool refresh)
{
    tft.setTextSize(1);
    tft.setTextFont(4);

    static char previousOutput[9] = "        "; // Previous state (8 characters + null terminator) "00:00:00"
    static uint16_t previousColor = TFT_GREEN;  // Track the last color used
    static bool isInitiated = false;            // Track initialization of static elements
    char output[9] = "        ";                // Current output (8 characters + null terminator)

    // Calculate hours, minutes, and seconds from durationInSec

    unsigned long hours = durationInSec / 3600;          // Get the number of full hours
    unsigned long minutes = (durationInSec % 3600) / 60; // Get the number of full minutes
    unsigned long seconds = durationInSec % 60;          // Get the remaining seconds

    // Fill the output array with the formatted time "HH:MM:SS"
    sprintf(output, "%02lu:%02lu:%02lu", hours, minutes, seconds);

    // Positions for characters
    String preText = "Next pass in ";
    int lPreText = preText.length(); // XXXXX
    int shift = 7;
    int xPos[] = {0, 14, 28, 42 - shift, 56 - shift, 70 - shift, 84 - 2 * shift, 98 - 2 * shift};
    //  1   2   :    4          5          :         7            8

    int leftmargin = tft.textWidth(preText);

    for (int i = 0; i < 8; i++)
        xPos[i] += x + leftmargin;

    // Handle color change or refresh logic
    if (color != previousColor)
    {
        // If the color has changed, trigger a full redraw
        refresh = true;
        previousColor = color; // Update the last used color
    }

    // Handle refresh logic for static elements (if needed)
    if (!isInitiated || refresh)
    {
        tft.setTextColor(color, TFT_BLACK);
        tft.drawString(preText, x, y);
        isInitiated = true;
    }

    // Update only changed characters or redraw all if refresh is triggered
    for (int i = 0; i < 8; i++)
    {
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

String displayRemainingVisibleTimeinMMSS(int delta) {
    // Ensure delta does not exceed 60 minutes
    delta = constrain(delta, 0, 3600);

    // Calculate minutes and seconds
    int minutes = delta / 60;
    int seconds = delta % 60;

    // Format as MM:SS
    char buffer[6]; // Buffer to hold the formatted string
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);

    return String(buffer); // Return as String
}


void computeSun()
{
    timeClient.update();                       // Attempt to update the time from the NTP server
    long unixtime = timeClient.getEpochTime(); // Get the current UNIX time

    // Date calculation from UNIX timestamp
    struct tm *timeinfo;
    time_t rawtime = unixtime;
    timeinfo = localtime(&rawtime);

    int year = timeinfo->tm_year + 1900; // tm_year is years since 1900
    int month = timeinfo->tm_mon + 1;    // tm_mon is months since January (0-11)
    int day = timeinfo->tm_mday;

    // Sun position calculation
    double transit, sunrise, sunset, zenith;
    int height = 200; // Height in meters
    double sun_altitude = SUNRISESET_STD_ALTITUDE - 0.0353 * sqrt(height);

    // Calculate the times of sunrise, transit, and sunset using SolarCalculator's calcSunriseSunset
    calcSunriseSunset(year, month, day, OBSERVER_LATITUDE, OBSERVER_LONGITUDE, transit, sunrise, sunset, sun_altitude);

    // Convert sunrise and sunset from hours to seconds (multiply by 3600)
    long sunrise_seconds = sunrise * 3600;
    long sunset_seconds = sunset * 3600;

    // Apply timezone offset (in seconds)
    sunrise_seconds += timezoneOffset;
    sunset_seconds += timezoneOffset;

    // Zenith is the position of the sun when it's directly overhead
    zenith = 90.0 - (sun_altitude + 0.0353 * sqrt(height)); // Approximation based on altitude

    // Calculate the duration of the day (difference between sunset and sunrise)
    double dayDuration = (sunset_seconds - sunrise_seconds) / 3600.0;      // Convert to hours
    int dayDuration_hr = int(dayDuration);                                 // Whole number of hours
    int dayDuration_min = int(round((dayDuration - dayDuration_hr) * 60)); // Remaining minutes

    // Print the results in HH:mm format
    char str[6];

    // Sunrise time conversion to HH:mm format
    int sunrise_min = int(round(sunrise * 60));
    int sunrise_hr = (sunrise_min / 60) % 24;
    int sunrise_mn = sunrise_min % 60;
    snprintf(str, sizeof(str), "%02d:%02d", sunrise_hr, sunrise_mn);
    Serial.print("Sunrise: ");
    Serial.println(str);

    // Transit time conversion to HH:mm format
    int transit_min = int(round(transit * 60));
    int transit_hr = (transit_min / 60) % 24;
    int transit_mn = transit_min % 60;
    snprintf(str, sizeof(str), "%02d:%02d", transit_hr, transit_mn);
    Serial.print("Transit: ");
    Serial.println(str);

    // Sunset time conversion to HH:mm format
    int sunset_min = int(round(sunset * 60));
    int sunset_hr = (sunset_min / 60) % 24;
    int sunset_mn = sunset_min % 60;
    snprintf(str, sizeof(str), "%02d:%02d", sunset_hr, sunset_mn);
    Serial.print("Sunset: ");
    Serial.println(str);

    // Print the day duration
    snprintf(str, sizeof(str), "%02d:%02d", dayDuration_hr, dayDuration_min);
    Serial.print("Day Duration: ");
    Serial.println(str);

    // Print zenith value (sun's position when directly overhead)
    Serial.print("Zenith: ");
    Serial.println(zenith);
}

void display7segmentClock(unsigned long unixTime, int xOffset, int yOffset, uint16_t textColor, bool refresh)
{
    // Static variables to track previous state and colon visibility
    static int previousArray[6] = {-1, -1, -1, -1, -1, -1}; // Initialize previous digit array

    Serial.print(refresh);
    if (refresh == true)
    {
        for (int i = 0; i < 6; i++)
        {
            previousArray[i] = -1;
        }
        refresh = false;
    }
    static bool colonVisible = true;     // Tracks colon visibility
                                         // Define the TFT_MIDGREY color as a local constant
    const uint16_t TFT_MIDGREY = 0x39a7; // Darker grey https://rgbcolorpicker.com/565
    // uint16_t TFT_MIDGREY = TFT_DARKGREY;
    int gap = 68;
    int gap2 = 20;
    int xCoordinates[6] = {xOffset, xOffset + gap, xOffset + 2 * gap + gap2, xOffset + 3 * gap + gap2, xOffset + 4 * gap + 2 * gap2, xOffset + 5 * gap + 2 * gap2};

    // Set the custom font
    tft.setFreeFont(&HB9IIU7segFonts);

    // Toggle colon visibility every second
    if (unixTime % 1 == 0)
    {
        colonVisible = !colonVisible;
    }

    // Display or hide colons based on colonVisible
    uint16_t colonColor = colonVisible ? textColor : TFT_BLACK;
    tft.setTextColor(colonColor, TFT_BLACK);
    tft.setCursor(xCoordinates[2] - 24, yOffset);
    tft.print(":");
    tft.setCursor(xCoordinates[4] - 24, yOffset);
    tft.print(":");

    // Calculate hours, minutes, and seconds
    int hours = (unixTime % 86400L) / 3600; // Hours since midnight XXXX
    int minutes = (unixTime % 3600) / 60;   // Minutes
    int seconds = unixTime % 60;            // Seconds

    // Current time digit array
    int timeArray[6] = {
        hours / 10,   // Tens digit of hours
        hours % 10,   // Units digit of hours
        minutes / 10, // Tens digit of minutes
        minutes % 10, // Units digit of minutes
        seconds / 10, // Tens digit of seconds
        seconds % 10  // Units digit of seconds
    };

    // Mapped characters for 0-9
    char mappedChars[10] = {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};

    // Update only changed digits
    for (int i = 0; i < 6; i++)
    {
        if (timeArray[i] != previousArray[i])
        {
            // Clear the previous digit
            tft.setTextColor(TFT_BLACK, TFT_BLACK);
            tft.setCursor(xCoordinates[i], yOffset);
            tft.print(previousArray[i]);

            // Print the new digit
            tft.setTextColor(textColor, TFT_BLACK);
            tft.setCursor(xCoordinates[i], yOffset);
            tft.print(timeArray[i]);

            // Print the mapped character below the digit, but skip if the mapped character is 'H'
            tft.setTextColor(TFT_MIDGREY, TFT_BLACK);

            char mappedChar = mappedChars[timeArray[i]]; // Get the mapped character
            if (mappedChar != 'H')
            {
                tft.setCursor(xCoordinates[i], yOffset); // Adjust Y offset for character display
                tft.print(mappedChar);
            }
        }
    }

    // Update the previous array
    for (int i = 0; i < 6; i++)
    {
        previousArray[i] = timeArray[i];
    }
}
