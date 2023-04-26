#include "alt_main.h"

#include "gpio.h"
#include "spi.h"
#include "usart.h"
#include "NRF24L01p.h"

NRF24L01p nRF24L01p(&hspi1, NRF_CE_GPIO_Port, NRF_CE_Pin, NRF_CSN_GPIO_Port, NRF_CSN_Pin, &huart1);

const uint64_t address = 0xF0F0F0F0E1LL;
uint8_t button_state = 0;

uint8_t buff[100];

int alt_main() {
    char message[] = "Hello World\r\n";
    for (uint16_t i = 0; i < strlen(message); i++)
    {
        buff[i] = message[i];
    }
    HAL_UART_Transmit(&huart1, buff, strlen(message), 2000);

    nRF24L01p.init();
    nRF24L01p.openWritingPipe(0xF0F0F0F0E1LL, 76);
    nRF24L01p.printAllRegisters();
/*
    radio.begin();                  //Starting the Wireless communication
    radio.openWritingPipe(address); //Setting the address where we will send the data
    radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
    radio.stopListening();          //This sets the module as transmitter
*/
	while (1) { 

        const char text[] = "Your Button State is HIGH";
        for (uint16_t i = 0; i < strlen(message); i++) {
            buff[i] = message[i];
        }
        bool result = nRF24L01p.write(buff); 

        buff[0] = result ? '1' : '0';
        buff[1] = '\r';
        buff[2] = '\n';
        HAL_UART_Transmit(&huart1, buff, 3, 2000);

        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, result ? GPIO_PIN_SET : GPIO_PIN_RESET);
        

        HAL_Delay(1000);

/*

        if(HAL_GPIO_ReadPin(BOOT1_GPIO_Port, BOOT1_Pin) == GPIO_PIN_SET) {

            const char text[] = "Your Button State is HIGH";
            radio.write(&text, sizeof(text));   
            
            char message[] = "Your Button State is HIGH\r\n";
            for (uint16_t i = 0; i < strlen(message); i++)
            {
                buff[i] = message[i];
            }
            HAL_UART_Transmit(&huart1, buff, strlen(message), 2000); 

        } else {
            const char text[] = "Your Button State is LOW";
            radio.write(&text, sizeof(text));   
            
            char message[] = "Your Button State is LOW\r\n";
            for (uint16_t i = 0; i < strlen(message); i++)
            {
                buff[i] = message[i];
            }
            HAL_UART_Transmit(&huart1, buff, strlen(message), 2000); 
        }


        radio.write(&button_state, sizeof(button_state));  //Sending the message to receiver 
        HAL_UART_Transmit(&huart1, &button_state, 1, 2000); 
        HAL_Delay(1000);
*/
	}


}

