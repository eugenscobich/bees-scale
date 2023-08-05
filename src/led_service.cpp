#include "led_service.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"

LedService::LedService() {}

void LedService::update() {
    uint32_t currentTick = HAL_GetTick();


    if (redAndOrangeLedStarted) {
        if (!greenLedStarted && redLedCurrentBlink == redAndOrangeRedLedNumberOfBlinks * 2) {
            greenLedStarted = true;
            greenLedPoused = false;
            greenLedCurrentBlink = 0;
            greenLedPreviousTick = redLedPreviousTick;
        }
        if (greenLedStarted && greenLedPoused) {
            greenLedStarted = false;
        }
    }

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

    if (redLedStarted) {
        if (redLedPoused) {
            HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
            if (redLedPreviousTick + redLedPouseIntervalInTicks < currentTick) {
                redLedPoused = false;
                redLedCurrentBlink = 0;
                redLedPreviousTick = currentTick;
            }
        } else {
            if (redLedPreviousTick + redLedBlinkIntervalInTicks < currentTick) {
                HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
                redLedPreviousTick = currentTick;
                if (redLedNumberOfToggle != 0) {
                    redLedCurrentBlink++;
                    if (redLedCurrentBlink == redLedNumberOfToggle) {
                        redLedPoused = true;
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

void LedService::blinkRedLed(uint8_t nuberOfBlinks, uint16_t intervalInTicks, uint16_t pouseIntervalInTicks) {
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
    redLedBlinkIntervalInTicks = intervalInTicks;
    redLedNumberOfToggle = nuberOfBlinks * 2;
    redLedPouseIntervalInTicks = pouseIntervalInTicks;
    redLedStarted = true;
    redLedPoused = false;
    redLedPreviousTick = HAL_GetTick();
}

void LedService::stopBlinkRedLed() {
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
    redLedBlinkIntervalInTicks = 0;
    redLedPouseIntervalInTicks = 0;
    redLedNumberOfToggle = 0;
    redLedStarted = false;
    redLedPoused = false;
}

void LedService::blinkErrorCode(uint8_t errorCode, uint16_t intervalInTicks, uint16_t pouseIntervalInTicks) {
    redAndOrangeRedLedNumberOfBlinks = errorCode / 10;
    redAndOrangeLedStarted = true;
    redAndOrangeLedPoused = false;

    blinkRedLed(errorCode, intervalInTicks, pouseIntervalInTicks);
    blinkGreenLed(errorCode % 10, intervalInTicks, pouseIntervalInTicks);
    greenLedStarted = false;
}

void LedService::stopBlinkErrorCode() {
    redAndOrangeRedLedNumberOfBlinks = 0;
    redAndOrangeOrangeLedNumberOfBlinks = 0;
    redAndOrangeLedPoused = 0;

    stopBlinkRedLed();
    stopBlinkGreenLed();
}


void LedService::blinkGreenRedOrangeLedOneTime() {
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);   
}