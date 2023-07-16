#include <stdio.h>

#define logInfo(...) { printf("%010lu [INFO ] %s: ", HAL_GetTick(), CLASS_NAME); printf(__VA_ARGS__); }
#define logDebug(...) { printf("%010lu [DEBUG] %s: ", HAL_GetTick(), CLASS_NAME); printf(__VA_ARGS__); }
#define logWarn(...) { printf("%010lu [WARN ] %s: ", HAL_GetTick(), CLASS_NAME); printf(__VA_ARGS__); }
#define logError(...) { printf("%010lu [ERROR] %s: ", HAL_GetTick(), CLASS_NAME); printf(__VA_ARGS__); }