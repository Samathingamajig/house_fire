#define PIR_PIN 2
#define BUTTON_PIN 3
#define PIR_LED_PIN 13
#define BUTTON_LED_PIN 4
#define TIMER_LED_PIN 5

#define MOTION_REQUIRE_TIME_MILLIS 10000

// Create variables:
int motionRaw = 0;
bool motionState = false; // We start with no motion detected.
int buttonRaw = 0;
bool buttonState = false;

bool timerRunning = false;
unsigned long motionDetectedTime = 0;

void setup()
{
  // Configure the pins as input or output:
  pinMode(PIR_LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LED_PIN, OUTPUT);
  pinMode(TIMER_LED_PIN, OUTPUT);

  // Begin serial communication at a baud rate of 9600:
  Serial.begin(9600);
}

void loop()
{
  // Read out the PIR_PIN and store as val:
  motionRaw = digitalRead(PIR_PIN);
  buttonRaw = !digitalRead(BUTTON_PIN);

  // Serial.println(millis() - motionDetectedTime);

  if (timerRunning && millis() > motionDetectedTime + MOTION_REQUIRE_TIME_MILLIS)
  {
    Serial.println("Oopsie poopsie you died");
    digitalWrite(TIMER_LED_PIN, LOW);
    timerRunning = false;
  }

  // If motion is detected (PIR_PIN = HIGH), do the following:
  if (motionRaw == HIGH)
  {
    digitalWrite(PIR_LED_PIN, HIGH); // Turn on the on-board LED.

    motionDetectedTime = millis();

    // Change the motion state to true (motion detected):
    if (motionState == false)
    {
      Serial.println("Motion detected!");
      motionState = true;
    }
  }

  // If no motion is detected (PIR_PIN = LOW), do the following:
  else
  {
    digitalWrite(PIR_LED_PIN, LOW); // Turn off the on-board LED.

    // Change the motion state to false (no motion):
    if (motionState == true)
    {
      Serial.println("Motion ended!");
      motionState = false;
    }
  }

  if (buttonRaw == HIGH && buttonState == false)
  {
    Serial.println("Button pushed!");
    digitalWrite(BUTTON_LED_PIN, HIGH);
    buttonState = true;
  }
  else if (buttonRaw == LOW && buttonState == true)
  {
    Serial.println("Button released!");
    timerRunning = !timerRunning;
    motionDetectedTime = millis();
    digitalWrite(TIMER_LED_PIN, timerRunning);
    digitalWrite(BUTTON_LED_PIN, LOW);
    buttonState = false;
  }
}
