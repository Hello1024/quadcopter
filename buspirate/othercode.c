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

//Spi Comm.pins with XN297/PPM, direct port access, do not change
#define PPM_pin   2             // PPM in
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

//
#define NOP() __asm__ __volatile__("nop")

#define PACKET_LENGTH 19
#define PACKET_INTERVAL 6 // interval of time between start of 2 packets, in ms

#define PPM_MIN 1000
#define PPM_MAX 2000

// PPM stream settings
#define CHANNELS 6
enum chan_order{  // TAER -> Spektrum/FrSky chan order
    THROTTLE,
    AILERON,
    ELEVATOR,
    RUDDER,
    AUX1,  // mode
    AUX2,  // flip control
};

//########## Variables #################
static uint8_t aid[4]={0xFF,0xFF,0xFF,0xFF}; // aircraft ID
static uint8_t txid[4]; // transmitter ID
static uint8_t freq[4]; // frequency hopping table
static uint8_t packet[PACKET_LENGTH];
volatile uint16_t Servo_data[CHANNELS] = {0,};
int ledPin = 13;

void setup() {
    for(uint8_t i=0;i<4;i++) {
        txid[i] = 0x4;  // chosen by fair dice roll
    }
    txid[1] %= 0x30;
    freq[0] = (txid[0] & 0x0F) + 0x03;
    freq[1] = (txid[0] >> 4) + 0x16;
    freq[2] = (txid[1] & 0x0F) + 0x2D;
    freq[3] = (txid[1] >> 4) + 0x40;
    
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
    if(_spi_read_address(0x10) != 0xCC) // reads 1 byte of Transmit address
        ; // we have a problem ....
    _spi_write_address(0x20, 0x0e); // Power on, TX mode, 2 byte CRC
    MOSI_off;
    delay(50);//50ms delay

    delay(50);
    //Bind to Receiver
    bind_XN297();
}

//############ MAIN LOOP ##############
void loop() {
    for (int chan = 0; chan < 4; chan++) {
        
        CE_off;
        delayMicroseconds(5);
        _spi_write_address(0x20, 0x0e); // TX mode
        _spi_write_address(0x25, freq[chan]); // Set RF chan
        _spi_write_address(0x27, 0x70); // Clear interrupts
        _spi_write_address(0xe1, 0x00); // Flush TX
        Write_Packet(0x55); // servo_data timing is updated in interrupt (ISR routine for decoding PPM signal)
         // wait
    }
}

//BIND_TX
void bind_XN297() {
    byte counter=255;
    bool bound=false;
    while(!bound){
        CE_off;
        delayMicroseconds(5);
        _spi_write_address(0x20, 0x0e); // Power on, TX mode, 2 byte CRC
        _spi_write_address(0x25, 0x02); // set RF channel 2
        _spi_write_address(0x27, 0x70); // Clear interrupts
        _spi_write_address(0xe1, 0x00); // Flush TX
        Write_Packet(0xaa); // send bind packet
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
                aid[0] = packet[5];
                aid[1] = packet[6];
                aid[2] = packet[7];
                aid[3] = packet[8];
                if(packet[9]==1) {
                    bound=true;
                    break;
                }
            }
        }
        CE_off;
        
    }

    digitalWrite(ledPin, HIGH);//LED on at end of bind
}

//-------------------------------
//-------------------------------
//XN297 SPI routines
//-------------------------------
//-------------------------------
void Write_Packet(uint8_t init){//24 bytes total per packet
    uint8_t i=0;
    unsigned char buf[20]
    buf[i++] = 
    buf[i++] = (0xa0); // Write TX payload
    buf[i++] = (init); // packet type: 0xaa or 0x55 aka bind packet or data packet)
    buf[i++] = (txid[0]); 
    buf[i++] = (txid[1]); 
    buf[i++] = (txid[2]);
    buf[i++] = (txid[3]);
    buf[i++] = (aid[0]); // Aircraft ID
    buf[i++] = (aid[1]);
    buf[i++] = (aid[2]);
    buf[i++] = (aid[3]);
    // channels data
    if (Servo_data[5] > 1500)
        bitSet(Servo_data[3], 12);// Set flip mode based on chan6 input
    packet[0]=lowByte(Servo_data[AILERON]);//low byte of servo timing(1000-2000us)
    packet[1]=highByte(Servo_data[AILERON]);//high byte of servo timing(1000-2000us)
    packet[2]=lowByte(Servo_data[ELEVATOR]);
    packet[3]=highByte(Servo_data[ELEVATOR]);
    packet[4]=lowByte(Servo_data[THROTTLE]);
    packet[5]=highByte(Servo_data[THROTTLE]);
    packet[6]=lowByte(Servo_data[RUDDER]);
    packet[7]=highByte(Servo_data[RUDDER]);
    for(i=0;i<4;i++){
        buf[i++] = (packet[0+2*i]);
        buf[i++] = (packet[1+2*i]);
    }
    // Set mode based on chan5 input
    if (Servo_data[4] > 1800)
         buf[i++] = 0x02;// mode 3
    else if (Servo_data[4] > 1300)
         buf[i++] = 0x01;// mode 2
    else
         buf[i++] = 0x00;// mode 1
    
    buf[i++] = (0x00);

    spi_txn(buf, 20);

    CE_on; // transmit
}

void Read_Packet() {
    unsigned char buf[PACKET_LENGTH+1]
    buf[0] = 0x61;
    for (i=0;i<PACKET_LENGTH;i++) buf[i+1]=0x00;
    
    spi_txn(&buf, PACKET_LENGTH+1);
    for (i=0;i<PACKET_LENGTH;i++) packet[i] = buf[i+1];
}

void for (i=0;i<PACKET_LENGTH;i++) buf[i+1]=0x00;
    (uint8_t command) {
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
}

void _spi_write_address(uint8_t address, uint8_t data) {
    unsigned char buf[2];
    buf[0] = address;
    buf[1] = data;
    spi_txn(&buf, 2);
}


uint8_t _spi_read_address(uint8_t address) {
    uint8_t result;
    unsigned char buf[2];
    buf[0] = address;
    buf[1] = 0x00;
    spi_txn(&buf, 2);

    return(buf[1]);
}
