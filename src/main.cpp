#include <Arduino.h>

#define LED 13 

void setup()
{
    // initialize serial
    Serial.begin(115200);
    
    pinMode(LED, OUTPUT);
}

// the loop function runs over and over again forever
void loop()
{
    Serial.println("ON");
    digitalWrite(LED, HIGH);    // turn the LED on (HIGH is the voltage level)
    delay(500);                 // wait for a second
    Serial.println("OFF");
    digitalWrite(LED, LOW);     // turn the LED off by making the voltage LOW
    delay(500);                 // wait for a second
}
