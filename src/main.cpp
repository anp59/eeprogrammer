/*
* This code assumes following pin layout for the Arduino Nano V3:

* +--------+--------+----------+----------------------+----------+------+
* |Name    |GPIO    |Signal    |Funktionsbaustein     |Direction | init |
* +--------+--------+----------+----------------------+----------+------+
* | D0     | PD0    | Rx       | UART                 | I        |      |
* | D1     | PD1    | Tx       | UART                 | O        |      |
* | D2     | PD2    | CE       | EEPROM               | O        |   H  |
* | D3     | PD3    | A9_VPE   | EEPROM               | O        |   L  |
* | D4     | PD4    | OE       | EEPROM               | O        |   H  |
* | D5     | PD5    | OE_VPP   | EEPROM               | O        |   L  |
* | D6     | PD6    | Data_6   | EEPROM               | I/O      |      |
* | D7     | PD7    | Data_7   | EEPROM               | I/O      |      |
* | D8     | PB0    | VPP_VPE  | Umschaltung VPP-VPE  | O        |   L  |
* | D9     | PB1    | LATCH    | Latch_595            | O        |   H  |
* | D10    | PB2    | SS       | SD_CS                | O        |   H  |
* | D11    | PB3    | MOSI     | SD_MOSI / Data_595   | O        |      |
* | D12    | PB4    | MISO     | SD_Miso              | I        |      |
* | D13    | PB5    | SCK      | SD_CLK / Clock_|595  | O        |      |
* | A0     | PC0    | Data_0   | EEPROM               | I/O      |      |
* | A1     | PC1    | Data_1   | EEPROM               | I/O      |      |
* | A2     | PC2    | Data_2   | EEPROM               | I/O      |      |
* | A3     | PC3    | Data_3   | EEPROM               | I/O      |      |
* | A4     | PC4    | Data_4   | EEPROM               | I/O      |      |
* | A5     | PC5    | Data_5   | EEPROM               | I/O      |      |
* +--------+--------+----------+----------------------+----------+------+
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "pin_definitions.hpp"

#define LED 13
//#define LATCH 9
uint8_t buffer[256];
uint8_t prgBuffer[256];
bool inputAvailable = false;
String inputString;

int programFile(const char *path, uint16_t adr);
void hexDump(const char *desc, void *addr, unsigned int offset, int len);

DECLARE_PIN (CE_pin, D, 2)
DECLARE_PIN (A9_VPE_pin, D, 3)
DECLARE_PIN (OE_pin, D, 4)
DECLARE_PIN (OE_VPP_pin, D, 5)
DECLARE_PIN (VPP_VPE_pin, B, 0)
DECLARE_PIN (LATCH_pin, B, 1)
DECLARE_PIN (SS_pin, B, 2)
DECLARE_PIN_GROUP (DATA_low, C, 0, 6)
DECLARE_PIN_GROUP (DATA_high, D, 6, 2)


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
    //write(OE_pin, 1);
}
void disable_OE_VPP()
{
    write(OE_VPP_pin, 0);
    //write(OE_pin, 0);
}

void eeprom_init_pins()
{
    set(CE_pin | OE_pin /*| SS_pin*/ | LATCH_pin );     // sicherstellen, dass Ausgaenge HIGH sind
    reset(A9_VPE_pin | OE_VPP_pin | VPP_VPE_pin);   // sicherstellen, dass Ausgaenge LOW sind
    init_as_output(CE_pin | OE_pin | A9_VPE_pin | OE_VPP_pin /*| SS_pin*/ | LATCH_pin | VPP_VPE_pin);
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
    eeprom_chip_select();
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

unsigned int eeprom_read_id(bool *success_ptr)
{
        unsigned int id = 0;
        unsigned int test_id = 0;

        // For comparison, read the address with A9 high
        uint16_t address = 1 << 9;
        uint16_t test_address = address; 
        uint16_t max_address = address + 1;
        
        if (test_address < 0xFFFF)
        {
            eeprom_set_address(test_address);
            test_id = eeprom_read_byte();
            eeprom_set_address(++test_address);
            test_id <<= 8;
            test_id |= eeprom_read_byte();
        }
        // Enable HV on A9 for identify mode
        eeprom_chip_deselect();
        begin_read();
        enable_A9_HV();
        _delay_us(10);
        do
        {
            eeprom_set_address(address);
            _delay_us(5);
            eeprom_chip_select();
            _delay_us(5);
            ++address;
            id <<= 8;
            id |= eeprom_data_in();
            eeprom_chip_deselect();
        } while (address <= max_address);
        disable_A9_HV();
        end_read();

        if (success_ptr)
        {
            *success_ptr = (test_id != id);
        }

        return id;
}

