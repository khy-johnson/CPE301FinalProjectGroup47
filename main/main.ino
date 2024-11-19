//Tyler Olson & Khyber Johnson
#include <LiquidCrystal.h>
#include <dht.h>  //install the DHTLib library
dht DHT;

#include <I2C_RTC.h>

static DS1307 RTC;



#include <Stepper.h>
const int stepsPerRevolution = 2048;
// Creates an instance of stepper class
// Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
Stepper myStepper = Stepper(stepsPerRevolution, 23, 27, 25, 29);

// LCD pins
const int RS = 50, EN = 52, D4 = 40, D5 = 42, D6 = 44, D7 = 46;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Temp sensor pin
const int humidTempPin = 3;

//led pins
const int yellowLEDPin = 10;
const int greenLEDPin = 11;
const int blueLEDPin = 12;
const int redLEDPin = 9;

const int dcfanPin = 2;

const int offButtonPin = 8;
const int onButtonPin = 7;

const int waterSensorPin = A15;

int waterLevel = 0;

int clockwisePin = 33;
int counterclockwisePin = 35;

enum StateType {
  DISABLED,
  IDLE,
  RUNNING,
  ERROR
};

StateType state = IDLE;


int tempThreshold = 22;
int waterThreshold = 100;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();

  myStepper.setSpeed(10);

  //init LEDs
  pinMode(yellowLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(blueLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);

  pinMode(dcfanPin, OUTPUT);
  pinMode(offButtonPin, INPUT_PULLUP);
  pinMode(onButtonPin, INPUT_PULLUP);
  pinMode(clockwisePin, INPUT_PULLUP);
  pinMode(counterclockwisePin, INPUT_PULLUP);

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  if (RTC.begin() == false) {
    Serial.println("RTC Not Connected!");
    while (true)
      ;
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

void loop() {
  updateLEDS();

  if (digitalRead(onButtonPin) == LOW) {
    changeState(DISABLED);

  } else if (digitalRead(offButtonPin) == LOW) {
    changeState(IDLE);
  }

  while (digitalRead(clockwisePin) == LOW && digitalRead(counterclockwisePin) == HIGH) {
    myStepper.step(64);
  }

  while (digitalRead(clockwisePin) == HIGH && digitalRead(counterclockwisePin) == LOW) {
    myStepper.step(-64);
  }


  waterLevel = analogRead(waterSensorPin);

  //Serial.print("Water level:");
  //Serial.println(waterLevel);

  switch (state) {
    case DISABLED:  //fan off, yellow led on
      printLCD("DISABLED", "");
      digitalWrite(dcfanPin, LOW);
      break;
    case IDLE:  //fan off, green led on
      updateTempHumidReading();
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      checkWaterLevel();
      digitalWrite(dcfanPin, LOW);

      if (DHT.temperature > tempThreshold) {
        changeState(RUNNING);
      }
      break;
    case ERROR:  //red led on
      checkWaterLevel();
      printLCD("Water level is", "too low!");
      digitalWrite(dcfanPin, LOW);
      break;
    case RUNNING:  //fan on, blue led on
      updateTempHumidReading();
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      if (DHT.temperature <= tempThreshold) {
        changeState(IDLE);
      }
      digitalWrite(dcfanPin, HIGH);
      checkWaterLevel();

      break;
  };


  delay(100);
}

void changeState(StateType newState) {
  if (newState == state) {
    return;
  }

  Serial.print("New State: " + stateToString(newState) + ", at: " + getCurrentTime());
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
  int chk = DHT.read11(humidTempPin);
}

void checkWaterLevel() {
  if (waterLevel <= waterThreshold) {
    changeState(ERROR);
  }
}

void updateLEDS() {
  switch (state) {
    case DISABLED:
      digitalWrite(yellowLEDPin, HIGH);
      digitalWrite(greenLEDPin, LOW);
      digitalWrite(blueLEDPin, LOW);
      digitalWrite(redLEDPin, LOW);
      break;
    case IDLE:
      digitalWrite(yellowLEDPin, LOW);
      digitalWrite(greenLEDPin, HIGH);
      digitalWrite(blueLEDPin, LOW);
      digitalWrite(redLEDPin, LOW);
      break;
    case RUNNING:
      digitalWrite(yellowLEDPin, LOW);
      digitalWrite(greenLEDPin, LOW);
      digitalWrite(blueLEDPin, HIGH);
      digitalWrite(redLEDPin, LOW);
      break;
    case ERROR:
      digitalWrite(yellowLEDPin, LOW);
      digitalWrite(greenLEDPin, LOW);
      digitalWrite(blueLEDPin, LOW);
      digitalWrite(redLEDPin, HIGH);
      break;
  }
}