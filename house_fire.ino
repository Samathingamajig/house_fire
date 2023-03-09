#include <LiquidCrystal.h>

#define PIR_PIN 2
#define ACTION_BUTTON_PIN 3
#define MENU_BUTTON_PIN 4
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

enum StateChange {
  NO_CHANGE,
  LOW_TO_HIGH,
  HIGH_TO_LOW,
};

class Latch {
public:
  bool state = 0;
  StateChange stateChange = NO_CHANGE;

  void setState(bool newState) {
    if (!state && newState) {
      stateChange = LOW_TO_HIGH;
    } else if (state && !newState) {
      stateChange = HIGH_TO_LOW;
    } else {
      stateChange = NO_CHANGE;
    }

    state = newState;
  }
};

Latch motionLatch;
Latch menuButtonLatch;
Latch actionButtonLatch;

enum TimerState {
  ENDED,
  RUNNING,
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
  pinMode(ACTION_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
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

  Serial.println(buffer);
}

void loop() {
  motionLatch.setState(digitalRead(PIR_PIN) == HIGH);
  menuButtonLatch.setState(digitalRead(MENU_BUTTON_PIN) == LOW); // Sam's buttons work backwards for some reason, on any good breadboard this should be `== HIGH`
  actionButtonLatch.setState(digitalRead(ACTION_BUTTON_PIN) == LOW); // same here

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


  if (motionLatch.state) {
    motionDetectedTime = cmillis;

    if (motionLatch.stateChange == LOW_TO_HIGH) {
      Serial.println("Motion detected");
      if (timerState == PAUSED_MOTION) {
        digitalWrite(TIMER_LED_PIN, HIGH);
        timerState = RUNNING;
      }
      printRemainingTime(timerRemaining);
    }
  } else if (motionLatch.stateChange == HIGH_TO_LOW) {
    Serial.println("Motion ended!");
  }

  if (menuButtonLatch.stateChange == HIGH_TO_LOW) {
    timerState = (timerState == RUNNING) ? PAUSED_MANUAL : RUNNING;
    motionDetectedTime = cmillis;
    digitalWrite(TIMER_LED_PIN, timerState == RUNNING);
    printRemainingTime(timerRemaining);
  }

  if (actionButtonLatch.stateChange == HIGH_TO_LOW) {
    if (timerRemaining < 0)
      timerRemaining = 0;
    timerRemaining += TIMER_INCREMENT;
    printRemainingTime(timerRemaining);
  }

  pmillis = cmillis;
}
