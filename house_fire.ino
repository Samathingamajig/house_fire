#include <LiquidCrystal.h>

#define PIR_PIN 2
#define ADD_TIME_BUTTON_PIN 3
#define START_STOP_BUTTON_PIN 4
#define TIMER_LED_PIN 5
#define D7_PIN 6
#define D6_PIN 7
#define D5_PIN 8
#define D4_PIN 9
#define EN_PIN 10
#define RS_PIN 11
#define PIR_LED_PIN 13

#define MOTION_REQUIRE_TIME_MILLIS 10000
#define TIMER_INCREMENT 300000L

LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

// Create variables:
int motionRaw = 0;
bool motionState = false;  // We start with no motion detected.
int buttonRaw = 0;
int button2Raw = 0;
bool buttonState = false;
bool button2State = false;

enum TimerState {
  RUNNING,
  ENDED,
  PAUSED_MOTION,
  PAUSED_MANUAL,
};

TimerState timerState = ENDED;
long timerRemaining = 0;
unsigned long motionDetectedTime = 0;

unsigned long cmillis = 0;
unsigned long pmillis = 0;
bool firstInSecond = false;

void setup() {
  pinMode(PIR_LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(ADD_TIME_BUTTON_PIN, INPUT_PULLUP);
  pinMode(START_STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TIMER_LED_PIN, OUTPUT);

  lcd.begin(16, 2);

  Serial.begin(9600);

  printRemainingTime(timerRemaining);
}

char *getTimeDescription(long timerRemaining) {
  if (timerState == RUNNING && timerRemaining > 0)
    return "left";
  if (timerRemaining > 0)
    return "(paused)";
  return "ready";
}

void printRemainingTime(long remainingTime) {
  if (remainingTime < 0)
    remainingTime = 0;
  else if (remainingTime % 1000 != 0)
    remainingTime += 1000;
  int hours = remainingTime / 3600000L % 10;
  int minutes = remainingTime / 60000 % 60;
  int seconds = remainingTime / 1000 % 60;
  lcd.setCursor(0, 0);
  char buffer[20];
  sprintf(buffer,
          "%c%s%2d:%02d %-8s",
          hours > 0 ? '0' + hours : ' ',
          hours > 0 ? ":" : " ",
          minutes,
          seconds,
          getTimeDescription(timerRemaining));
  if (hours > 0 && minutes < 10)
    buffer[2] = '0';
  lcd.print(buffer);

  Serial.print(buffer);
  Serial.println(" remaining");
}

void loop() {
  motionRaw = digitalRead(PIR_PIN);
  buttonRaw = !digitalRead(START_STOP_BUTTON_PIN);
  button2Raw = !digitalRead(ADD_TIME_BUTTON_PIN);

  cmillis = millis();
  firstInSecond = (cmillis / 1000) != (pmillis / 1000);

  if (timerState == RUNNING)
    timerRemaining -= (long)(cmillis - pmillis);

  if (firstInSecond && timerState == RUNNING) {
    printRemainingTime(timerRemaining);
    if (timerRemaining < 0) {
      timerState = ENDED;
      digitalWrite(TIMER_LED_PIN, LOW);
    }
  }

  if (timerState == RUNNING && millis() > motionDetectedTime + MOTION_REQUIRE_TIME_MILLIS) {
    Serial.println("Oopsie poopsie you died");
    digitalWrite(TIMER_LED_PIN, LOW);
    timerState = PAUSED_MOTION;
    printRemainingTime(timerRemaining);
  }

  // If motion is detected (PIR_PIN = HIGH), do the following:
  if (motionRaw == HIGH) {
    digitalWrite(PIR_LED_PIN, HIGH);  // Turn on the on-board LED.

    motionDetectedTime = millis();

    // Change the motion state to true (motion detected):
    if (motionState == false) {
      Serial.println("Motion detected!");
      motionState = true;
      if (timerState == PAUSED_MOTION)
        digitalWrite(TIMER_LED_PIN, HIGH);
        timerState = RUNNING;
        printRemainingTime(timerRemaining);
    }
  }

  // If no motion is detected (PIR_PIN = LOW), do the following:
  else {
    digitalWrite(PIR_LED_PIN, LOW);  // Turn off the on-board LED.

    // Change the motion state to false (no motion):
    if (motionState == true) {
      Serial.println("Motion ended!");
      motionState = false;
    }
  }

  if (buttonRaw == HIGH && buttonState == false) {
    buttonState = true;
  } else if (buttonRaw == LOW && buttonState == true) {
    timerState = timerState == RUNNING ? PAUSED_MANUAL : RUNNING;

    motionDetectedTime = millis();
    digitalWrite(TIMER_LED_PIN, timerState == RUNNING);
    printRemainingTime(timerRemaining);
    buttonState = false;
  }

  if (button2Raw == HIGH && button2State == false) {
    button2State = true;
  } else if (button2Raw == LOW && button2State == true) {
    if (timerRemaining < 0)
      timerRemaining = 0;
    timerRemaining += TIMER_INCREMENT;
    printRemainingTime(timerRemaining);
    button2State = false;
  }

  pmillis = cmillis;
}