void eeprom_read_bytes_at(const uint16_t address, uint8_t *buf, const int len)
{
    int offset = 0;
    
    eeprom_set_data_in();
    
    while ( offset < len )
    {
        set(OE_pin | CE_pin);
        eeprom_set_address(address + offset);
        write(CE_pin, 0);
        write(OE_pin, 0);
        delayMicroseconds(3);   // Toe
        buf[offset] = eeprom_data_in();
        offset++;
    }
    set(OE_pin | CE_pin); 
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

void read_id() {
    uint8_t id_byte1 = 0;
    uint8_t id_byte2 = 0;

    eeprom_set_data_in();
    set(OE_pin | CE_pin);
    eeprom_set_address(0);
    enable_A9_HV();
    delayMicroseconds(5);
    write(CE_pin, 0);
    
    delayMicroseconds(5);
    write(OE_pin, 0);
    delayMicroseconds(5); // Toe
    id_byte1 = eeprom_data_in();
    write(OE_pin, 1);

    eeprom_set_address(1);
    delayMicroseconds(5);
    write(OE_pin, 0);
    delayMicroseconds(5); // Toe
    id_byte2 = eeprom_data_in();
    write(OE_pin, 1);
    disable_A9_HV();
    write(CE_pin, 1);
    Serial.print("ID = ");
    Serial.print(id_byte1, HEX);
    Serial.print(" / ");
    Serial.println(id_byte2, HEX);
}

void erase()
{
    eeprom_set_data_in();
    set(OE_pin | CE_pin);
    eeprom_set_address(0);
    enable_A9_HV();
    enable_OE_VPP();
    delayMicroseconds(5);   //Toes OE/VPP setup time, min 2us
    write(CE_pin, 0);
    delay(100);             // Tpwe erase puls width (95...105 ms)
    write(CE_pin, 1);
    delayMicroseconds(5);   // Toeh
    disable_OE_VPP();       // OE bleibt H
    disable_A9_HV();
}

bool blank_check(uint16_t max_address, uint16_t *adr_fail)
{
    uint16_t address = 0;
    uint8_t b = 0;
    
    eeprom_set_data_in();

    do {
        set(OE_pin | CE_pin);
        eeprom_set_address(address);
        write(CE_pin, 0);
        write(OE_pin, 0);
        delayMicroseconds(3); // Toe
        if ( (b = eeprom_data_in()) != 0xFF ) 
            break;
    } while ( address++ < max_address );
    set(OE_pin | CE_pin);
    if ( b != 0xFF ) {
        if ( adr_fail )
            *adr_fail = address;
        return false;
    }
    return true;
}

bool program( uint16_t address, uint8_t *buf, int len)
{
    int offset = 0;
    uint8_t b = 0xFF;
    int loops = 0;
    
    set(OE_pin | CE_pin);
    eeprom_set_data_out(); 
    enable_OE_VPP();    
    do
    {
        eeprom_set_address(address + offset);
        delayMicroseconds(3);       // Tds
        eeprom_data_out(buf[offset]);
        delayMicroseconds(5);       // Tas
        write(CE_pin, 0);
        delayMicroseconds(95);     // Tpwp (funktioniert auch mit 5 us!)
        write(CE_pin, 1);
        delayMicroseconds(3);       // Tdh / Tah / Toeh
             //verify
            write(OE_pin, 0);
            eeprom_set_data_in();
            disable_OE_VPP();
            write(CE_pin, 0);
            delayMicroseconds(30);      // Tdv1
            b = eeprom_data_in();
            set(OE_pin | CE_pin);
            enable_OE_VPP();
            delayMicroseconds(5);
            eeprom_set_data_out();
            //Serial.print(b != buf[offset] ? "-" : "+"); 
        if ( b != buf[offset] ) 
            loops++;
        else {
            offset++;
            loops = 0;
        }   
    } while (offset < len && loops < 20 );
    set(OE_pin | CE_pin);
    disable_OE_VPP();
    eeprom_set_data_in();
    return (loops == 0); 
}


void setup()
{
    
    inputString.reserve(20);
    memset(buffer, 0x55, sizeof(buffer));
    memset(prgBuffer, 0xFF, sizeof(prgBuffer));
    memcpy(prgBuffer, "Das ist ein Test", 16);

    // initialize serial
    Serial.begin(115200);

    eeprom_init_pins();
    eeprom_set_data_in();
    Serial.print("Stack size1: ");
    Serial.println(RAMEND - SP);
    Serial.println("EEPrommer V0");   
    SPI.begin();

    if ( !SD.begin(SS) )
        Serial.println("SD Initialization failed");


    if (SD.exists(const_cast<char *>("dump.txt")) )
         Serial.println(PSTR("ok"));
    else
        Serial.println(PSTR("fail"));
    Serial.print("Stack size2: ");
    Serial.println(RAMEND - SP);
    File f = SD.open("dump.txt");
    Serial.print("Stack size3: ");
    Serial.println(RAMEND - SP);
    f.seek(0);
    int len = f.readBytes(buffer, 20);
    buffer[len] = 0;
    hexDump("Hexdump", buffer, 0, len);
    f.close();
}

// the loop function runs over and over again forever
void loop()
{
    static uint16_t adr = 0;
    static uint16_t nextAdr = 0;
    //File f;

    if ( inputAvailable ) {
        Serial.println(inputString);
        inputString.trim();
        switch (inputString[0]) {
            case 'a':
            case 'A': 
                if (inputString[1] == 'x' || inputString[1] == 'X' )
                    nextAdr = adr = strtoul(inputString.substring(2).c_str(), 0, 16);
                else
                    nextAdr = adr = inputString.substring(1).toInt();
                Serial.print("Adresse (hex) = "); Serial.println(adr, HEX);
                //Serial.println(adr, DEC);
                break;
            case 'h':
            case 'H':
                Serial.println(adr, HEX);
                hexDump("dump", buffer, adr, sizeof(buffer));
                break;
            case 'e':
            case 'E':
                Serial.print("Erasing...");
                erase();
                if ( !blank_check(0xFFFF, 0) )
                    Serial.println (" failed");
                else 
                    Serial.println(" ok");
            break;
            case 'r':
                eeprom_read_bytes_at(adr, buffer, sizeof(buffer));
                hexDump("read", buffer, adr, sizeof(buffer));
                nextAdr = adr + sizeof(buffer);
                break;
            case 'n':
                eeprom_read_bytes_at(nextAdr, buffer, sizeof(buffer));
                hexDump("next read", buffer, nextAdr, sizeof(buffer));
                nextAdr += sizeof(buffer);
                break;
            case 'p':
                hexDump("prgBuffer", prgBuffer, 0, sizeof(prgBuffer));
                Serial.print("Programming at "); Serial.println(adr, HEX);
                if ( !program(adr, prgBuffer, sizeof(prgBuffer)) )
                    Serial.println("programming fails");
                break;
            case 'b':
                uint16_t fail;
                Serial.print("Blank check ");
                if ( blank_check(0xFFFF, &fail) )
                    Serial.println("ok!");
                else {
                    Serial.print("failed on address ");
                    Serial.println(fail, HEX);
                } 
                break;
            case 'f':
                int rc;
                
                rc = programFile(inputString.substring(1).c_str(), adr);
                //rc = programFile("test.txt", adr);
                if ( rc < 0 ) {
                    Serial.print("return code = "); 
                    Serial.println(rc);
                }
                break;
            default:
                Serial.println("?");
        } 
        inputString = "";
        inputAvailable = false;
    }
}

void serialEvent()
{
    int len;
    while (Serial.available() && !inputAvailable )
    {
        char inChar = (char)Serial.read();
        // Serial.println(int(inChar));
        switch (inChar) {
            case '\n':
                inputAvailable = true;
                break;
            case '\b':
                if ( (len = inputString.length()) > 0 )
                    inputString.remove(len-1);
                    // fall thru 
            case '\r':  
                break;
            default:
                inputString += inChar;
        } 
    }
}

int programFile(const char *path, uint16_t adr)
{
    int file_size;
    int bytes_written = 0;
    Serial.println(path);
    if (!SD.exists(const_cast<char *>(path)))
        return -1;
    
    File f = SD.open("dump.txt");
    Serial.println(f.size());
    Serial.print("Stack size: ");
    Serial.println(RAMEND - SP);
    if ( !f )
        return -2;
    file_size = f.size();
    if ((unsigned int)file_size > (0xFFFF - adr)) {
        f.close();
        return -3;
    }
    Serial.print("File: ");
    Serial.print(path);
    Serial.print(" / Size: ");
    Serial.println(file_size);
    //f.seek(0);
    Serial.print("Programming ... ");
    while (f.available())
    {
        int bytes_readed = f.readBytes(buffer, sizeof(buffer));
        if (bytes_readed)
        {
            if (program(adr, buffer, bytes_readed))
            {
                Serial.print("#");
                adr += bytes_readed;
                bytes_written += bytes_readed;
            }
            else
            {
                f.close();
                return -4;
            }
        }
    }
    Serial.println();
    Serial.print(bytes_written);
    Serial.println(" bytes written");
    f.close();
    return bytes_written;
}

void hexDump(const char *desc, void *addr, unsigned int offset, int len)
{
    int i;
    unsigned char ascii_buffer[17]; // ASCII Block
    char sprintfbuffer[14];
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
                Serial.print(" | ");
                Serial.println((char *)ascii_buffer);
            }
            // Output the offset.
            //printf("  %08x ", offset);
            sprintf(sprintfbuffer, "  0x%08X ", offset);
            Serial.print(sprintfbuffer);
            offset += 16;
        }
 
        // Now the hex code for the specific character.
        //printf(" %02x", pc[i]);
        sprintf(sprintfbuffer, " %02x", pc[i]);
        Serial.print(sprintfbuffer);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            ascii_buffer[i % 16] = '.';
        else
            ascii_buffer[i % 16] = pc[i];
        ascii_buffer[(i % 16) + 1] = '\0';
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
    Serial.println((char *)ascii_buffer);
}
