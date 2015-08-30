// Update : a xn297 is not required anymore, it can be emulated with a nRF24l01 :
// https://gist.github.com/goebish/ab4bc5f2dfb1ac404d3e

// **************************************************************
// ****************** CX-10 Tx Code (blue PCB) ******************
// by goebish on RCgroups.com
// based on green pcb cx-10 TX code by closedsink on RCgroups.com
// based largely on flysky code by midelic on RCgroups.com
// Thanks to PhracturedBlue, hexfet, ThierryRC
// Hasi for his arduino PPM decoder
// **************************************************************
// Hardware any M8/M168/M328 setup(arduino promini,duemilanove...as)
// !!! take care when flashing the AVR, the XN297 RF chip supports 3.6V max !!!
// !!! use a 3.3V programmer or disconnect the RF module when flashing.     !!!
// !!! XN297 inputs are not 5 Volt tolerant, use a 3.3V MCU or logic level  !!!
// !!! adapter or voltage divider on MOSI/SCK/CS/CE if using 5V MCU         !!!

// DIY CX-10 RF module - XN297 and circuitry harvested from *BLUE* CX-10 controller
// pinout: http://imgur.com/a/unff4
// XN297 datasheet: http://www.foxware-cn.com/UploadFile/20140808155134.pdf

#include "CX10.h"

#ifdef RFDUINO

#include "SPI.h"

#define A0 0
#define A1 1
#define CS_pin    6
#define CE_pin    2
#define MOSI_pin  PIN_SPI_MOSI    // 5
#define SCK_pin   PIN_SPI_SCK     // 4
#define MISO_pin  PIN_SPI_MISO    // 3

#define pinOn(pin)     NRF_GPIO->OUTSET = 1 << (pin)
#define pinOff(pin)    NRF_GPIO->OUTCLR = 1 << (pin)

#define CS_on     pinOn(CS_pin)
#define CS_off    pinOff(CS_pin)
#define CE_on     pinOn(CE_pin)
#define CE_off    pinOff(CE_pin)
#define MOSI_on   pinOn(MOSI_pin)
#define MOSI_off  pinOff(MOSI_pin)
#define SCK_on    pinOn(SCK_pin)
#define SCK_off   pinOff(SCK_pin)

#define cli()
#define sei()

#else

//Spi Comm.pins with XN297/PPM, direct port access, do not change
#define MOSI_pin  5             // MOSI-D5
#define SCK_pin   4             // SCK-D4
#define CS_pin    6             // CS-D6
#define CE_pin    3             // CE-D3
#define MISO_pin  7             // MISO-D7
//---------------------------------
// spi outputs
#define  CS_on PORTD |= 0x40    // PORTD6
#define  CS_off PORTD &= 0xBF   // PORTD6
#define  CE_on PORTD |= 0x08    // PORTD3
#define  CE_off PORTD &= 0xF7   // PORTD3
//
#define  SCK_on PORTD |= 0x10   // PORTD4
#define  SCK_off PORTD &= 0xEF  // PORTD4
#define  MOSI_on PORTD |= 0x20  // PORTD5
#define  MOSI_off PORTD &= 0xDF // PORTD5
// spi input
#define  MISO_on (PIND & 0x80)  // PORTD7
#endif

//
#define NOP() __asm__ __volatile__("nop")

#define PACKET_LENGTH 19
#define PACKET_INTERVAL 6 // interval of time between start of 2 packets, in ms
#define BIND_INTERVAL   1001


// PPM stream settings
enum chan_order{  // TAER -> Spektrum/FrSky chan order
    THROTTLE,
    AILERON,
    ELEVATOR,
    RUDDER,
    AUX1,  // mode
    AUX2,  // flip control
};

//########## Variables #################
static uint8_t packet[PACKET_LENGTH];
int ledPin = 13;

