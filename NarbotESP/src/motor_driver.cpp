#include "motor_driver.h"

#include <Arduino.h>

#include "obstacle_safety.h"

namespace {

#ifdef MAIN_ESP
// DFRobot TB6612FNG motor driver on Seeed XIAO ESP32S3 Arduino pins.
constexpr int LEFT_DIR_PIN = D6;   // GPIO43, DIR1
constexpr int LEFT_PWM_PIN = D5;   // GPIO6, PWM1
constexpr int RIGHT_DIR_PIN = D4;  // GPIO5, DIR2
constexpr int RIGHT_PWM_PIN = D3;  // GPIO4, PWM2

constexpr int LEFT_PWM_CHANNEL = 2;
constexpr int RIGHT_PWM_CHANNEL = 3;
constexpr int MOTOR_PWM_FREQ = 20000;
constexpr int MOTOR_PWM_RESOLUTION = 8;
constexpr int DEFAULT_MOTOR_SPEED = 180;
constexpr int MAX_MOTOR_SPEED = 255;
constexpr int MOTOR_RAMP_STEP = 12;
constexpr unsigned long MOTOR_RAMP_INTERVAL_MS = 30;
#else
constexpr int DEFAULT_MOTOR_SPEED = 0;
constexpr int MAX_MOTOR_SPEED = 0;
#endif

MotorDirection leftDirection = MotorDirection::Stopped;
MotorDirection rightDirection = MotorDirection::Stopped;
int leftPwm = 0;
int rightPwm = 0;
int leftTargetPwm = 0;
int rightTargetPwm = 0;
int targetSpeed = DEFAULT_MOTOR_SPEED;
unsigned long lastRampAt = 0;

#ifdef MAIN_ESP
void writeMotor(int dirPin, int pwmChannel, MotorDirection direction, int speed) {
  digitalWrite(dirPin, direction == MotorDirection::Backward ? HIGH : LOW);
  ledcWrite(pwmChannel, speed);
}

int stepToward(int value, int target) {
  if (value < target) {
    return min(value + MOTOR_RAMP_STEP, target);
  }

  if (value > target) {
    return max(value - MOTOR_RAMP_STEP, target);
  }

  return value;
}
#endif

void setMotorState(MotorDirection newLeftDirection, MotorDirection newRightDirection) {
  bool leftDirectionChanged = leftDirection != newLeftDirection;
  bool rightDirectionChanged = rightDirection != newRightDirection;

  leftDirection = newLeftDirection;
  rightDirection = newRightDirection;
  leftTargetPwm = leftDirection == MotorDirection::Stopped ? 0 : targetSpeed;
  rightTargetPwm = rightDirection == MotorDirection::Stopped ? 0 : targetSpeed;

  if (leftDirection == MotorDirection::Stopped || leftDirectionChanged) {
    leftPwm = 0;
  }

  if (rightDirection == MotorDirection::Stopped || rightDirectionChanged) {
    rightPwm = 0;
  }

#ifdef MAIN_ESP
  writeMotor(LEFT_DIR_PIN, LEFT_PWM_CHANNEL, leftDirection, leftPwm);
  writeMotor(RIGHT_DIR_PIN, RIGHT_PWM_CHANNEL, rightDirection, rightPwm);
  Serial.printf("Motor target: left=%s pwm=%d right=%s pwm=%d speed=%d\n",
                motorDirectionName(leftDirection),
                leftTargetPwm,
                motorDirectionName(rightDirection),
                rightTargetPwm,
                targetSpeed);
#endif
}

}  // namespace

void setupMotorDriver() {
#ifdef MAIN_ESP
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);

  digitalWrite(LEFT_PWM_PIN, LOW);
  digitalWrite(RIGHT_PWM_PIN, LOW);
  digitalWrite(LEFT_DIR_PIN, LOW);
  digitalWrite(RIGHT_DIR_PIN, LOW);

  ledcSetup(LEFT_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
  ledcWrite(LEFT_PWM_CHANNEL, 0);
  ledcWrite(RIGHT_PWM_CHANNEL, 0);
  ledcAttachPin(LEFT_PWM_PIN, LEFT_PWM_CHANNEL);
  ledcAttachPin(RIGHT_PWM_PIN, RIGHT_PWM_CHANNEL);
#endif

  stopMotors();

#ifdef MAIN_ESP
  Serial.println("Motor driver initialized");
  Serial.printf("Left motor: DIR D6/GPIO%d, PWM D5/GPIO%d, LEDC channel %d\n", LEFT_DIR_PIN, LEFT_PWM_PIN, LEFT_PWM_CHANNEL);
  Serial.printf("Right motor: DIR D4/GPIO%d, PWM D3/GPIO%d, LEDC channel %d\n", RIGHT_DIR_PIN, RIGHT_PWM_PIN, RIGHT_PWM_CHANNEL);
#endif
}

void handleMotorDriver() {
#ifdef MAIN_ESP
  if (millis() - lastRampAt < MOTOR_RAMP_INTERVAL_MS) {
    return;
  }

  lastRampAt = millis();
  int nextLeftPwm = stepToward(leftPwm, leftTargetPwm);
  int nextRightPwm = stepToward(rightPwm, rightTargetPwm);

  if (nextLeftPwm == leftPwm && nextRightPwm == rightPwm) {
    return;
  }

  leftPwm = nextLeftPwm;
  rightPwm = nextRightPwm;
  writeMotor(LEFT_DIR_PIN, LEFT_PWM_CHANNEL, leftDirection, leftPwm);
  writeMotor(RIGHT_DIR_PIN, RIGHT_PWM_CHANNEL, rightDirection, rightPwm);
#endif
}

void stopMotors() {
  setMotorState(MotorDirection::Stopped, MotorDirection::Stopped);
}

void driveForward() {
  if (isForwardBlocked()) {
    Serial.println("Forward blocked by obstacle safety");
    stopMotors();
    return;
  }

  setMotorState(MotorDirection::Forward, MotorDirection::Forward);
}

void driveBackward() {
  setMotorState(MotorDirection::Backward, MotorDirection::Backward);
}

void turnLeft() {
  setMotorState(MotorDirection::Backward, MotorDirection::Forward);
}

void turnRight() {
  setMotorState(MotorDirection::Forward, MotorDirection::Backward);
}

void setMotorTargetSpeed(int speed) {
  targetSpeed = constrain(speed, 0, MAX_MOTOR_SPEED);
  leftTargetPwm = leftDirection == MotorDirection::Stopped ? 0 : targetSpeed;
  rightTargetPwm = rightDirection == MotorDirection::Stopped ? 0 : targetSpeed;

#ifdef MAIN_ESP
  Serial.printf("Motor speed target set to %d\n", targetSpeed);
#endif
}

MotorDriverState getMotorDriverState() {
  return {leftDirection, rightDirection, leftPwm, rightPwm, targetSpeed};
}

const char *motorDirectionName(MotorDirection direction) {
  if (direction == MotorDirection::Forward) {
    return "Forward";
  }

  if (direction == MotorDirection::Backward) {
    return "Backward";
  }

  return "Stopped";
}
