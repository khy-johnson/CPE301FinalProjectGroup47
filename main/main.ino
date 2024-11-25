//Tyler Olson & Khyber Johnson
#include <I2C_RTC.h>
#include <Stepper.h>
#include <LiquidCrystal.h>
#include <dht.h>

static dht DHT;
static DS1307 RTC;

//Stepper motor
const int STEPS_PER_REV = 2048;
Stepper myStepper = Stepper(STEPS_PER_REV, 23, 27, 25, 29);  // IN1-IN3-IN2-IN4

//LCD pins
const int RS = 50, EN = 52, D4 = 40, D5 = 42, D6 = 44, D7 = 46;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//Temp sensor pin
const int HUMID_TEMP_PIN = 3;

//LED pins
const int YELLOW_LED_PIN = 10;
const int GREEN_LED_PIN = 11;
const int BLUE_LED_PIN = 12;
const int RED_LED_PIN = 9;

//DC fan pin
const int DC_FAN_PIN = 2;

//State button pins
const int OFF_BUTTON_PIN = 8;
const int ON_BUTTON_PIN = 7;

//Stepper motor pins
int CLOCKWISE_PIN = 33;
int COUNTER_CLOCKWISE_PIN = 35;

//Water sensor pin
const int WATER_PIN = A15;

enum StateType {
  DISABLED,
  IDLE,
  RUNNING,
  ERROR
};

StateType state = IDLE;
int waterLevel = 0;
unsigned long previousMillis = 0;

const int UPDATE_INTERVAL = 60000;
const int TEMP_THRESHOLD = 22;
const int WATER_THRESHOLD = 25;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();

  myStepper.setSpeed(10);

  //init LEDs
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  pinMode(DC_FAN_PIN, OUTPUT);
  pinMode(OFF_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ON_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CLOCKWISE_PIN, INPUT_PULLUP);
  pinMode(COUNTER_CLOCKWISE_PIN, INPUT_PULLUP);

  updateTempHumidReading();

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  if (RTC.begin() == false) {
    Serial.println("RTC Not Connected!");
    while (true)
      ;
  }
}

void loop() {
  updateLEDS();
  handleStateButtons();

  waterLevel = analogRead(WATER_PIN);
  unsigned long currentMillis = millis();

  switch (state) {
    case DISABLED:  //fan off, yellow led on
      printLCD("DISABLED", "");
      digitalWrite(DC_FAN_PIN, LOW);
      break;
    case IDLE:  //fan off, green led on
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
        previousMillis = currentMillis;

        updateTempHumidReading();
      }

      handleStepperButtons();
      checkWaterLevel();

      digitalWrite(DC_FAN_PIN, LOW);

      if (DHT.temperature > TEMP_THRESHOLD) {
        changeState(RUNNING);
      }
      break;
    case ERROR:  //red led on
      printLCD("Water level is", "too low!");

      checkWaterLevel();

      digitalWrite(DC_FAN_PIN, LOW);
      break;
    case RUNNING:  //fan on, blue led on
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));
      if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
        previousMillis = currentMillis;

        updateTempHumidReading();
      }

      handleStepperButtons();
      checkWaterLevel();

      digitalWrite(DC_FAN_PIN, HIGH);

      if (DHT.temperature <= TEMP_THRESHOLD) {
        changeState(IDLE);
      }
      break;
  };
}

void handleStateButtons() {
  if (digitalRead(ON_BUTTON_PIN) == LOW) {
    changeState(DISABLED);

  } else if (digitalRead(OFF_BUTTON_PIN) == LOW) {
    changeState(IDLE);
  }
}

void handleStepperButtons() {
  while (digitalRead(CLOCKWISE_PIN) == LOW && digitalRead(COUNTER_CLOCKWISE_PIN) == HIGH) {
    myStepper.step(64);
  }

  while (digitalRead(CLOCKWISE_PIN) == HIGH && digitalRead(COUNTER_CLOCKWISE_PIN) == LOW) {
    myStepper.step(-64);
  }
}

void changeState(StateType newState) {
  if (newState == state) {
    return;
  }

  if (newState == RUNNING) {
    Serial.print("Fan turned ON at: " + getCurrentTime());
  } else if (state == RUNNING) {
    Serial.print("Fan turned OFF at: " + getCurrentTime());
  }

  state = newState;
  lcd.clear();
}

void printLCD(String firstLine, String secondLine) {
  lcd.setCursor(0, 0);
  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
}

String stateToString(StateType stateEnum) {
  switch (stateEnum) {
    case DISABLED:
      return "DISABLED";
      break;
    case IDLE:
      return "IDLE";
      break;
    case RUNNING:
      return "RUNNING";
      break;
    case ERROR:
      return "ERROR";
      break;
  }
}

void updateTempHumidReading() {
  int chk = DHT.read11(HUMID_TEMP_PIN);
}

void checkWaterLevel() {
  if (waterLevel <= WATER_THRESHOLD) {
    changeState(ERROR);
  }
}

void updateLEDS() {
  switch (state) {
    case DISABLED:
      digitalWrite(YELLOW_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(BLUE_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
      break;
    case IDLE:
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(BLUE_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
      break;
    case RUNNING:
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(BLUE_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, LOW);
      break;
    case ERROR:
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(BLUE_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);
      break;
  }
}

String getCurrentTime() {
  String output = "";
  switch (RTC.getWeek()) {
    case 1:
      output += "SUN";
      break;
    case 2:
      output += "MON";
      break;
    case 3:
      output += "TUE";
      break;
    case 4:
      output += "WED";
      break;
    case 5:
      output += "THU";
      break;
    case 6:
      output += "FRI";
      break;
    case 7:
      output += "SAT";
      break;
  }
  output += (" ");
  output += RTC.getDay();
  output += "-";
  output += RTC.getMonth();
  output += "-";
  output += RTC.getYear();
  output += " ";
  output += RTC.getHours();
  output += ":";
  output += RTC.getMinutes();
  output += ":";
  output += RTC.getSeconds();
  if (RTC.getHourMode() == CLOCK_H12) {
    switch (RTC.getMeridiem()) {
      case HOUR_AM:
        output += " AM";
        break;
      case HOUR_PM:
        output += " PM";
        break;
    }
  }
  output += "\n";
  return output;
}