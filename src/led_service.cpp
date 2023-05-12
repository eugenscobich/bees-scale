#include "led_service.h"

LedService::LedService() {}

void LedService::update(State* state) {
    uint32_t currentHalTick = HAL_GetTick();
    if (blinkGreenIntervalInTicks > 0 && previousGreenHalTick + blinkGreenIntervalInTicks < currentHalTick) {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
        previousGreenHalTick = currentHalTick;
    }
}

void LedService::blinkGreenLed(uint16_t intervalInTicks) {
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    blinkGreenIntervalInTicks = intervalInTicks;
    previousGreenHalTick = HAL_GetTick();
}

void LedService::stopBlinkGreenLed() {
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
    blinkGreenIntervalInTicks = 0;
}
