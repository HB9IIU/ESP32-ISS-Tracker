#include <AccelStepper.h>

// Motor Driver Pins for Azimuth (X-axis)
#define IN1azimuth 19
#define IN2azimuth 18
#define IN3azimuth 23
#define IN4azimuth 22

// Motor Driver Pins for Elevation (Y-axis)
#define IN1elevation 13
#define IN2elevation 12
#define IN3elevation 2
#define IN4elevation 4

// Steps per revolution for your motors
const int stepsPerRevolution = 2048;  // Adjust this based on your motor specs

// Initialize AccelStepper objects for Azimuth and Elevation motors
AccelStepper AzimuthStepper(AccelStepper::FULL4WIRE, IN1azimuth, IN2azimuth, IN3azimuth, IN4azimuth);
AccelStepper ElevationStepper(AccelStepper::FULL4WIRE, IN1elevation, IN2elevation, IN3elevation, IN4elevation);

// Speed and acceleration settings for both motors (increased values for testing)
float azimuthSpeed = 200.0;  // Speed in steps per second (higher value for testing)
float elevationSpeed = 200.0; // Speed in steps per second (higher value for testing)
float azimuthAcceleration = 100.0; // Acceleration in steps per second^2 (higher value for testing)
float elevationAcceleration = 100.0; // Acceleration in steps per second^2 (higher value for testing)

void setup() {
  // Set max speed and acceleration for both motors
  AzimuthStepper.setMaxSpeed(azimuthSpeed);
  AzimuthStepper.setAcceleration(azimuthAcceleration);
  
  ElevationStepper.setMaxSpeed(elevationSpeed);
  ElevationStepper.setAcceleration(elevationAcceleration);

  // Initialize the serial port
  Serial.begin(115200);
}

void loop() {
  // Target coordinates (x, y) in degrees
  int x = 180;  // Azimuth (X-axis) target
  int y = 45;   // Elevation (Y-axis) target

  // Calculate target steps for Azimuth and Elevation
  long azimuthSteps = map(x, 0, 360, 0, stepsPerRevolution);   // Map azimuth angle to steps
  long elevationSteps = map(y, 0, 90, 0, stepsPerRevolution / 4); // Map elevation angle to steps

  // Move both Azimuth and Elevation steppers simultaneously
  AzimuthStepper.moveTo(azimuthSteps);
  ElevationStepper.moveTo(elevationSteps);

  // Run motors until both reach their target position
  while (AzimuthStepper.distanceToGo() != 0 || ElevationStepper.distanceToGo() != 0) {
    AzimuthStepper.run();
    ElevationStepper.run();
  }

  delay(5000);  // Wait for 5 seconds at the target position before moving again

  // Optionally, move back to position (0, 0) or any other coordinates
  AzimuthStepper.moveTo(0);  // Azimuth (X-axis) goes back to 0°
  ElevationStepper.moveTo(0);  // Elevation (Y-axis) goes back to 0°
  
  // Run motors until both reach position (0, 0)
  while (AzimuthStepper.distanceToGo() != 0 || ElevationStepper.distanceToGo() != 0) {
    AzimuthStepper.run();
    ElevationStepper.run();
  }
}
