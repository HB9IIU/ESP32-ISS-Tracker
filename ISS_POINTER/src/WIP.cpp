#include "config.h" // Include the configuration header

#include <Arduino.h>
#include <Stepper.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

unsigned long lastMoveTime = 0;
unsigned long lastIamAliveTime = 0;
const int moveInterval = 1000; // Adjust this for smooth motor updates
const int IamAliveInterval=20000; // to move the arrow whent at home position to signal readiness
const int stepsPerRevolution = 2048;     // Adjust this according to your stepper motor's specs
int azimuthStepperCurrentPosition = 0;   // Variable to track the current azimuth position
int elevationStepperCurrentPosition = 0; // Variable to track the current elevation position

int pointerAzimuth;
int pointerElevation;

// ULN2003 Motor Driver Pins for Azimuth
#define IN1azimuth 19 //
#define IN2azimuth 18 //
#define IN3azimuth 23 //
#define IN4azimuth 22 //

#define IN1elevation 12 // en realité IN2
#define IN2elevation 13 // en realité IN1
#define IN3elevation 2  // ok
#define IN4elevation 4  // ok

// WebSocket client
WebSocketsClient webSocket;

// JSON buffer size
const size_t bufferSize = 128;
DynamicJsonDocument doc(bufferSize); // Using DynamicJsonDocument

// Retry mechanism
const int maxRetryAttempts = 50; // Maximum number of retry attempts
int currentRetryAttempt = 0;     // Current retry attempt count
const int retryInterval = 5000;  // Retry interval in milliseconds (5 seconds)

// Function prototypes
void connectToWiFi(int maxTries);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void handleWebSocketMessage(const char *payload);
void connectWebSocket();
void retryWebSocketConnection();

// Initialize the stepper library
Stepper AzimuthStepper(stepsPerRevolution, IN1azimuth, IN3azimuth, IN2azimuth, IN4azimuth);
Stepper ElevationStepper(stepsPerRevolution, IN1elevation, IN3elevation, IN2elevation, IN4elevation);

void AzimuthStepperGotoAngle(int angle);
void ElevationStepperGotoAngle(int angle);
void setHomePosition();

void testRun();

void setup()
{
  // Initialize the serial port
  Serial.begin(115200);
  // Connect to WiFi
  connectToWiFi(4);
  Serial.println("Connected to WiFi");
  // Connect to WiFi
  setHomePosition();

  // Initialize WebSocket connection
  connectWebSocket();

  // Notify WiFi Connection and North Setting
  for (int i = 1; i <= 5; i++)
  {
    ElevationStepperGotoAngle(60);
    delay(0);
    ElevationStepperGotoAngle(0);
    delay(0);
  }

  testRun();

  
}

void loop()
{
  webSocket.loop(); // Keep the WebSocket connection alive

  unsigned long currentTime = millis();

  // Check if it's time to move the stepper motors
  if (currentTime - lastMoveTime >= moveInterval)
  {
    lastMoveTime = currentTime;
    //pointerElevation = abs(pointerElevation); for debugging

    if (pointerElevation > 0)
    {
      Serial.print("           Azimuth: ");
      Serial.println(pointerAzimuth);
      Serial.print("           Elevation: ");
      Serial.println(pointerElevation);

      AzimuthStepperGotoAngle(pointerAzimuth);
      ElevationStepperGotoAngle(pointerElevation);
    }
    else
    {
      AzimuthStepperGotoAngle(0);
      ElevationStepperGotoAngle(0);

      if (currentTime - lastIamAliveTime >= IamAliveInterval) {
        lastIamAliveTime=currentTime;
       // Notify WiFi Connection and North Setting
  for (int i = 1; i <= 5; i++)
  {
    ElevationStepperGotoAngle(10);
    delay(0);
    ElevationStepperGotoAngle(0);
    delay(0);
  }

 }

    }
  }


}

// Function to move the azimuth stepper to the desired angle (0-360°)
void AzimuthStepperGotoAngle(int angle)
{
  // Ensure the angle is within 0-360°
  angle = constrain(angle, 0, 359); // Valid angle range is 0 to 359

  int newpos;
  int stepsToMove;
  // For angles 0° to 180°, calculate the position linearly
  if (angle >= 0 && angle <= 180)
  {
    // Linear mapping to position from 0 to 1024
    newpos = map(angle, 0, 180, 0, 1024);
    // Calculate the number of steps needed to reach the target position from the current position
  }
  // For angles 180° to 360°, calculate the position linearly but decreasing
  else
  {
    // Linear mapping from 1024 to 0, negative values are mirrored
    newpos = (angle - 180) * (0 - (-1024)) / (360 - 180) + (-1024);
  }
  stepsToMove = newpos - azimuthStepperCurrentPosition;

  if (abs(stepsToMove) > 10)
  {
    AzimuthStepper.setSpeed(10);
  }
  else
  {
    AzimuthStepper.setSpeed(5);
  }

  // Move the stepper to the target position
  AzimuthStepper.step(stepsToMove);

  // Update the current position
  azimuthStepperCurrentPosition = newpos;
}

