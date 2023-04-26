#ifndef __Log__
#define __Log__

#include "usart.h"

class Log {
private:
  UART_HandleTypeDef* huart;

public:
    Log::Log(UART_HandleTypeDef* _huart);
    void debug(char *logMessage);
};


#endif // __Log__