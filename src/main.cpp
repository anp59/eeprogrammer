/*
* This code assumes following pin layout for the Arduino Nano V3:

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
#include "pin_definitions.hpp"

#define LED 13
//#define LATCH 9
uint8_t buffer[256];

void hexDump(const char *desc, void *addr, unsigned int offset, int len);

DECLARE_PIN (CE_pin, D, 2);
DECLARE_PIN (A9_VPE_pin, D, 3);
DECLARE_PIN (OE_pin, D, 4);
DECLARE_PIN (OE_VPP_pin, D, 5);
DECLARE_PIN_GROUP(DATA_low, C, 0, 6);
DECLARE_PIN_GROUP(DATA_high, D, 6, 2);
DECLARE_PIN(LATCH_pin, B, 1)

bool is_writing = false;



void enable_A9_HV()
{
    write(A9_VPE_pin, 1);
}
void disable_A9_HV()
{
    write(A9_VPE_pin, 0);
}

void enable_OE_VPP()
{
    write(OE_VPP_pin, 1);
    write(OE_pin, 1);
}
void disable_OE_VPP()
{
    write(OE_VPP_pin, 0);
    write(OE_pin, 0);
}

void eeprom_init_pins()
{
    make_output(CE_pin);
    make_output(A9_VPE_pin);
    disable_A9_HV();
    make_output(OE_pin);
    make_output(OE_VPP_pin);
    disable_OE_VPP();
}
void eeprom_output_enable() { 
    write(OE_pin, 0); 
}
void eeprom_output_disable() { 
    write(OE_pin, 1); 
    delayMicroseconds(1); 
}
void eeprom_chip_select() { 
    write(CE_pin, 0); 
}
void eeprom_chip_deselect() { 
    write(CE_pin, 1); 
    delayMicroseconds(1); 
}
void eeprom_set_data_out() { 
    make_output(DATA_low);  
    make_output(DATA_high); 
}
void eeprom_set_data_in() { // ohne pullup
    //make_input_with_pull_ups(DATA_low);
    //make_input_with_pull_ups(DATA_high);
    make_input_without_pull_ups(DATA_low);
    make_input_without_pull_ups(DATA_high); 
}

uint8_t eeprom_data_in() 
{ 
    return read(DATA_low) + (read(DATA_high) << DATA_high.shift); 
}
void eeprom_data_out(uint8_t byte) { 
    write(DATA_low, byte & DATA_low.mask );     // mask = 0x3F, shift = 0
    write(DATA_high, byte >> DATA_high.shift);  // mask = 0xC0, shift = 6
}

void eeprom_set_address(uint16_t address)
{
    // todo: Adressbereich des EEPROMS prüfen und address ggf. maskieren
    
    write(LATCH_pin, 0);//digitalWrite(LATCH, LOW);
    SPI.transfer16(address);
    write(LATCH_pin, 1);//digitalWrite(LATCH, HIGH);
    delayMicroseconds(1);
    write(LATCH_pin, 0);
}

void begin_write()
{
    eeprom_output_disable();
    is_writing = true;
    eeprom_set_data_out();
}

void end_write()
{
    is_writing = false;
    eeprom_set_data_in();
}

void begin_read()
{
    end_write();
    eeprom_output_enable();
}

void end_read()
{
    eeprom_output_disable();
}

uint8_t eeprom_read_byte()
{
    begin_read();
    eeprom_chip_select();
    uint8_t byte = eeprom_data_in();
    eeprom_chip_deselect();
    end_read();
    return byte;
}

void eeprom_read_to_buffer(uint16_t address)
{
    // todo: max Adresse pruefen
    unsigned int i = 0;
    uint16_t offset = address;
    eeprom_chip_deselect();
    begin_read();
    do
    {
        // flow_control_t status;
        // do
        // {
        //     status = uart_check_flow_control();
        //     if (status == FLOW_BREAK)
        //     {
        //         uart_puts_P(PSTR("! Read aborted\n"));
        //         goto abort_reading;
        //     }
        // } while (status == FLOW_PAUSE);

        eeprom_set_address(address);
        eeprom_chip_select();
        ++address;
        buffer[i++] = eeprom_data_in();
        eeprom_chip_deselect();
        
    } while (i < sizeof(buffer));
    end_read();
    Serial.println(PSTR("! Read end\n"));
    hexDump("EEPROM Read", buffer, offset, sizeof(buffer));
}

void setup()
{
    // initialize serial
    Serial.begin(115200);
    
    //pinMode(LED, OUTPUT);
    eeprom_init_pins();
    eeprom_set_data_in();
    // uint8_t data = eeprom_data_in();
    // eeprom_set_data_out();
    // eeprom_data_out(data);


    pinMode(SS, OUTPUT);
    digitalWrite(SS, HIGH); 
    
    make_output(LATCH_pin);
    //pinMode(LATCH, OUTPUT);
    write(LATCH_pin, 1);
    //digitalWrite(LATCH, HIGH);


    SPI.begin();
    //SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));

    if ( !SD.begin(SS) )
        Serial.println("Initialization failed");

    if (SD.exists(const_cast<char *>("test.txt")) )
        Serial.println(PSTR("ok"));
    else
        Serial.println(PSTR("fail"));

    File f = SD.open("test.txt");
    f.seek(0);
    int len = f.readBytes(buffer, 20);
    buffer[len] = 0;
    Serial.println((char*)buffer);
    //hexDump("Hexdump", buffer, 0, len);
    f.close();
    eeprom_read_to_buffer(0x0000);
    eeprom_read_to_buffer(0x1000);
    eeprom_read_to_buffer(0x2000);
    eeprom_read_to_buffer(0xA000);
    eeprom_read_to_buffer(0xFF00);
}

// the loop function runs over and over again forever
void loop()
{
    // Serial.println("disable_OE_VPP()");     disable_OE_VPP();
    // delay(3000);
    // Serial.println("eeprom_output_enable()");     eeprom_output_enable();
    // delay(3000);
    // Serial.println("eeprom_output_disable()");  eeprom_output_disable();
    // delay(3000);
    // Serial.println("enable_OE_VPP()");    enable_OE_VPP();
    // delay(3000);
    // Serial.println("eeprom_output_enable()");    eeprom_output_enable();
    // delay(3000);
    // Serial.println("eeprom_output_disable()");    eeprom_output_disable();
    // delay(3000);
    // eeprom_set_address(0x703D);
    // delay(100);  
    // eeprom_set_address(0x8FC2);    
    // delay(100);  
    // for (int i = 0; i <= 3; i++)
    // {
        
        // eeprom_set_data_in();
        // Serial.print(eeprom_data_in(), HEX); Serial.print(" - ");
        // Serial.println(read(DATA_low), HEX);
        // Serial.println(DATA_low.mask, HEX);

        //write(LATCH_pin, 0);
        //digitalWrite(LATCH, LOW);  // Disable any internal transference in the SN74HC595
        //SPI.transfer(i);           // Transfer data to the SN74HC595
        //write(LATCH_pin, 1);
        //digitalWrite(LATCH, HIGH); // Start the internal data transference in the SN74HC595
        //delay(1000);               // Wait for next iteration
    // }

}

void hexDump(const char *desc, void *addr, unsigned int offset, int len)
{
    int i;
    unsigned char buff[17];
    char sprintfbuffer[12];
    unsigned char *pc = (unsigned char *)addr;

    if (len == 0)
        return;
    // Output description if given.
    if (desc != NULL) {
        //printf("%s:\n", desc);
        Serial.print(desc); Serial.println(":");
    }
    // Process every byte in the data.
    for (i = 0; i < len; i++)
    {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0)
        {
            // Just don't print ASCII for the zeroth line.
            if (i != 0) {
                //printf("  %s\n", buff);
                Serial.print(" | "); Serial.println((char*)buff);
            }
            // Output the offset.
            //printf("  %08x ", offset);
            sprintf(sprintfbuffer, "  0x%08x ", offset);
            Serial.print(sprintfbuffer);
            offset += 16;
        }

        // Now the hex code for the specific character.
        //printf(" %02x", pc[i]);
        sprintf(sprintfbuffer, " %02x", pc[i]);
        Serial.print(sprintfbuffer);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0)
    {
        //printf("   ");
        Serial.print("   ");
        i++;
    }

    // And print the final ASCII bit.
    //printf("  %s\n", buff);
    Serial.print(" | ");
    Serial.println((char *)buff);
}
