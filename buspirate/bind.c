#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter

#include "iface_nrf24l01.h"
#include "buspirate.h"

#define BIND_COUNT 4360   // 6 seconds
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf printf

#define CX10_PACKET_SIZE 15
#define CX10A_PACKET_SIZE 19       // CX10 blue board packets have 19-byte payload
#define CX10_PACKET_PERIOD   1316  // Timeout for callback in uSec
#define CX10A_PACKET_PERIOD  6000

#define INITIAL_WAIT     500

// flags 
#define FLAG_FLIP       0x1000 // goes to rudder channel
#define FLAG_MODE_MASK  0x0003
#define FLAG_HEADLESS   0x0004
// flags2
#define FLAG_VIDEO      0x0002
#define FLAG_SNAPSHOT   0x0004


enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};

enum {
    FORMAT_CX10_GREEN = 0,
    FORMAT_CX10_BLUE,
    FORMAT_DM007,
};

// For code readability
enum {
    CHANNEL1 = 0,   // Aileron
    CHANNEL2,       // Elevator
    CHANNEL3,       // Throttle
    CHANNEL4,       // Rudder
    CHANNEL5,       // Rate/Mode (+ Headless on CX-10A)
    CHANNEL6,       // Flip
    CHANNEL7,       // Still Camera (DM007)
    CHANNEL8,       // Video Camera (DM007)
    CHANNEL9,       // Headless (DM007)
    CHANNEL10
};

static u8 packet[CX10A_PACKET_SIZE]; // CX10A (blue board) has larger packet size
static u8 packet_size;
static u16 packet_period;
static u8 phase;
static u8 bind_phase;
static u16 bind_counter;
static u16 throttle, rudder, elevator, aileron, flags, flags2;
static const u8 rx_tx_addr[] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc};

// frequency channel management
#define RF_BIND_CHANNEL 0x02
#define NUM_RF_CHANNELS    4
static u8 current_chan = 0;
static u8 txid[4];
static u8 rf_chans[4];

enum {
    CX10_INIT1 = 0,
    CX10_BIND1,
    CX10_BIND2,
    CX10_DATA
};

// Bit vector from bit position
#define BV(bit) (1 << bit)


static void read_controls(u16* throttle, u16* rudder, u16* elevator, u16* aileron, u16* flags, u16* flags2)
{

    // All values from 2000 - 3000.  1500 = middle.

    *aileron  = 1500;
    *elevator = 1500;
    *throttle = 1000;
    *rudder   = 1500;

    *flags &= ~FLAG_MODE_MASK;
    // Channel 5 - mode
    if (0) {
        if (0)
            *flags |= 1;
        else
            *flags |= 2; // headless on CX-10A
    }

    // Channel 6 - flip flag
    if (1)
        *flags &= ~FLAG_FLIP;
    else
        *flags |= FLAG_FLIP;


    dbgprintf("ail %5d, ele %5d, thr %5d, rud %5d, flags 0x%4x, flags2 0x%2x\n",
            *aileron, *elevator, *throttle, *rudder, *flags, *flags2);
}

static void send_packet(u8 bind)
{
    u8 offset = 4;
    packet[0] = bind ? 0xAA : 0x55;
    packet[1] = txid[0];
    packet[2] = txid[1];
    packet[3] = txid[2];
    packet[4] = txid[3];
    // for CX-10A [5]-[8] is aircraft id received during bind 
    read_controls(&throttle, &rudder, &elevator, &aileron, &flags, &flags2);
//packet[5] = 0xff;
//packet[6] = 0xff;
//packet[7] = 0xff;
//packet[8] = 0xff;

    packet[5+offset] = aileron & 0xff;
    packet[6+offset] = (aileron >> 8) & 0xff;
    packet[7+offset] = elevator & 0xff;
    packet[8+offset] = (elevator >> 8) & 0xff;
    packet[9+offset] = throttle & 0xff;
    packet[10+offset] = (throttle >> 8) & 0xff;
    packet[11+offset] = rudder & 0xff;
    packet[12+offset] = ((rudder >> 8) & 0xff) | ((flags & FLAG_FLIP) >> 8);  // 0x10 here is a flip flag 
    packet[13+offset] = flags & 0xff;
    packet[14+offset] = flags2 & 0xff;

    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (bind) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    } else {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chans[current_chan++]);
        current_chan %= NUM_RF_CHANNELS;
    }
    // clear packet status bits and TX FIFO
    // NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    // NRF24L01_FlushTx();

   // CE_lo();  // prevent early sends.
    XN297_WritePayload(packet, packet_size);

   // CE_hi();
 //   CE_hi();
    // TODO 15us delay
 //   CE_lo();



}

