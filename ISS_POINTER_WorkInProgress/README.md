# MLA-Controller-Slave

This repository contains the code for the **ESP32 microcontroller** functioning as the **MASTER** device in a radio control system. 
The MASTER is equipped with a TFT touchscreen interface, providing users with a graphical interface to control and monitor the system's operations.

## Overview

The MASTER device communicates with a SLAVE ESP32 microcontroller, which manages a stepper motor via a microstepping driver. 
This setup enables precise control of mechanical movements in various applications, such as antenna positioning for radio transceivers.
![MLA Controller Image](Doc/Diag/diag.png)
## Features

- **TFT Touchscreen Interface**: Interactive GUI for user input and feedback.
- **Bluetooth Communication**: Connects with transceivers like the Icom IC 705.
- **Ad Hoc 2.4 GHz Network**: Facilitates wireless communication between MASTER and SLAVE devices.
- **Stepper Motor Control**: Implements microstepping for smooth and precise movements.

## Requirements

- ESP32 Microcontroller
- TFT Touchscreen Display
- Microstepping Driver
- Stepper Motor with Reduction Gear
- Required Libraries:
  - [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
  - [WiFi](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/your-repo-name.git


