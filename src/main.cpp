/*
/* This code assumes following pin layout for the Arduino Nano V3:

* +--------+--------+----------+----------------------+----------+
* |Name    |GPIO    |Signal    |Funktionsbaustein     |Direction |
* +--------+--------+----------+----------------------+----------+
* | D0     | PD0    | Rx       | UART                 | I        |
* | D1     | PD1    | Tx       | UART                 | O        |
* | D2     | PD2    | CE       | EEPROM               | O        |
* | D3     | PD3    | A9_VPE   | EEPROM               | O        |
* | D4     | PD4    | OE       | EEPROM               | O        |
* | D5     | PD5    | OE_VPP   | EEPROM               | O        |
* | D6     | PD6    | Data_6   | EEPROM               | I/O      |
* | D7     | PD7    | Data_7   | EEPROM               | I/O      |
* | D8     | PB0    | VPP_VPE  | Umschaltung VPP-VPE  | O        |
* | D9     | PB1    | LATCH    | Latch_595            | O        |
* | D10    | PB2    | SS       | SD_CS                | O        |
* | D11    | PB3    | MOSI     | SD_MOSI / Data_595   | O        |
* | D12    | PB4    | MISO     | SD_Miso              | I        |
* | D13    | PB5    | SCK      | SD_CLK / Clock_|595  | O        |
* | A0     | PC0    | Data_0   | EEPROM               | I/O      |
* | A1     | PC1    | Data_1   | EEPROM               | I/O      |
* | A2     | PC2    | Data_2   | EEPROM               | I/O      |
* | A3     | PC3    | Data_3   | EEPROM               | I/O      |
* | A4     | PC4    | Data_4   | EEPROM               | I/O      |
* | A5     | PC5    | Data_5   | EEPROM               | I/O      |
* +--------+--------+----------+----------------------+----------+
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#define LED 13
#define LATCH 9
char buffer[256];

void eepromSetAddress(uint16_t address)
{
    // todo: Adressbereich des EEPROMS prüfen und address ggf. maskieren
    digitalWrite(LATCH, LOW);
    SPI.transfer16(address);
    digitalWrite(LATCH, LOW);
}

void eepromBeginRead()
{

}

void eepromEndRead()
{

}
uint8_t eepromReadByte()
{

}

void setup()
{
    // initialize serial
    Serial.begin(115200);
    
    //pinMode(LED, OUTPUT);
        
    pinMode(SS, OUTPUT);
    digitalWrite(SS, HIGH); 
    
    pinMode(LATCH, OUTPUT);
    digitalWrite(LATCH, HIGH);
    

    SPI.begin();
    //SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    if ( !SD.begin(SS) )
        Serial.println("Initialization failed");

    if (SD.exists(const_cast<char *>("test.txt")) )
        Serial.println("ok");
    else
        Serial.println("fail");

    File f = SD.open("test.txt");
    f.seek(75);
    int len = f.readBytes(buffer, 20);
    buffer[len] = 0;
    Serial.println(buffer);
    f.close();    
}

// the loop function runs over and over again forever
void loop()
{
    for (int i = 0; i <= 3; i++)
    {

        digitalWrite(LATCH, LOW);  // Disable any internal transference in the SN74HC595
        SPI.transfer(i);           // Transfer data to the SN74HC595
        digitalWrite(LATCH, HIGH); // Start the internal data transference in the SN74HC595
        delay(1000);               // Wait for next iteration
    }
}
