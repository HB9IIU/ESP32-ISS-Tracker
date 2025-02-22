#include <WiFi.h>
#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BluetoothSerial SerialBT;
BLEScan* pBLEScan;
int scanTime = 5;  // BLE scan duration in seconds
const int ledPin = 2;  // Pin for the onboard LED (typically GPIO 2)

// Callback for BLE scan
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found BLE device: ");
    Serial.print(advertisedDevice.getName().c_str());
    Serial.print(" - Address: ");
    Serial.println(advertisedDevice.getAddress().toString().c_str());
  }
};

// Display device information
void displayDeviceInfo() {
  Serial.println("ESP32 Device Info:");
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Chip Revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("CPU Frequency: ");
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print("Flash Size: ");
  Serial.println(ESP.getFlashChipSize() / (1024 * 1024));  // in MB
  Serial.print("Free Heap Memory: ");
  Serial.println(ESP.getFreeHeap());
}

// LED blink test
void ledBlinkTest() {
  for (int i = 1; i <= 5; i++) {
    Serial.print("LED Blink Count: ");
    Serial.println(i);

    digitalWrite(ledPin, HIGH);   // Turn the LED on
    delay(500);                   // Wait for 500ms
    digitalWrite(ledPin, LOW);    // Turn the LED off
    delay(500);                   // Wait for 500ms
  }
}
// Memory usage test
void memoryUsageTest() {
  Serial.print("Free heap memory: ");
  Serial.println(ESP.getFreeHeap());
}

// WiFi stability test
void wifiStabilityTest(const char* ssid, const char* password) {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    delay(5000);  // Hold connection for a while
    WiFi.disconnect();
    Serial.println("Disconnected from WiFi.");
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

// Bluetooth stress test
void bluetoothStressTest() {
  if (SerialBT.hasClient()) {
    Serial.println("Bluetooth Client Connected.");
    SerialBT.end();
    Serial.println("Bluetooth Disconnected.");
  } else {
    SerialBT.begin("ESP32_TestDevice");
    Serial.println("Bluetooth Started.");
  }
  delay(5000);  // Wait before switching Bluetooth state
}

// Heap memory fragmentation test
void heapFragmentationTest() {
  Serial.println("Allocating and deallocating memory...");
  char* buffer = (char*)malloc(1000);  // Allocate 1KB
  if (buffer) {
    strcpy(buffer, "Test memory allocation.");
    Serial.println(buffer);
    free(buffer);  // Free memory
  }
  Serial.print("Free heap memory after test: ");
  Serial.println(ESP.getFreeHeap());
}

// Test task for task creation/deletion stress test
void testTask(void* pvParameters) {
  for (int i = 0; i < 10; i++) {
    Serial.println("Running task...");
    delay(500);
  }
  vTaskDelete(NULL);  // Delete the task when done
}

// Task creation/deletion stress test
void createAndDeleteTaskTest() {
  Serial.println("Creating a new task...");
  xTaskCreate(testTask, "Test Task", 1024, NULL, 1, NULL);
  delay(2000);  // Wait before creating the next task
}

// HTTP request loop for network request stress test
void httpRequestTest(const char* ssid, const char* password, const char* url) {
  static bool connected = false;
  if (!connected) {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    connected = true;
  }

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("HTTP GET... Code: %d\n", httpCode);
  } else {
    Serial.printf("HTTP GET... Failed: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  delay(2000);  // Wait before the next request
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }  // Wait for Serial to be ready (only needed for some environments like USB Serial)

  // Set the onboard LED as output
  pinMode(ledPin, OUTPUT);

  // Display device information
  displayDeviceInfo();

  // Initialize Bluetooth Serial
  if (!SerialBT.begin("ESP32_BT_Scanner")) {  // Set the device name for Bluetooth
    Serial.println("An error occurred while starting Bluetooth!");
    return;
  }
  Serial.println("Bluetooth Started. You can now connect with other devices.");

  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // Create BLE scan object
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);    // Enable active scan for better results
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  // Run all stress tests
  ledBlinkTest();
  memoryUsageTest();
  wifiStabilityTest("NO WIFI FOR YOU!!!", "Nestle2010Nestle");
  bluetoothStressTest();
  heapFragmentationTest();
  createAndDeleteTaskTest();
  httpRequestTest("NO WIFI FOR YOU!!!", "Nestle2010Nestle", "http://example.com");

  // BLE scan
  Serial.println("Starting BLE scan...");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("BLE devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("BLE scan complete.");

  delay(2000);  // Adjust delay as needed for observing results
}
