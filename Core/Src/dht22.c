/**
 * @file    dht22.c
 * @brief   Reliable DHT22 (AM2302) driver for STM32 using DWT for µs timing
 * @note    Best results with EXTERNAL 4.7kΩ pull-up resistor!
 * @date    January 2025
 */

#include "dht22.h"
#include "main.h"           // for HAL_GetTick() and system defines

/* Make sure these are defined in dht22.h or main.h */
#define DHT22_PORT    GPIOB
#define DHT22_PIN     GPIO_PIN_12

/* ==================== DWT (Data Watchpoint and Trace) ==================== */

static void DWT_Init(void)
{
#if (__CORTEX_M == 7)   // STM32F7 needs unlock
    DWT->LAR = 0xC5ACCE55;  // Unlock DWT
#endif
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void delay_us(uint32_t us)
{
    if (us == 0) return;
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < cycles);
}

/* ==================== GPIO HELPERS ==================== */

static void DHT22_SetPinOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;     // Push-Pull is more reliable!
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

static void DHT22_SetPinInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;             // Internal pullup as fallback
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

/* ==================== MACROS ==================== */

#define DHT22_LOW()   HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET)
#define DHT22_HIGH()  HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET)
#define DHT22_READ()  HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN)

/* ==================== WAIT WITH TIMEOUT ==================== */

static uint8_t wait_for_pin(GPIO_PinState desired_state, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t timeout_cycles = timeout_us * (SystemCoreClock / 1000000U);

    while (DHT22_READ() != desired_state)
    {
        if ((DWT->CYCCNT - start) > timeout_cycles)
            return 0; // timeout
    }
    return 1; // success
}

/* ==================== MAIN DHT22 READ FUNCTION ==================== */
/**
 * @brief   Read temperature and humidity from DHT22
 * @param   data   Pointer to store the result
 * @return  1 = success, 0 = failure (checksum or timeout)
 * @note    Call this function no more often than every ~2 seconds
 */
/**
 * @brief  Reads temperature and humidity from DHT22 sensor
 * @param  data: Pointer to DHT22_Data_t structure to store results
 * @return 1 = success (valid data + checksum OK), 0 = failure
 * @note   Minimum interval between calls: ~2 seconds
 */
uint8_t DHT22_Read(DHT22_Data_t *data)
{
    uint8_t bits[5] = {0};
    uint8_t i, j;

    // Initialize DWT cycle counter only once
    static uint8_t dwt_initialized = 0;
    if (!dwt_initialized)
    {
        DWT_Init();
        dwt_initialized = 1;
        printf("DHT22: DWT initialized\r\n");
    }

    // Disable interrupts during critical timing section
    __disable_irq();

    printf("DHT22: Starting read sequence\r\n");

    // 1. Set pin as output and idle HIGH
    DHT22_SetPinOutput();
    DHT22_HIGH();
    HAL_Delay(2);               // Give it time to stabilize

    // 2. Send start signal: LOW for at least 18-30 ms
    printf("DHT22: Pulling line LOW (start signal)...\r\n");
    DHT22_LOW();
    HAL_Delay(25);              // Increased to 25 ms - more reliable for many modules

    printf("DHT22: Releasing line HIGH\r\n");
    DHT22_HIGH();
    delay_us(50);               // Slightly longer release time

    // 3. Switch to input mode
    printf("DHT22: Switching to input mode\r\n");
    DHT22_SetPinInput();

    // 4. Wait for sensor response sequence (80-200 µs low → 80 µs high → low)
    printf("DHT22: Waiting for sensor response (first LOW)... ");
    if (!wait_for_pin(GPIO_PIN_RESET, 300))
    {
        printf("TIMEOUT!\r\n");
        goto fail;
    }
    printf("OK\r\n");

    printf("DHT22: Waiting for HIGH... ");
    if (!wait_for_pin(GPIO_PIN_SET, 150))
    {
        printf("TIMEOUT!\r\n");
        goto fail;
    }
    printf("OK\r\n");

    printf("DHT22: Waiting for second LOW... ");
    if (!wait_for_pin(GPIO_PIN_RESET, 150))
    {
        printf("TIMEOUT!\r\n");
        goto fail;
    }
    printf("OK - sensor is responding!\r\n");

    // 5. Read 40 bits (5 bytes)
    printf("DHT22: Reading 40 bits...\r\n");
    for (j = 0; j < 5; j++)
    {
        for (i = 0; i < 8; i++)
        {
            // Wait for rising edge (start of bit)
            if (!wait_for_pin(GPIO_PIN_SET, 120))
            {
                printf("DHT22: Bit timeout - waiting HIGH at byte %d bit %d\r\n", j, i);
                goto fail;
            }

            // Sample after ~50-60 µs (sweet spot for most DHT22/AM2302)
            delay_us(58);   // ← Tuned value: 55-62 µs works best in practice

            if (DHT22_READ() == GPIO_PIN_SET)
            {
                bits[j] |= (1U << (7 - i));
            }

            // Wait for falling edge (end of bit)
            if (!wait_for_pin(GPIO_PIN_RESET, 120))
            {
                printf("DHT22: Bit timeout - waiting LOW at byte %d bit %d\r\n", j, i);
                goto fail;
            }
        }
    }

    __enable_irq();  // End of critical section

    // 6. Checksum verification
    uint8_t checksum = bits[0] + bits[1] + bits[2] + bits[3];
    printf("DHT22: Checksum calc = 0x%02X, received = 0x%02X ", checksum, bits[4]);
    if (checksum != bits[4])
    {
        printf("→ FAIL\r\n");
        return 0;
    }
    printf("→ OK\r\n");

    // 7. Convert raw values to real units
    uint16_t hum_raw  = (bits[0] << 8) | bits[1];
    uint16_t temp_raw = (bits[2] << 8) | bits[3];

    data->humidity = hum_raw / 10.0f;

    if (temp_raw & 0x8000)   // negative temperature
    {
        temp_raw &= 0x7FFF;
        data->temperature = -(temp_raw / 10.0f);
    }
    else
    {
        data->temperature = temp_raw / 10.0f;
    }

    data->checksum_ok = 1;

    printf("DHT22: SUCCESS → Temp = %.1f °C   Humidity = %.1f %%\r\n",
           data->temperature, data->humidity);

    return 1;

fail:
    __enable_irq();
    printf("DHT22: READ FAILED\r\n");
    data->checksum_ok = 0;
    return 0;
}