CX10::CX10() {
    randomSeed((analogRead(A0) & 0x1F) | (analogRead(A1) << 5));
    for (int n = 0; n < CRAFT; ++n) {
        Craft *c = &craft_[n];
        c->nextPacket = 0;
        memset(c->aid, 0xff, sizeof c->aid);
        memset(c->Servo_data, 0, sizeof c->Servo_data);
        setThrottle(n, 0);
        setAileron(n, 500);
        setElevator(n, 500);
        setRudder(n, 500);
        for(uint8_t i=0;i<4;i++) {
#ifdef RFDUINO
            c->txid[i] = random(256);
#else
            c->txid[i] = random();
#endif
        }
        c->txid[1] %= 0x30;
        c->freq[0] = (c->txid[0] & 0x0F) + 0x03;
        c->freq[1] = (c->txid[0] >> 4) + 0x16;
        c->freq[2] = (c->txid[1] & 0x0F) + 0x2D;
        c->freq[3] = (c->txid[1] >> 4) + 0x40;
    }
    pinMode(ledPin, OUTPUT);
    //RF module pins
    pinMode(MOSI_pin, OUTPUT);
    pinMode(SCK_pin, OUTPUT);
    pinMode(MISO_pin, INPUT);
    pinMode(CS_pin, OUTPUT);
    pinMode(CE_pin, OUTPUT);
    digitalWrite(ledPin, LOW);//start LED off
    CS_on;//start CS high
    CE_on;//start CE high
    MOSI_on;//start MOSI high
    SCK_on;//start sck high
    delay(70);//wait 70ms
    CS_off;//start CS low
    CE_off;//start CE low
    MOSI_off;//start MOSI low
    SCK_off;//start sck low
    delay(100);
    CS_on;//start CS high
    delay(10);
#ifdef RFDUINO
    SPI.begin();
    SPI.setFrequency(250);
#endif
    //############ INIT1 ##############
    CS_off;
    _spi_write(0x3f); // Set Baseband parameters (debug registers) - BB_CAL
    _spi_write(0x4c);
    _spi_write(0x84);
    _spi_write(0x67);
    _spi_write(0x9c);
    _spi_write(0x20);
    CS_on;
    delayMicroseconds(5);
    CS_off;
    _spi_write(0x3e); // Set RF parameters (debug registers) - RF_CAL
    _spi_write(0xc9);
    _spi_write(0x9a);
    _spi_write(0xb0);
    _spi_write(0x61);
    _spi_write(0xbb);
    _spi_write(0xab);
    _spi_write(0x9c);
    CS_on;
    delayMicroseconds(5);
    CS_off;
    _spi_write(0x39); // Set Demodulator parameters (debug registers) - DEMOD_CAL
    _spi_write(0x0b);
    _spi_write(0xdf);
    _spi_write(0xc4);
    _spi_write(0xa7);
    _spi_write(0x03);
    CS_on;
    delayMicroseconds(5);
    CS_off;
    _spi_write(0x30); // Set TX address 0xCCCCCCCC
    _spi_write(0xcc);
    _spi_write(0xcc);
    _spi_write(0xcc);
    _spi_write(0xcc);
    _spi_write(0xcc);
    CS_on;
    delayMicroseconds(5);
    CS_off;
    _spi_write(0x2a); // Set RX pipe 0 address 0xCCCCCCCC
    _spi_write(0xcc);
    _spi_write(0xcc);
    _spi_write(0xcc);
    _spi_write(0xcc);
    _spi_write(0xcc);
    CS_on;
    delayMicroseconds(5);
    _spi_write_address(0xe1, 0x00); // Clear TX buffer
    _spi_write_address(0xe2, 0x00); // Clear RX buffer
    _spi_write_address(0x27, 0x70); // Clear interrupts
    _spi_write_address(0x21, 0x00); // No auto-acknowledge
    _spi_write_address(0x22, 0x01); // Enable only data pipe 0
    _spi_write_address(0x23, 0x03); // Set 5 byte rx/tx address field width
    _spi_write_address(0x25, 0x02); // Set channel frequency
    _spi_write_address(0x24, 0x00); // No auto-retransmit
    _spi_write_address(0x31, PACKET_LENGTH); // 19-byte payload
    _spi_write_address(0x26, 0x07); // 1 Mbps air data rate, 5dbm RF power
    _spi_write_address(0x50, 0x73); // Activate extra feature register
    _spi_write_address(0x3c, 0x00); // Disable dynamic payload length
    _spi_write_address(0x3d, 0x00); // Extra features all off
    MOSI_off;
    delay(100);//100ms delay

    //############ INIT2 ##############
    healthy_ = _spi_read_address(0x10) == 0xCC;
    
    _spi_write_address(0x20, 0x0e); // Power on, TX mode, 2 byte CRC
    MOSI_off;
    delay(100);
    nextBind_ = millis();
    nextSlot_ = 0;
}

