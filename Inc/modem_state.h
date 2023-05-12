#ifndef __MODEM_STATE_H__
#define __MODEM_STATE_H__

#define MODEM_CMD_CHECK_MODEM_AND_POWER_ON_IF_NEED 0
#define MODEM_CMD_UNKNOWN 1

#define MODEM_STATE_UNKNOWN 0
#define MODEM_STATE_RUNNING 1
#define MODEM_STATE_SUCCESS 2
#define MODEM_STATE_ERROR 3

typedef struct {
    uint8_t previousCmd;
    uint8_t currentCmd;
    uint8_t nextCmd;
    uint8_t previousState;
    uint8_t currentState;
} ModemState;

#endif /* __MODEM_STATE_H__ */