
#include <Arduino.h>
#include <Stepper.h>

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution

// ULN2003 Motor Driver Pins
#define IN1azimuth 19
#define IN2azimuth 18
#define IN3azimuth 23
#define IN4azimuth 22

#define IN1elevation 27
#define IN2elevation 14
#define IN3elevation 25
#define IN4elevation 26


// initialize the stepper library
Stepper AzimuthStepper(stepsPerRevolution, IN1azimuth, IN3azimuth, IN2azimuth, IN4azimuth);
Stepper ElevationStepper(stepsPerRevolution, IN1elevation, IN3elevation, IN2elevation, IN4elevation);



void setup() {
  // set the speed at 5 rpm
  AzimuthStepper.setSpeed(15);
  ElevationStepper.setSpeed(15);
  // initialize the serial port
  Serial.begin(115200);
}

void loop() {
  // step one revolution in one direction:
  Serial.println("clockwise");
  AzimuthStepper.step(stepsPerRevolution);
  delay(1000);

  // step one revolution in the other direction:
  Serial.println("counterclockwise");
  AzimuthStepper.step(-stepsPerRevolution);
  delay(1000);

 // step one revolution in one direction:
 Serial.println("clockwise");
 ElevationStepper.step(stepsPerRevolution);
 delay(1000);

 // step one revolution in the other direction:
 Serial.println("counterclockwise");
 ElevationStepper.step(-stepsPerRevolution);
 delay(1000);




}