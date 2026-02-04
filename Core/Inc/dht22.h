#ifndef __DHT22_H
#define __DHT22_H

#include "stm32f7xx_hal.h"

// Change pin here if needed
#define DHT22_PORT GPIOB
#define DHT22_PIN  GPIO_PIN_12

typedef struct
{
    float temperature;
    float humidity;
    uint8_t checksum_ok;
} DHT22_Data_t;

uint8_t DHT22_Read(DHT22_Data_t *data);

#endif
