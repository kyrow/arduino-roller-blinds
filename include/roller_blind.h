#pragma once
#include <Arduino.h>
#include <AccelStepper.h>

class RollerBlind
{
private:
  uint8_t id;
  uint8_t dirPin;
  uint8_t stepPin;
  uint8_t enPin;
  AccelStepper stepper;
  bool enabled;
  unsigned long timeoutMs;
  long stepsToTop;
  long currentStep;
  float maxSpeed;
  float acceleration;
  unsigned long moveStartTime;
  long targetPosition;
  bool timeoutOccurred;
  bool justStopped;

public:
  RollerBlind(uint8_t dir, uint8_t step, uint8_t en, unsigned long ms = 60000, long stepsToTop = 10000, float maxSpeed = 1000, float acceleration = 500);

  void begin(uint8_t id, int initialPercent);
  void enable();
  void disable();
  void update();
  bool isMoving();
  bool hasJustStopped();

  bool moveToTop();
  bool moveToZero();
  bool moveToPercent(int percent);
  void stop();
  long getCurrentStep() { return currentStep; }
  long getStepsToTop() { return stepsToTop; }
  int getPositionPercent();
  bool hasTimeoutOccurred() { return timeoutOccurred; }
  void clearTimeoutFlag() { timeoutOccurred = false; }
};