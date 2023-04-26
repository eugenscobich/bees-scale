#include "Log.h"

Log::Log(UART_HandleTypeDef* _huart) :
    huart(_huart) {
    
}

void Log::debug(char *logMessage) {
    
}