#pragma once

#include <Arduino.h>

enum class MotorDirection {
  Stopped,
  Forward,
  Backward,
};

struct MotorDriverState {
  MotorDirection leftDirection;
  MotorDirection rightDirection;
  int leftPwm;
  int rightPwm;
};

void setupMotorDriver();
void handleMotorDriver();

void stopMotors();
void driveForward();
void driveBackward();
void turnLeft();
void turnRight();

MotorDriverState getMotorDriverState();
const char *motorDirectionName(MotorDirection direction);
