#include "led_service.h"

LedService::LedService() {}

void LedService::update() {
    uint32_t currentTick = HAL_GetTick();
    if (greenLedStarted) {
        if (greenLedPoused) {
            HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
            if (greenLedPreviousTick + greenLedPouseIntervalInTicks < currentTick) {
                greenLedPoused = false;
                greenLedCurrentBlink = 0;
                greenLedPreviousTick = currentTick;
            }
        } else {
            if (greenLedPreviousTick + greenLedBlinkIntervalInTicks < currentTick) {
                HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
                greenLedPreviousTick = currentTick;
                if (greenLedNumberOfToggle != 0) {
                    greenLedCurrentBlink++;
                    if (greenLedCurrentBlink == greenLedNumberOfToggle) {
                        greenLedPoused = true;
                    }
                }
            }
        }
    }
}

void LedService::blinkGreenLed(uint8_t nuberOfBlinks, uint16_t intervalInTicks, uint16_t pouseIntervalInTicks) {
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    greenLedBlinkIntervalInTicks = intervalInTicks;
    greenLedNumberOfToggle = nuberOfBlinks * 2;
    greenLedPouseIntervalInTicks = pouseIntervalInTicks;
    greenLedStarted = true;
    greenLedPoused = false;
    greenLedPreviousTick = HAL_GetTick();
}

void LedService::stopBlinkGreenLed() {
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
    greenLedBlinkIntervalInTicks = 0;
    greenLedPouseIntervalInTicks = 0;
    greenLedNumberOfToggle = 0;
    greenLedStarted = false;
    greenLedPoused = false;
}