static void cx10_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
     NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67\x9c\x20", 5); 
     NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0\x61\xbb\xab\x9c", 7); 
     NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4\xa7\x03", 5); 

    XN297_SetTXAddr(rx_tx_addr, 5);
    XN297_SetRXAddr(rx_tx_addr, 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR,0);     // No auto retransmits
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, packet_size); // bytes of data payload for rx pipe 1 
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x07);
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(TXPOWER_150mW);
    
    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);     // Set feature bits on


}

static u16 cx10_callback()
{
    static int try = 0;
    try++;
    switch (phase) {
    case CX10_INIT1:
        phase = bind_phase;
        break;

    case CX10_BIND1:
        printf("bind1\n");
        if (bind_counter == 0) {
            phase = CX10_DATA;
        } else {
            send_packet(1);
            bind_counter -= 1;
        }
        break;
        
    case CX10_BIND2:
        if( (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0xF)==0) { // RX fifo data ready  //& BV(NRF24L01_07_RX_DR)
            printf("reply!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            XN297_ReadPayload(packet, packet_size);
            // NRF24L01_SetTxRxMode(TXRX_OFF);
            if(packet[9] == 1) {
                NRF24L01_SetTxRxMode(TX_EN);
                phase = CX10_BIND1;
                while(1);
            }
            
        } else {
            //NRF24L01_SetTxRxMode(TXRX_OFF);
            if (try%30 == 0) {
                    printf("bind2\n");
		    NRF24L01_SetTxRxMode(TX_EN);
		    if (try%300 == 0) {
		      for(u8 i=0; i<4; i++)
		        packet[5+i] = 0xFF; // clear aircraft id
		    }
		    send_packet(1);
		    //usleep(300);
		    // switch to RX mode
		    //NRF24L01_SetTxRxMode(TXRX_OFF);
		    //NRF24L01_FlushRx();    // no stealing my data!
		    NRF24L01_SetTxRxMode(RX_EN);
		    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) 
		                  | BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_PRIM_RX));
            } 
        }
        break;

    case CX10_DATA:
        printf("data\n");
        send_packet(0);
        break;
    }
    return packet_period;
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void initialize_txid()
{
    u32 lfsr = 0x6E472105ul;

    // tx id
    txid[0] = (lfsr >> 24) & 0xFF;
    txid[1] = ((lfsr >> 16) & 0xFF) % 0x30;
    txid[2] = (lfsr >> 8) & 0xFF;
    txid[3] = lfsr & 0xFF;
    // rf channels
    rf_chans[0] = 0x03 + (txid[0] & 0x0F);
    rf_chans[1] = 0x16 + (txid[0] >> 4);
    rf_chans[2] = 0x2D + (txid[1] & 0x0F);
    rf_chans[3] = 0x40 + (txid[1] >> 4);
}

static void initialize()
{


    packet_size = CX10A_PACKET_SIZE;
    packet_period = CX10A_PACKET_PERIOD;
    bind_phase = CX10_BIND2;
    bind_counter=0;
    for(u8 i=0; i<4; i++)
        packet[5+i] = 0xFF; // clear aircraft id
    packet[9] = 0;


    initialize_txid();
    flags = 0;
    flags2 = 0;
    cx10_init();
    phase = CX10_INIT1;
    // CLOCK_StartTimer(INITIAL_WAIT, cx10_callback);
}



int main() {
  initialize();

  while(1)
  cx10_callback();
  
  return 0;
}