static void print4(const uint8_t d[4]) {
    Serial.print(d[0], HEX);
    Serial.print(':');
    Serial.print(d[1], HEX);
    Serial.print(':');
    Serial.print(d[2], HEX);
    Serial.print(':');
    Serial.print(d[3], HEX);

}

void CX10::printAID(int slot) const {
    print4(craft_[slot].aid);
}

void CX10::printTXID(int slot) const {
    print4(craft_[slot].txid);
}

//############ MAIN LOOP ##############
void CX10::loop() {
    for (int slot = 0; slot < CRAFT; ++slot) {
        Craft *c = &craft_[slot];
        if(c->nextPacket != 0 && micros() >= c->nextPacket) {
            c->nextPacket += PACKET_INTERVAL * 1000;
            CE_off;
            delayMicroseconds(5);
            _spi_write_address(0x20, 0x0e); // TX mode
            _spi_write_address(0x25, c->freq[c->chan]); // Set RF chan
            _spi_write_address(0x27, 0x70); // Clear interrupts
            _spi_write_address(0xe1, 0x00); // Flush TX
            Write_Packet(slot, 0x55); // servo_data timing is updated in interrupt (ISR routine for decoding PPM signal)
            if (++c->chan == 4)
                c->chan = 0;
        }
    }
    if (nextSlot_ < CRAFT && millis() >= nextBind_) {
        if (bind_XN297(nextSlot_))
            ++nextSlot_;
        nextBind_ = millis() + BIND_INTERVAL;
    }
}

void CX10::setAileron(int slot, int value){ craft_[slot].Servo_data[AILERON] = value + 1000; }
void CX10::setElevator(int slot, int value){ craft_[slot].Servo_data[ELEVATOR] = value + 1000; }
void CX10::setThrottle(int slot, int value){ craft_[slot].Servo_data[THROTTLE] = value + 1000; }
void CX10::setRudder(int slot, int value){ craft_[slot].Servo_data[RUDDER] = value + 1000; }
  
//BIND_TX
bool CX10::bind_XN297(int slot) {
    byte counter=255;
    bool bound=false;
    Craft *c = &craft_[slot];

    while(!bound && counter > 240){
        CE_off;
        delayMicroseconds(5);
        _spi_write_address(0x20, 0x0e); // Power on, TX mode, 2 byte CRC
        _spi_write_address(0x25, 0x02); // set RF channel 2
        _spi_write_address(0x27, 0x70); // Clear interrupts
        _spi_write_address(0xe1, 0x00); // Flush TX
        Write_Packet(slot, 0xaa); // send bind packet
        delay(2);
        _spi_write_address(0x27, 0x70); // Clear interrupts
        _spi_write_address(0x25, 0x02); // Set RF channel
        _spi_write_address(0x20, 0x0F); // Power on, RX mode, 2 byte CRC
        CE_on; // RX mode
        uint32_t timeout = millis()+4;
        while(millis()<timeout) {
            if(_spi_read_address(0x07) == 0x40) { // data received
                CE_off;
                Read_Packet();
                c->aid[0] = packet[5];
                c->aid[1] = packet[6];
                c->aid[2] = packet[7];
                c->aid[3] = packet[8];
                if(packet[9]==1) {
                    bound=true;
                    break;
                }
            }
        }
        CE_off;
        digitalWrite(ledPin, bitRead(--counter,3)); //check for 0bxxxx1xxx to flash LED
    }
    if (bound) {
        digitalWrite(ledPin, HIGH);//LED on at end of bind
        c->nextPacket = micros() + PACKET_INTERVAL * 1000 - 200;
        c->chan = 0;
        Serial.println("found a craft!");
        printTXID(slot);
        Serial.print('+');
        printAID(slot);
        Serial.println("");
    }
    return bound;
}

