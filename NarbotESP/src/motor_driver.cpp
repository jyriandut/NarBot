#include "motor_driver.h"

#include <Arduino.h>

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
constexpr int MOTOR_SPEED = 220;
#else
constexpr int MOTOR_SPEED = 0;
#endif

MotorDirection leftDirection = MotorDirection::Stopped;
MotorDirection rightDirection = MotorDirection::Stopped;
int leftPwm = 0;
int rightPwm = 0;

#ifdef MAIN_ESP
void writeMotor(int dirPin, int pwmChannel, MotorDirection direction, int speed) {
  digitalWrite(dirPin, direction == MotorDirection::Backward ? HIGH : LOW);
  ledcWrite(pwmChannel, speed);
}
#endif

void setMotorState(MotorDirection newLeftDirection, MotorDirection newRightDirection) {
  leftDirection = newLeftDirection;
  rightDirection = newRightDirection;
  leftPwm = leftDirection == MotorDirection::Stopped ? 0 : MOTOR_SPEED;
  rightPwm = rightDirection == MotorDirection::Stopped ? 0 : MOTOR_SPEED;

#ifdef MAIN_ESP
  writeMotor(LEFT_DIR_PIN, LEFT_PWM_CHANNEL, leftDirection, leftPwm);
  writeMotor(RIGHT_DIR_PIN, RIGHT_PWM_CHANNEL, rightDirection, rightPwm);
  Serial.printf("Motor state: left=%s pwm=%d right=%s pwm=%d\n",
                motorDirectionName(leftDirection),
                leftPwm,
                motorDirectionName(rightDirection),
                rightPwm);
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
}

void stopMotors() {
  setMotorState(MotorDirection::Stopped, MotorDirection::Stopped);
}

void driveForward() {
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

MotorDriverState getMotorDriverState() {
  return {leftDirection, rightDirection, leftPwm, rightPwm};
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
