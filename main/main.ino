//Tyler Olson & Khyber Johnson
#include <LiquidCrystal.h>
#include <dht.h>  //install the DHTLib library
dht DHT;

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

enum StateType {
  DISABLED,
  IDLE,
  RUNNING,
  ERROR
};

StateType state = IDLE;


int tempThreshold = 20;
int waterThreshold = 100;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();

  //init LEDs
  pinMode(yellowLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(blueLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);

  pinMode(dcfanPin, OUTPUT);
  pinMode(offButtonPin, INPUT_PULLUP);
  pinMode(onButtonPin, INPUT_PULLUP);
}

void loop() {
  updateLEDS();

  if (digitalRead(onButtonPin) == LOW) {
    changeState(DISABLED);

  } else if (digitalRead(offButtonPin) == LOW) {
    changeState(IDLE);
  }

  waterLevel = analogRead(waterSensorPin);

  Serial.print("Water level:");
  Serial.println(waterLevel);


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

      digitalWrite(dcfanPin, HIGH);
      checkWaterLevel();

      break;
  };


  delay(100);
}

void changeState(StateType newState) {
  state = newState;
  lcd.clear();
}

void printLCD(String firstLine, String secondLine) {
  lcd.setCursor(0, 0);
  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
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