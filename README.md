# Nano-RP2040 Fitness Tracker
## Overview
This project uses a Nano RP2040 board connected to an external heart rate monitor to track various fitness and health activities.
The Nano RP2040 board has a Raspberry Pi RP2040 microcontroller and an inbuilt 6-axis Inertia Measurement Unit (IMU) with a dedicated
machine learning core that will help us detect activities such as walking and jogging.
The data collected from the sensors is sent to a Node.js backend via MQTT for analysis.

## Components 
* Nano-RP2040 microcontroller
* MAX30100 heart rate and SpO2 sensor
* Breadboard and jumper wires
* USB cable for programming and power
* Mosquitto MQTT broker

## Hardware setup
1. Connect the MAX30100 sensor to the Nano-RP2040:
   * VCC to 3.3V
   * GND to GND
   * SDA to GPIO12 (I2C SDA)
   * SCL to GPIO13 (I2C SCL)
