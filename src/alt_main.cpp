#include "alt_main.h"

#include "gpio.h"
#include "spi.h"
#include "usart.h"
#include "rtc.h"
#include "NRF24L01p.h"
#include <stdio.h>

#define DEVICE_IS_MASTER false

NRF24L01p nRF24L01p(&hspi1, NRF_CE_GPIO_Port, NRF_CE_Pin, NRF_CSN_GPIO_Port, NRF_CSN_Pin);

const uint8_t deviceAddress[5] = {0x00, 0x00, 0x00, 0x00, 0x01};

uint8_t data[32] = {0};

void printDeviceInfo();
void setLocalDateTime();
void setAlarmDateTime();
void handleReceiveDataEvent();
void goToStandByMode();


int alt_main() {
    HAL_Delay(100);
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    printDeviceInfo();

    









    
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

    //nRF24L01p.printAllRegisters();
    
    //date
    data[0] = 0x02;
    data[1] = 0x02;
    data[2] = 0x02;

    // time
    data[3] = 0x02;
    data[4] = 0x02;
    data[5] = 0x01;

    // alarm
    data[6] = 0x02;
    data[7] = 0x02;
    data[8] = 0x0A;


    if (!DEVICE_IS_MASTER) {
        if (nRF24L01p.isPowerUp() && nRF24L01p.isInRxMode() && nRF24L01p.isDataAvailable()) {
            // waked up by NRF IRQ
            printf("It seems we were waked up by NRF IRQ\r\n");
            handleReceiveDataEvent();

        } else if ((PWR->CSR) & (PWR_CSR_SBF)) {
            
            printf("It seems we were waked up from standby\r\n");
            // waked up by RTC alarm
            // TODO move NRF to RX and go standby

            RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
            
            // time
            data[6] = localTime.Hours;
            data[7] = localTime.Minutes;
            data[8] = localTime.Seconds;

            data[8] += 10;
            if (data[8] >= 60) {
                data[8] -= 60;
                data[7] += 1;
            }
            if (data[7] >= 60) {
                data[7] -= 60;
                data[6] += 1;
            }

            if (data[6] >= 24) {
                data[6] -= 24;
            }

            // TODO check years;
            setAlarmDateTime();
            HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
            HAL_Delay(2000);
            goToStandByMode();
        } else {
            printf("It seems we were waked up by power on\r\n");
            // TODO check current time
            // Configure NRF to RX mode
            // if date/time is not configured wait packet. Configure NRF to RX mode
            
            /*
            while(!nRF24L01p.isDataAvailable()) {
                // handle LED blink;
            //}
            handleReceiveDataEvent();
            */
            
            setLocalDateTime();
            setAlarmDateTime();
            HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
            HAL_Delay(2000);
            goToStandByMode();
          
        }
    }

    while(1) {
        printf("Error!!!\r\n");
    }
}

void printDeviceInfo() {
    printf("\r\n");
    printf("Bees Scale Device powered by Eugen Scobich\r\n");
    printf("Device Address: 0x%0X%0X%0X%0X%0X\r\n", deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3], deviceAddress[4]);
    printf("Device Mode: %s\r\n", DEVICE_IS_MASTER ? "Master" : "Slave");
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();

    printf("Device Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);
    printf("Device Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);

}

void setLocalDateTime() {
    // 0:2 bytes contains current date
    // 3:5 bytes contains current time
    HAL_RTC_SetLocalDate(data[0], data[1], data[1]);
    RTC_DateTypeDef localDate = HAL_RTC_GetLocalDate();
    printf("Set Date: %02d.%02d.%02d\r\n", localDate.Date, localDate.Month, localDate.Year);

    HAL_RTC_SetLocalTime(data[3], data[4], data[5]);
    RTC_TimeTypeDef localTime = HAL_RTC_GetLocalTime();
    printf("Set Time: %02d:%02d:%02d\r\n", localTime.Hours, localTime.Minutes, localTime.Seconds);
    
}

void setAlarmDateTime() {
    // 6:8 bytes contains alarm time
    HAL_RTC_SetLocalAlarm(data[6], data[7], data[8]);
    RTC_TimeTypeDef localAlarm = HAL_RTC_GetLocalAlarm();
    printf("Set Alarm Time: %02d:%02d:%02d\r\n", localAlarm.Hours, localAlarm.Minutes, localAlarm.Seconds);
}


void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
    printf("Alarm!!!\r\n");
}

void handleReceiveDataEvent(){
    nRF24L01p.receive(data);
    setLocalDateTime();
    setAlarmDateTime();

    // data 0 MCU temp
    // data 1:2 Bat voltage
    // data 3:7 Reserved

    // data 8:9 scale sensor 1
    // data 10:11 temp sensor 1
    // data 12:13 humidity sensor 1
    // data 14:15 reserved

    // data 16:17 scale sensor 1
    // data 18:19 temp sensor 2
    // data 20:21 humidity sensor 2
    // data 22:23 reserved

    // data 24:25 scale sensor 3
    // data 26:27 temp sensor 3
    // data 28:29 humidity sensor 3
    // data 30:31 reserved


    



    // TODO check sensors
    // TODO return data
    
    // TODO turn NRF off
    // TODO go standby
}

void goToStandByMode() {
    printf("Go to StandBy Mode\r\n");
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN);
    PWR->CR |= PWR_CR_CWUF;
    PWR->CSR |= PWR_CSR_EWUP;
    PWR->CR |= PWR_CR_CSBF;
    HAL_PWR_EnterSTANDBYMode();
}