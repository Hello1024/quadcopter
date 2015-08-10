#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;

typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;

enum ProtoCmds {
    PROTOCMD_INIT,
    PROTOCMD_DEINIT,
    PROTOCMD_BIND,
    PROTOCMD_CHECK_AUTOBIND,
    PROTOCMD_NUMCHAN,
    PROTOCMD_DEFAULT_NUMCHAN,
    PROTOCMD_CURRENT_ID,
    PROTOCMD_GETOPTIONS,
    PROTOCMD_SETOPTIONS,
    PROTOCMD_TELEMETRYSTATE,
    PROTOCMD_RESET,
};

enum TXRX_State {
    TXRX_OFF,
    TX_EN,
    RX_EN,
};

enum PinConfigState {
    CSN_PIN,
    ENABLED_PIN,
    DISABLED_PIN,
    RESET_PIN,
};


enum TxPower {
    TXPOWER_100uW,
    TXPOWER_300uW,
    TXPOWER_1mW,
    TXPOWER_3mW,
    TXPOWER_10mW,
    TXPOWER_30mW,
    TXPOWER_100mW,
    TXPOWER_150mW,
    TXPOWER_LAST,
};


void usleep(int delay);
