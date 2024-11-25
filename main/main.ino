//Tyler Olson & Khyber Johnson
#include <I2C_RTC.h>
#include <Stepper.h>
#include <LiquidCrystal.h>
#include <dht.h>
#define TBE 0x20

static dht DHT;
static DS1307 RTC;

//Serial registers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int *myUBRR0 = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char *)0x00C6;

volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;

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
const int WATER_THRESHOLD = 100;

void setup() {
  U0init(9600);
  lcd.begin(16, 2);
  lcd.clear();

  myStepper.setSpeed(10);

  // LEDs
  DDRB |= (1 << PB4);  // Set Pin 10 (PB4) as output
  DDRB |= (1 << PB5);  // Set Pin 11 (PB5) as output
  DDRB |= (1 << PB6);  // Set Pin 12 (PB6) as output
  DDRH |= (1 << PH6);  // Set Pin 9 (PH6) as output

  // DC motor
  DDRE |= (1 << PE4);  // Set pin 2 (PE4) as output

  // Off button
  DDRH &= ~(1 << PH5);  // Set Pin 8 (PH5) as input
  PORTH |= (1 << PH5);  // Enable pull-up resistor on Pin 8 (PH5)

  // On button
  DDRH &= ~(1 << PH4);  // Set Pin 7 (PH4) as input
  PORTH |= (1 << PH4);  // Enable pull-up resistor on Pin 7 (PH4)

  // Clockwise button
  DDRC &= ~(1 << PC4);  // Set Pin 33 (PC4) as input
  PORTC |= (1 << PC4);  // Enable pull-up resistor on Pin 33 (PC4)

  // Counterclockwise button
  DDRC &= ~(1 << PC2);  // Set Pin 35 (PC2) as input
  PORTC |= (1 << PC2);  // Enable pull-up resistor on Pin 35 (PC2)

  // Get initial temp value
  updateTempHumidReading();

  // Init for analog read
  adc_init();
}

void loop() {
  updateLEDS();
  handleStateButtons();

  waterLevel = adc_read(15);
  unsigned long currentMillis = millis();

  switch (state) {
    case DISABLED:  // Fan off, yellow led on
      printLCD("DISABLED", "");
      PORTE &= ~(1 << PE4);  // Set DC fan low
      break;
    case IDLE:  // Fan off, green led on
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));

      if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
        previousMillis = currentMillis;

        updateTempHumidReading();
      }

      handleStepperButtons();
      checkWaterLevel();

      PORTE &= ~(1 << PE4);  // Set dc fan low

      if (DHT.temperature > TEMP_THRESHOLD) {
        changeState(RUNNING);
      }
      break;
    case ERROR:  // Red led on
      printLCD("Water level is", "too low!");

      checkWaterLevel();

      PORTE &= ~(1 << PE4);  // Set dc fan low
      break;
    case RUNNING:  // Fan on, blue led on
      printLCD("Temp: " + String(DHT.temperature), "Humidity:" + String(DHT.humidity));
      if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
        previousMillis = currentMillis;

        updateTempHumidReading();
      }

      handleStepperButtons();
      checkWaterLevel();

      PORTE |= (1 << PE4);  // Set dc fan high

      if (DHT.temperature <= TEMP_THRESHOLD) {
        changeState(IDLE);
      }
      break;
  };
}

void handleStateButtons() {
  if (!(PINH & (1 << PH4))) { // Pin 7 low
    changeState(DISABLED);
  } else if (!(PINH & (1 << PH5))) { // Pin 8 low
    changeState(IDLE);
  }
}

void handleStepperButtons() {
  while (!(PINC & (1 << PC4)) && (PINC & (1 << PH2))) { // Pin 33 low, 35 high
    myStepper.step(64);
  }

  while ((PINC & (1 << PC4)) && !(PINC & (1 << PH2))) { // Pin 33 high, 35 low
    myStepper.step(-64);
  }
}

void changeState(StateType newState) {
  if (newState == state) {
    return;
  }

  if (newState == RUNNING) {
    String text = "Fan turned ON at: " + getCurrentTime();
    U0print(text.c_str());
  } else if (state == RUNNING) {
    String text = "Fan turned OFF at: " + getCurrentTime();
    U0print(text.c_str());
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
      PORTB |= (1 << PB4); // Pin 10
      PORTB &= ~(1 << PB5); // Pin 11
      PORTB &= ~(1 << PB6); // Pin 12
      PORTH &= ~(1 << PH6); // Pin 9
      break;
    case IDLE:
      PORTB &= ~(1 << PB4); // Pin 10
      PORTB |=  (1 << PB5); // Pin 11
      PORTB &= ~(1 << PB6); // Pin 12
      PORTH &= ~(1 << PH6); // Pin 9
      break;
    case RUNNING:
      PORTB &= ~(1 << PB4); // Pin 10
      PORTB &= ~(1 << PB5); // Pin 11
      PORTB |= (1 << PB6); // Pin 12
      PORTH &= ~(1 << PH6); // Pin 9
      break;
    case ERROR:
      PORTB &= ~(1 << PB4); // Pin 10
      PORTB &= ~(1 << PB5); // Pin 11
      PORTB &= ~(1 << PB6); // Pin 12
      PORTH |= (1 << PH6); // Pin 9
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

void adc_init() {
  // setup the A register
  *my_ADCSRA |= 0b10000000;  // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111;  // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111;  // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000;  // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111;  // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000;  // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX &= 0b01111111;  // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX |= 0b01000000;  // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX &= 0b11011111;  // clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11100000;  // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num) {
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if (adc_channel_num > 7) {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while ((*my_ADCSRA & 0x40) != 0)
    ;
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

void U0init(int U0baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  // Same as (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

void U0putchar(unsigned char U0pdata) //Function to write a character to the USART0 transmit buffer
{
    // 
    while (!(*myUCSR0A & TBE)) {} //Waits for the transmit buffer to be empty and disable until the transmit buffer is empty

    *myUDR0 = U0pdata; //Writes the char to the transmit buffer
}

void U0print(const char* str) {
  while (*str) {
    U0putchar(*str++);
  }
}

void U0sendHexASCII(unsigned char U0pdata) //Converts and sends the ASCII char in hexadecimal format
{
    char hexString[6]; //Buffer to store "0xYY\n"
    sprintf(hexString, "0x%02X\n", U0pdata); //Format the string with the hex value of U0pdata
    for (int i = 0; hexString[i] != '\0'; i++) {
        U0putchar(hexString[i]); //Send each character of the string
    }
}