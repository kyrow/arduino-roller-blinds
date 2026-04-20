#include <roller_blind.h>

RollerBlind::RollerBlind(uint8_t dir, uint8_t step, uint8_t en, unsigned long ms, long topSteps, float maxSpeed, float acceleration)
    : id(0), dirPin(dir), stepPin(step), enPin(en), timeoutMs(ms), stepsToTop(topSteps),
      stepper(AccelStepper::DRIVER, step, dir), enabled(false), currentStep(0), maxSpeed(maxSpeed), acceleration(acceleration), timeoutOccurred(false), justStopped(false)
{
}

void RollerBlind::begin(uint8_t idx, int initialPercent)
{
  id = idx;
  pinMode(enPin, OUTPUT);
  disable();

  stepper.setMaxSpeed(maxSpeed);
  stepper.setAcceleration(acceleration);

  initialPercent = constrain(initialPercent, 0, 100);
  long initialPosition = (stepsToTop * initialPercent) / 100;
  stepper.setCurrentPosition(initialPosition);
  currentStep = initialPosition;
  targetPosition = initialPosition;

  Serial.printf("Blind %d: Initialized at %d%% (step %ld)\n", id, initialPercent, initialPosition);
}

bool RollerBlind::isMoving()
{
  return stepper.isRunning();
}

void RollerBlind::enable()
{
  digitalWrite(enPin, LOW);
  enabled = true;
}

void RollerBlind::disable()
{
  digitalWrite(enPin, HIGH);
  enabled = false;
}

void RollerBlind::stop()
{
  currentStep = stepper.currentPosition();
  stepper.stop();
  stepper.setCurrentPosition(currentStep);
  targetPosition = currentStep;
  stepper.moveTo(targetPosition);
  justStopped = true;
}

bool RollerBlind::hasJustStopped()
{
  if (justStopped)
  {
    justStopped = false;
    return true;
  }
  return false;
}

int RollerBlind::getPositionPercent()
{
  if (stepsToTop == 0)
    return 0;

  long position = constrain(currentStep, 0, stepsToTop);
  int percent = (position * 100) / stepsToTop;
  return constrain(percent, 0, 100);
}

void RollerBlind::update()
{
  if (stepper.isRunning())
  {
    if (!enabled)
      enable();
    stepper.run();
    currentStep = stepper.currentPosition();

    if (!timeoutOccurred && millis() - moveStartTime >= timeoutMs)
    {
      Serial.println("Movement timeout!");
      timeoutOccurred = true;
      stop();
      return;
    }
  }
  else if (enabled)
  {
    currentStep = stepper.currentPosition();
    justStopped = true;
    disable();
  }
}

bool RollerBlind::moveToTop()
{
  enable();
  timeoutOccurred = false;
  targetPosition = stepsToTop;
  stepper.moveTo(targetPosition);
  moveStartTime = millis();
  Serial.printf("Moving to 100%% (step %ld)\n", targetPosition);
  return true;
}

bool RollerBlind::moveToZero()
{
  enable();
  timeoutOccurred = false;
  targetPosition = 0;
  stepper.moveTo(targetPosition);
  moveStartTime = millis();
  Serial.println("Moving to 0%");
  return true;
}

bool RollerBlind::moveToPercent(int percent)
{
  percent = constrain(percent, 0, 100);
  targetPosition = (stepsToTop * percent) / 100;
  enable();
  timeoutOccurred = false;
  stepper.moveTo(targetPosition);
  moveStartTime = millis();
  Serial.printf("Moving to %d%% (step %ld)\n", percent, targetPosition);
  return true;
}