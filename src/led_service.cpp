#include "led_service.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"

LedService::LedService() {}

void LedService::update() {
    uint32_t currentTick = HAL_GetTick();
    if (greenLedStarted) {
        if (greenLedPoused) {
            HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
            if (greenLedPreviousTick + greenLedPouseIntervalInTicks < currentTick) {
                greenLedPoused = false;
                greenLedCurrentBlink = 0;
                greenLedPreviousTick = currentTick;
            }
        } else {
            if (greenLedPreviousTick + greenLedBlinkIntervalInTicks < currentTick) {
                HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
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
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    greenLedBlinkIntervalInTicks = intervalInTicks;
    greenLedNumberOfToggle = nuberOfBlinks * 2;
    greenLedPouseIntervalInTicks = pouseIntervalInTicks;
    greenLedStarted = true;
    greenLedPoused = false;
    greenLedPreviousTick = HAL_GetTick();
}

void LedService::stopBlinkGreenLed() {
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    greenLedBlinkIntervalInTicks = 0;
    greenLedPouseIntervalInTicks = 0;
    greenLedNumberOfToggle = 0;
    greenLedStarted = false;
    greenLedPoused = false;
}