//-------------------------------
//-------------------------------
//XN297 SPI routines
//-------------------------------
//-------------------------------
void CX10::Write_Packet(int slot, uint8_t init) { // 24 bytes total per packet (FIXME: wrong?)
    Craft *c = &craft_[slot];
    uint8_t i;

    CS_off;
    _spi_write(0xa0); // Write TX payload
    _spi_write(init); // packet type: 0xaa or 0x55 aka bind packet or data packet)
    _spi_write(c->txid[0]); 
    _spi_write(c->txid[1]); 
    _spi_write(c->txid[2]);
    _spi_write(c->txid[3]);
    _spi_write(c->aid[0]); // Aircraft ID
    _spi_write(c->aid[1]);
    _spi_write(c->aid[2]);
    _spi_write(c->aid[3]);
    // channels data
    if (c->Servo_data[5] > 1500)
        bitSet(c->Servo_data[3], 12);// Set flip mode based on chan6 input
    cli(); // disable interrupts
    packet[0]=lowByte(c->Servo_data[AILERON]);//low byte of servo timing(1000-2000us)
    packet[1]=highByte(c->Servo_data[AILERON]);//high byte of servo timing(1000-2000us)
    packet[2]=lowByte(c->Servo_data[ELEVATOR]);
    packet[3]=highByte(c->Servo_data[ELEVATOR]);
    packet[4]=lowByte(c->Servo_data[THROTTLE]);
    packet[5]=highByte(c->Servo_data[THROTTLE]);
    packet[6]=lowByte(c->Servo_data[RUDDER]);
    packet[7]=highByte(c->Servo_data[RUDDER]);
    sei(); // enable interrupts
    for(i=0;i<4;i++){
        _spi_write(packet[0+2*i]);
        _spi_write(packet[1+2*i]);
    }
    // Set mode based on chan5 input
    if (c->Servo_data[4] > 1800)
        i = 0x02;// mode 3
    else if (c->Servo_data[4] > 1300)
        i = 0x01;// mode 2
    else
        i= 0x00;// mode 1
    
    _spi_write(i);
    _spi_write(0x00);
    MOSI_off;
    CS_on;
    CE_on; // transmit
}

void CX10::Read_Packet() {
    uint8_t i;
    CS_off;
    _spi_write(0x61); // Read RX payload
    for (i=0;i<PACKET_LENGTH;i++) {
        packet[i]=_spi_read();
    }
    CS_on;
}

void CX10::_spi_write(uint8_t command) {
#ifdef RFDUINO
    SPI.transfer(command);
#else
    uint8_t n=8;
    SCK_off;
    MOSI_off;
    while(n--) {
        if(command&0x80)
            MOSI_on;
        else
            MOSI_off;
        SCK_on;
        NOP();
        SCK_off;
        command = command << 1;
    }
    MOSI_on;
#endif
}

void CX10::_spi_write_address(uint8_t address, uint8_t data) {
    CS_off;
    _spi_write(address);
    NOP();
    _spi_write(data);
    CS_on;
}

// read one byte from MISO
uint8_t CX10::_spi_read() {
#ifdef RFDUINO
    return SPI.transfer(0);
#else
    uint8_t result=0;
    uint8_t i;
    MOSI_off;
    NOP();
    for(i=0;i<8;i++) {
        if(MISO_on) // if MISO is HIGH
            result = (result<<1)|0x01;
        else
            result = result<<1;
        SCK_on;
        NOP();
        SCK_off;
        NOP();
    }
    return result;
#endif
}

uint8_t CX10::_spi_read_address(uint8_t address) {
    uint8_t result;
    CS_off;
    _spi_write(address);
    result = _spi_read();
    CS_on;
    return(result);
}
