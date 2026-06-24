# SMARS Robot Motor Control — Code Specification

## Goal

Create firmware for **ESP32-CAM** that controls **two DC motors** through an **L298N** motor driver for bench testing.

This is the **first stage only**:
- no camera logic yet
- no red-dot detection yet
- no chassis logic yet

The code must only prove that the robot can drive its motors correctly.

---

## Hardware

- **Board:** ESP32-CAM
- **Motor driver:** L298N
- **Motors:** 2 × G12-N20 geared DC motors

---

## Wiring Assumptions

### ESP32-CAM to L298N
- `GPIO13` → `IN1`
- `GPIO14` → `IN2`
- `GPIO15` → `IN3`
- `GPIO12` → `IN4`
- `GND` → `GND`

### L298N
- `OUT1`, `OUT2` → left motor
- `OUT3`, `OUT4` → right motor
- `ENA`, `ENB` jumpers remain installed for now

### Power
- L298N motor power comes from external supply
- ESP32-CAM is powered separately
- grounds must be shared

---

## Functional Requirements

The firmware must:

1. initialize all motor control pins as outputs
2. stop both motors on startup
3. open serial communication at `115200`
4. accept single-character commands from serial input
5. control motor direction based on commands

---

## Required Commands

### Movement commands
- `f` → drive forward
- `b` → drive backward
- `l` → turn left
- `r` → turn right
- `s` → stop

### Behavior
- each command must immediately change motor state
- unknown characters may be ignored

---

## Motor Logic

### Left motor
Controlled by:
- `IN1`
- `IN2`

### Right motor
Controlled by:
- `IN3`
- `IN4`

### Direction rules
For each motor:
- forward = first pin `HIGH`, second pin `LOW`
- backward = first pin `LOW`, second pin `HIGH`
- stop = both pins `LOW`

---

## Startup Behavior

On boot, the firmware must:
1. start serial output
2. configure pins
3. stop motors
4. print a short help message showing available commands

---

## Serial Output

The code should print:
- a startup banner
- available commands
- a short message whenever a valid command is received

Example:
- `Forward`
- `Backward`
- `Left`
- `Right`
- `Stop`

---

## Non-Goals for This Version

This version does **not** need:
- PWM speed control
- camera initialization
- Wi-Fi
- web server
- autonomous driving
- red color detection
- obstacle detection
- dual camera support

---

## Acceptance Criteria

The code is considered complete when:

1. the ESP32-CAM boots successfully
2. serial monitor works at `115200`
3. entering `f` spins both motors forward
4. entering `b` spins both motors backward
5. entering `l` turns left
6. entering `r` turns right
7. entering `s` stops both motors

---

## Next Version

After this version works, the next stage should add:
1. safer flashing workflow notes for ESP32-CAM
2. optional PWM speed control
3. integration with front camera logic
4. conversion from manual commands to autonomous steering
