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

enum StateType {
  DISABLED,
  IDLE,
  RUNNING,
  ERROR
};

StateType state = IDLE;

int tempThreshold = 22;
int waterThreshold = 25;

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
}

void loop() {
  updateLEDS();




  switch (state) {
    case DISABLED:  //fan off, yellow led on
      printLCD("DISABLED", "");
      //fan off
      break;
    case IDLE:  //fan off, green led on
      updateTempHumidReading();
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      digitalWrite(dcfanPin, LOW);

      if (DHT.temperature > tempThreshold) {
        state = RUNNING;
      }
      break;
    case ERROR:  //red led on
      printLCD("Water level is too low", "");
      break;
    case RUNNING:  //fan on, blue led on
      updateTempHumidReading();
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      digitalWrite(dcfanPin, HIGH);

      if (DHT.temperature <= tempThreshold) {
        state = IDLE;
      }

      // if (waterLevel < waterThreshold) {state = ERROR;}
      break;
  };

  delay(100);
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