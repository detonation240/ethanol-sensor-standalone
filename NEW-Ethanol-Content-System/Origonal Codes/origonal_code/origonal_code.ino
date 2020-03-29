//Flex Fuel sensor code
//Input frequency on pin 8 - pulled up to 5V through 1.8K resistor
//V_out pin 3 - 0-5V output through 1.8K resistor with 10uf capacitor to ground
//PLX iMFD serial packet on Tx pin 0 through 3.3V TTL voltage divider (3.3K/6.8K)

#include <Arduino.h>
#include <FreqMeasure.h>

double sum = 0;
int count = 0;
int ethanol_int, V_out_int;
float freq, ethanol, V_out, E_scalar;
float E0 = 50; //calibration data for pure gasoline
float E85 = 135; //calibration data for E85
long P0, P1, Pdelta;
byte PLXpacket[7] {0x80, 0x00, 0x11, 0x00, 0x00, 0x00, 0x40};

//PLX iMFD packet, sent every 100ms:
//0x80      Start byte
//0x00 0x11 Sensor device fuel level
//0x00      Sensor instance
//0x00 0x00 Sensor value (integer, unscaled)
//0x40      Stop byte

void setup()
{
  pinMode (3, OUTPUT);
  FreqMeasure.begin();
  Serial.begin(19200);
  P0 = millis();
  E_scalar = (E85 - E0) / 85;
}

void loop()
{
  if (FreqMeasure.available())
  {
    // average several readings together
    sum = sum + FreqMeasure.read();
    count = count + 1;
    if (count > 30)
    {
      freq = FreqMeasure.countToFrequency(sum / count);
      sum = 0;
      count = 0;
    }
  }

  ethanol = (freq - E0) / E_scalar; //scale frequency to E% interpolating E0 and E85 values
  if (ethanol > 100)
  {
    ethanol = 100;
  }
  if (ethanol < 0)
  {
    ethanol = 0;
  }
  ethanol_int = (int)ethanol;

  V_out = 5 - (0.05 * ethanol);
  V_out = 51 * V_out; //scale to 255
  V_out_int = (int)V_out; //convert to integer for analogWrite

  analogWrite(3, V_out_int); //output V_out as PWM voltage on pin 3

  P1 = millis(); //send PLX packet on Tx pin every 100ms
  Pdelta = P1 - P0;
  if (Pdelta >= 100)
  {
    P0 = P1;
    PLXpacket[5] = ethanol_int; //set data byte in PLX packet to E%
    Serial.write(PLXpacket, 7);
  }
}
