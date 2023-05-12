#ifndef __STATE_H__
#define __STATE_H__

#define CMD_UNKNOWN 0
#define CMD_CHECK_MODEM_AND_POWER_ON_IF_NEED 1
#define CMD_READ_SMS 2

#define STATE_UNKNOWN 0
#define STATE_MODEM_STATE_UNKOWN 1
#define STATE_MODEM_POWERED_ON 2
#define STATE_MODEM_STATE_ERROR 3

typedef struct {
    uint8_t previousCmd;
    uint8_t currentCmd;
    uint8_t nextCmd;
    uint8_t previousState;
    uint8_t currentState;
} State;

#endif /* __STATE_H__ */