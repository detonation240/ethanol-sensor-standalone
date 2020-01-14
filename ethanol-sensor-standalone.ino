/*
Aaron S. (Detonation) 2020
Parts: Arduino Nano, 0.96 OLED 4 Pin, 4.7K & 3.3k Ohm resistor 0.1uF Capacitor, GM PN#13577429 or compatible GM Flex sensor
OLED Display
GND - GND
VCC - 5V
A4 - SDA
A5 - SLC

Flex Fuel Sensor
D8 - Flex input (Must use 4.7kohm resistor with 5v divider from Arduino) 
GND - Vehicle GND
VCC - Vehicle 12v IGN Switched

Flex Sensor ECU Out
D11 - To ECU (D11 to One lead of Cap, Other Lead of Cap to Ground) (One lead of 3.3k Resistor to SAME lead of cap as D11, Other lead of resistor is 0-5v out)
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

int inpPin = 8;
int outPin = 11;    //define PWM output, possible pins with LCD and 32khz freq. are 3 and 11 (Nano and Uno)
volatile uint16_t revTick;    //Ticks per revolution
uint16_t pwm_output  = 0;      //integer for storing PWM value (0-255 value)
int HZ = 0;                  //unsigned 16bit integer for storing HZ input
int ethanol = 0;              //Store ethanol percentage here
float expectedv;              //store expected voltage here - range for typical GM sensors is usually 0.5-4.5v
uint16_t voltage = 0;              //store display millivoltage here (0-5000)
//temperature variables
int duty;                     //Duty cycle (0.0-100.0)
float period;                 //Store period time here (eg.0.0025 s)
float temperature = 0;        //Store fuel temperature here
int fahr = 0;
int cels = 0;
int celstemp = 0;
float fahrtemp = 0;
static long highTime = 0;
static long lowTime = 0;
static long tempPulse;

void setupTimer()   // setup timer1
{           
  TCCR1A = 0;      // normal mode
  TCCR1B = 132;    // (10000100) Falling edge trigger, Timer = CPU Clock/256, noise cancellation on
  TCCR1C = 0;      // normal mode
  TIMSK1 = 33;     // (00100001) Input capture and overflow interupts enabled

  TCNT1 = 0;       // start from 0
}
ISR(TIMER1_CAPT_vect)    // PULSE DETECTED!  (interrupt automatically triggered, not called by main program)
{
  revTick = ICR1;      // save duration of last revolution
  TCNT1 = 0;       // restart timer for next revolution
}
ISR(TIMER1_OVF_vect)    // counter overflow/timeout
{ revTick = 0; }        // Ticks per second = 0




void setup() {
  pinMode(inpPin,INPUT);
  setPwmFrequency(outPin,1); //Modify frequency on PWM output
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  //display.drawPixel(10, 10, WHITE);
  display.display();
  delay(1000);

  setupTimer();
  Serial.begin(9600);
}


void loop() {
  
  getfueltemp(inpPin); //read fuel temp from input duty cycle
  
  if (revTick > 0) // Avoid dividing by zero, sample in the HZ
      {HZ = 62200 / revTick;}     // 3456000ticks per minute, 57600 per second 
      else                        // 62200 calibrated for more accuracy
      {HZ = 0;}                   
  
    //calculate ethanol percentage
      if (HZ > 50) // Avoid dividing by zero
      {ethanol = HZ-50;}
      else
      {ethanol = 0;}
  
  if (ethanol > 99) // Avoid overflow in PWM
  {ethanol = 99;}

  expectedv = ((((HZ-50.0)*0.01)*4)+0.5);
/////////////////////////ECU OUT///////////////////////////////////////////////////////
  
  //calculations
  pwm_output = 1.1 * (255 * (expectedv/5.0)); //calculate output PWM for ECU
  //PWM output
  analogWrite(outPin, pwm_output); //write the PWM value to output pin

/////////////////////////////////////////////////
///////////////////////////////////////////////// DISPLAY ///////
/////////////////////////////////////////////////
///////////////////Title & EGT////////////////////////  
      display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("    DetonationEMS");
  display.setCursor(0,8);
  display.setTextSize(2);
///////////////////E85//////////////////////////
    //display.setTextSize(1);
    display.print("Eth:  ");
    display.print(ethanol);
    display.println("%");
    display.setTextSize(1);
    display.print("Fuel Temp:   ");
    display.print(fahr);
    display.print("F");
    display.display();

    delay(500);
}

void getfueltemp(int inpPin){ //read fuel temp from input duty cycle
  highTime = 0;
  lowTime = 0;
  
  tempPulse = pulseIn(inpPin,HIGH);
    if(tempPulse>highTime){
    highTime = tempPulse;
    }
  
  tempPulse = pulseIn(inpPin,LOW);
    if(tempPulse>lowTime){
    lowTime = tempPulse;
    }
  
  duty = ((100*(highTime/(double (lowTime+highTime))))); //Calculate duty cycle (integer extra decimal)
  float T = (float(1.0/float(HZ)));             //Calculate total period time
  float period = float(100-duty)*T;             //Calculate the active period time (100-duty)*T
  float temp2 = float(10) * float(period);      //Convert ms to whole number
  temperature = ((40.25 * temp2)-81.25);        //Calculate temperature for display (1ms = -40, 5ms = 80)
  celstemp = int(temperature);
  cels = celstemp;
  fahrtemp = ((temperature*1.8)+32);
  fahr = fahrtemp;
}

void setPwmFrequency(int pin, int divisor) { //This code snippet raises the timers linked to the PWM outputs
  byte mode;                                 //This way the PWM frequency can be raised or lowered. Prescaler of 1 sets PWM output to 32KHz (pin 3, 11)
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