// Function to move the elevation stepper to the desired angle (0-90°)
void ElevationStepperGotoAngle(int angle)
{
  // Ensure the angle is within 0-90°
  angle = constrain(angle, 0, 90); // Valid angle range is 0 to 90

  int newpos;

  // For angles 0° to 90°, calculate the position linearly
  if (angle >= 0 && angle <= 90)
  {
    // Linear mapping to position from 0 to 512, i.e 2048/4
    newpos = map(angle, 0, 90, 0, 512);
  }

  // Calculate the number of steps needed to reach the target position from the current position
  int stepsToMove = newpos - elevationStepperCurrentPosition;

  // Adjust speed based on the number of steps to move
  if (abs(stepsToMove) > 10)
  {
    ElevationStepper.setSpeed(10); // Faster speed for large movements
  }
  else
  {
    ElevationStepper.setSpeed(5); // Slower speed for small movements
  }

  // Move the stepper to the target position
  ElevationStepper.step(stepsToMove);

  // Update the current position
  elevationStepperCurrentPosition = newpos;
}

void setHomePosition()
{

  // setting home position

  Serial.println("Going to ELEVATION home position");
  ElevationStepper.setSpeed(5);
  ElevationStepper.step(-1024 - 10); // move to zero, or -90°+
  elevationStepperCurrentPosition = 0;
  ElevationStepperGotoAngle(32);
  elevationStepperCurrentPosition = 0;

  Serial.println("Going to AZIMUTH home position");
  AzimuthStepper.setSpeed(5);
  AzimuthStepper.step(2048 + 50); // 1 full turn +10 to be sure with hit the mechanical limit
  AzimuthStepper.step(-2048 / 2); // going to North, half turn
  azimuthStepperCurrentPosition = 0;
  Serial.println("Homing Completed");
}

void testRun()
{
  Serial.println("360° / 90° test run......");
  // Moving from 0° to 359° in 1-degree steps
  for (int azimuthAngle = 0; azimuthAngle < 360; azimuthAngle++)
  {
    AzimuthStepperGotoAngle(azimuthAngle);
    // Elevation calculation: sin(angle * PI / 180) gives the sine value for the angle
    float elevationAngle = sin(PI / 360 * azimuthAngle) * 90;
    ElevationStepperGotoAngle(elevationAngle);
  }
  AzimuthStepperGotoAngle(0);
  ElevationStepperGotoAngle(0);
  Serial.println("Test run completed; back to home position");
}

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("Disconnected from WebSocket");
    retryWebSocketConnection();
    break;
  case WStype_CONNECTED:
    Serial.println("Connected to WebSocket");
    break;
  case WStype_TEXT:
    // Call a function to handle WebSocket message
    handleWebSocketMessage((const char *)payload);
    break;
  case WStype_PONG:
  case WStype_ERROR:
    break;
  }
}

// Function to handle WebSocket message
void handleWebSocketMessage(const char *payload)
{
  DeserializationError error = deserializeJson(doc, payload);
  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  // Retrieve values from the JSON
  const char *satName = doc["satName"];
  float altitude = doc["altitude"];
  float azimuth = doc["azimuth"];
  float elevation = doc["elevation"];
  float latitude = doc["latitude"];
  float longitude = doc["longitude"];
  float distance = doc["distance"];
  float sunAzimuth = doc["sunAzimuth"];
  float sunElevation = doc["sunElevation"];

  // Print the retrieved values
  Serial.println("Satellite Data:");
  Serial.print("Sat Name: ");
  Serial.println(satName);
  Serial.print("Altitude: ");
  Serial.println(altitude);
  Serial.print("Azimuth: ");
  Serial.println(azimuth);
  Serial.print("Elevation: ");
  Serial.println(elevation);
  Serial.print("Latitude: ");
  Serial.println(latitude);
  Serial.print("Longitude: ");
  Serial.println(longitude);
  Serial.print("Distance: ");
  Serial.println(distance);
  Serial.print("Sun Azimuth: ");
  Serial.println(sunAzimuth);
  Serial.print("Sun Elevation: ");
  Serial.println(sunElevation);

  pointerAzimuth = round(azimuth);     // Rounds to the nearest integer;
  pointerElevation = round(elevation); // Rounds to the nearest integer;
}

void connectWebSocket()
{
  // Attempt to connect to WebSocket

  webSocket.begin(websocket_host, websocket_port, "/");

  // Define WebSocket event handlers
  webSocket.onEvent(webSocketEvent);
}

void retryWebSocketConnection()
{
  if (currentRetryAttempt < maxRetryAttempts)
  {
    currentRetryAttempt++;
    Serial.print("Retrying WebSocket connection in ");
    Serial.print(retryInterval / 1000);
    Serial.println(" seconds...");
    delay(retryInterval);
    connectWebSocket();
  }
  else
  {
    Serial.println("Maximum retry attempts reached. Check network and server.");
  }
}

void connectToWiFi(int maxTries)
{
  int attempts = 0;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    attempts++;
    delay(1000);
    Serial.print("Connecting to WiFi... Attempt ");
    Serial.println(attempts);

    if (attempts >= maxTries)
    {
      Serial.println("Max connection attempts reached. Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println("Connected to WiFi");
}