//Tyler Olson & Khyber Johnson
//CPE 301 Final Project group 47
//Swamp Cooler Code

#include <LiquidCrystal.h> //LCD Arduino Library
#include <dht.h>  //DC Motor Arduino Library
#include <I2C_RTC.h> //Real Time Clock Arduino Library
#include <Stepper.h> //Stepper Motor Arduino Library

dht DHT;
static DS1307 RTC;

const int stepsPerRevolution = 2048;
//Creates an instance of stepper class
//Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
Stepper myStepper = Stepper(stepsPerRevolution, 23, 27, 25, 29);

//LCD pins
const int RS = 50, EN = 52, D4 = 40, D5 = 42, D6 = 44, D7 = 46;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//Temp sensor pin
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
int clockwisePin = 33;
int counterclockwisePin = 35;
int waterLevel = 0;

volatile bool onButtonPressed = false;
volatile bool offButtonPressed = false;

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

  //Set LED pins as OUTPUT (DDRx corresponds to the data direction register)
  DDRB |= (1 << DDB1); //Pin 9
  DDRB |= (1 << DDB2); //Pin 10
  DDRB |= (1 << DDB3); //Pin 11
  DDRB |= (1 << DDB4); //Pin 12

  //Set DC fan pin as OUTPUT
  DDRD |= (1 << DDD2); //Pin 2

  //Set On/Off buttons as INPUT_PULLUP
  DDRD &= ~(1 << DDD7); //Pin 7 (INPUT)
  PORTD |= (1 << PORTD7); //Enable pull-up resistor

  DDRB &= ~(1 << DDB0); //Pin 8 (INPUT)
  PORTB |= (1 << PORTB0); //Enable pull-up resistor

  //Set clockwise and counterclockwise pins as INPUT_PULLUP
  DDRA &= ~(1 << DDA1); //Pin 33 (INPUT)
  PORTA |= (1 << PORTA1); //Enable pull-up resistor

  DDRA &= ~(1 << DDA3); //Pin 35 (INPUT)
  PORTA |= (1 << PORTA3); //Enable pull-up resistor

  while (!Serial) {
    ;  //wait for serial port to connect. Needed for native USB port only
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

  //Attach interrupts to On/Off buttons
  attachInterrupt(digitalPinToInterrupt(onButtonPin), onButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(offButtonPin), offButtonISR, FALLING);
}




void loop() {
  updateLEDS();

  if (customDigitalRead(onButtonPin) == LOW) {
    changeState(DISABLED);
    onButtonPressed = false;

  } else if (customDigitalRead(offButtonPin) == LOW) {
    changeState(IDLE);
    offButtonPressed = false;  // Reset flag
  }

  while (customDigitalRead(clockwisePin) == LOW && customDigitalRead(counterclockwisePin) == HIGH) {
    myStepper.step(64);
  }

  while (customDigitalRead(clockwisePin) == HIGH && customDigitalRead(counterclockwisePin) == LOW) {
    myStepper.step(-64);
  }


  waterLevel = customAnalogRead(waterSensorPin);


  switch (state) {
    case DISABLED:  //fan off, yellow led on
      printLCD("DISABLED", "");
      customDigitalWrite(dcfanPin, LOW);
      break;
    case IDLE:  //fan off, green led on
      updateTempHumidReading();
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      checkWaterLevel();
      customDigitalWrite(dcfanPin, LOW);

      if (DHT.temperature > tempThreshold) {
        changeState(RUNNING);
      }
      break;
    case ERROR:  //red led on
      checkWaterLevel();
      printLCD("Water level is", "too low!");
      customDigitalWrite(dcfanPin, LOW);
      break;
    case RUNNING:  //fan on, blue led on
      updateTempHumidReading();
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      if (DHT.temperature <= tempThreshold) {
        changeState(IDLE);
      }
      customDigitalWrite(dcfanPin, HIGH);
      checkWaterLevel();

      break;
  };


  customDelay(60000); //1 Minute update cycle
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
      customDigitalWrite(yellowLEDPin, HIGH);
      customDigitalWrite(greenLEDPin, LOW);
      customDigitalWrite(blueLEDPin, LOW);
      customDigitalWrite(redLEDPin, LOW);
      break;
    case IDLE:
      customDigitalWrite(yellowLEDPin, LOW);
      customDigitalWrite(greenLEDPin, HIGH);
      customDigitalWrite(blueLEDPin, LOW);
      customDigitalWrite(redLEDPin, LOW);
      break;
    case RUNNING:
      customDigitalWrite(yellowLEDPin, LOW);
      customDigitalWrite(greenLEDPin, LOW);
      customDigitalWrite(blueLEDPin, HIGH);
      customDigitalWrite(redLEDPin, LOW);
      break;
    case ERROR:
      customDigitalWrite(yellowLEDPin, LOW);
      customDigitalWrite(greenLEDPin, LOW);
      customDigitalWrite(blueLEDPin, LOW);
      customDigitalWrite(redLEDPin, HIGH);
      break;
  }
}

void customDigitalWrite(uint8_t pin, uint8_t value) {
  if (pin >= 0 && pin <= 7) {
    if (value == HIGH) {
      PORTD |= (1 << pin);
    } else {
      PORTD &= ~(1 << pin);
    }
  } else if (pin >= 8 && pin <= 13) {
    uint8_t bit = pin - 8;
    if (value == HIGH) {
      PORTB |= (1 << bit);
    } else {
      PORTB &= ~(1 << bit);
    }
  } else if (pin >= A0 && pin <= A7) {
    uint8_t bit = pin - A0;
    if (value == HIGH) {
      PORTC |= (1 << bit);
    } else {
      PORTC &= ~(1 << bit);
    }
  }
}

uint8_t customDigitalRead(uint8_t pin) {
  if (pin >= 0 && pin <= 7) {
    return (PIND & (1 << pin)) ? HIGH : LOW;
  } else if (pin >= 8 && pin <= 13) {
    uint8_t bit = pin - 8;
    return (PINB & (1 << bit)) ? HIGH : LOW;
  } else if (pin >= A0 && pin <= A7) {
    uint8_t bit = pin - A0;
    return (PINC & (1 << bit)) ? HIGH : LOW;
  }
  return LOW;
}

int customAnalogRead(uint8_t pin) {
  if (pin >= A0 && pin <= A7) {
    pin -= A0;
  } else if (pin > 7) {
    return 0;
  }

  ADMUX = (ADMUX & 0xF0) | (pin & 0x0F);

  ADCSRA |= (1 << ADSC);

  while (ADCSRA & (1 << ADSC));

  return ADC;
}

void customDelay(unsigned long milliseconds) {
  unsigned long iterations = milliseconds * (F_CPU / 16000); 

  for (unsigned long i = 0; i < iterations; i++) {
    __asm__ __volatile__("nop");
  }
}

// Interrupt Service Routine for the onButtonPin
void onButtonISR() {
  onButtonPressed = true; // Set the flag to indicate that the button was pressed
}

// Interrupt Service Routine for the offButtonPin
void offButtonISR() {
  offButtonPressed = true; // Set the flag to indicate that the button was pressed
}

