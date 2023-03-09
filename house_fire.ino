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

const long motionRequireTimesMillis[]{
  0,  // off
  10L * 1000,
  1L * 1000 * 60,
  2L * 1000 * 60,
  5L * 1000 * 60,
  10L * 1000 * 60,
  15L * 1000 * 60,
  20L * 1000 * 60,
  30L * 1000 * 60,
  60L * 1000 * 60
};
const int motionRequireTimesLength = 10;
int motionRequireTimeIndex = 0;

enum class Scene {
  Main,
  TimeChanging,
  MotionTimeChanging,
};

Scene scene = Scene::Main;

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
  if (timerState == RUNNING && timerRemaining > 0) {
    return "left    ";
  } else if (timerRemaining > 0) {
    return "(paused)";
  } else {
    return "ready   ";
  }
}

void printMotionRequireTime(long motionRequireTimeMillis) {
  int minutes = motionRequireTimeMillis / 60000;
  int seconds = motionRequireTimeMillis / 1000 % 60;

  char buffer[20];

  if (minutes > 0) {
    sprintf(buffer, "%5d minute%s", minutes, minutes == 1 ? "" : "s");
  } else if (seconds > 0) {
    sprintf(buffer, "%5d second%s", seconds, seconds == 1 ? "" : "s");
  } else {
    strcpy(buffer, "   (disabled)");
  }
  // sprintf(buffer, "%16s", buffer);

  lcd.home();
  lcd.print("                ");
  lcd.home();
  lcd.print(buffer);

  Serial.println(buffer);
}

void printTimeString(long remainingTime) {
  if (remainingTime < 0) {
    remainingTime = 0;
  } else if (remainingTime % 1000 != 0) {
    remainingTime += 1000;
  }
  int hours = remainingTime / 3600000L % 10;
  int minutes = remainingTime / 60000 % 60;
  int seconds = remainingTime / 1000 % 60;
  char buffer[20];
  sprintf(buffer,
          "%c%s%2d:%02d",
          hours > 0 ? '0' + hours : ' ',
          hours > 0 ? ":" : " ",
          minutes,
          seconds);
  if (hours > 0 && minutes < 10) {
    buffer[2] = '0';
  }
  lcd.print(buffer);
  Serial.println(buffer);
}

void printRemainingTime(long remainingTime) {
  if (remainingTime < 0) {
    remainingTime = 0;
  } else if (remainingTime % 1000 != 0) {
    remainingTime += 1000 - remainingTime % 1000;
  }
  lcd.setCursor(0, 0);
  printTimeString(remainingTime);
  lcd.setCursor(8, 0);
  lcd.print(getTimeDescription(remainingTime));
  Serial.println(getTimeDescription(remainingTime));
}

void loop() {
  motionLatch.setState(digitalRead(PIR_PIN) == HIGH);
  menuButtonLatch.setState(digitalRead(MENU_BUTTON_PIN) == LOW);      // Sam's buttons work backwards for some reason, on any good breadboard this should be `== HIGH`
  actionButtonLatch.setState(digitalRead(ACTION_BUTTON_PIN) == LOW);  // same here

  cmillis = millis();
  firstInSecond = (cmillis / 1000) != (pmillis / 1000);

  if (scene == Scene::Main) {
    if (timerState == RUNNING) {
      timerRemaining -= (long)(cmillis - pmillis);
    }

    if (firstInSecond && timerState == RUNNING) {
      printRemainingTime(timerRemaining);
      if (timerRemaining < 0) {
        timerState = ENDED;
        timerRemaining = 0;
        digitalWrite(TIMER_LED_PIN, LOW);
      }
    }

    if (timerState == RUNNING
        && (unsigned long)(cmillis - motionDetectedTime) > motionRequireTimesMillis[motionRequireTimeIndex]
        && motionRequireTimesMillis[motionRequireTimeIndex] != 0) {
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

    if (actionButtonLatch.stateChange == HIGH_TO_LOW) {
      motionDetectedTime = cmillis;
      if (timerRemaining > 0) {
        timerState = (timerState == RUNNING) ? PAUSED_MANUAL : RUNNING;
        digitalWrite(TIMER_LED_PIN, timerState == RUNNING);
        printRemainingTime(timerRemaining);
      }
    }

    bool transitionSafe = timerState == ENDED || timerState == PAUSED_MANUAL || timerState == PAUSED_MOTION;
    if (menuButtonLatch.stateChange == HIGH_TO_LOW) {
      if (transitionSafe) {
        if (timerState == PAUSED_MOTION) {
          timerState = PAUSED_MANUAL;
        }

        scene = Scene::TimeChanging;
        lcd.clear();
        lcd.setCursor(2, 1);
        lcd.print("Modify Timer");
        lcd.home();
        printTimeString(timerRemaining);
      } else {
        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
    } else if (menuButtonLatch.stateChange == LOW_TO_HIGH && !transitionSafe) {
      lcd.setCursor(2, 1);
      lcd.print("(stop timer)");
    }
  } else if (scene == Scene::TimeChanging) {
    if (actionButtonLatch.stateChange == HIGH_TO_LOW) {
      if (timerRemaining < 0) {
        timerRemaining = 0;
      }
      motionDetectedTime = cmillis;
      timerRemaining += TIMER_INCREMENT;
      if (timerRemaining > 35999000L) {
        timerRemaining = 35999000L;
      }
      lcd.home();
      printTimeString(timerRemaining);
    }

    if (menuButtonLatch.stateChange == HIGH_TO_LOW) {
      scene = Scene::MotionTimeChanging;
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print("Motion Timer");
      printMotionRequireTime(motionRequireTimesMillis[motionRequireTimeIndex]);
      lcd.home();
    }
  } else if (scene == Scene::MotionTimeChanging) {
    if (actionButtonLatch.stateChange == HIGH_TO_LOW) {
      motionRequireTimeIndex = (motionRequireTimeIndex + 1) % motionRequireTimesLength;
      printMotionRequireTime(motionRequireTimesMillis[motionRequireTimeIndex]);
    }

    if (menuButtonLatch.stateChange == HIGH_TO_LOW) {
      scene = Scene::Main;
      lcd.clear();
      lcd.home();
      printRemainingTime(timerRemaining);
    }
  }

  pmillis = cmillis;
}
