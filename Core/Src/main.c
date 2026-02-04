/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DHT22 Environmental Data Logger
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dht22.h"
#include "Sd_spi.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
char uart_buf[100];
char csv_buffer[150];
uint32_t sample_count = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void DHT22_DataLogger(void)
{
    printf("\r\n");
    printf("================================================\r\n");
    printf("  STM32 DHT22 Environmental Data Logger\r\n");
    printf("  Temperature & Humidity Monitoring System\r\n");
    printf("================================================\r\n\r\n");

    // Mount SD Card
    printf("Step 1: Mounting SD Card...\r\n");
    if (sd_mount() != 0)
    {
        printf("ERROR: SD Card Mount Failed!\r\n");
        printf("   Check SD card and connections.\r\n");
        return;
    }
    printf("✓ SD Card Mounted Successfully\r\n\r\n");

    // Create CSV file with header
    printf("Step 2: Creating CSV file...\r\n");
    const char *filename = "env_log.csv";
    const char *header = "Sample,Temperature(C),Humidity(%),Status,Timestamp\r\n";

    if (sd_write_file(filename, header) != 0)
    {
        printf("Failed to create CSV file!\r\n");
        sd_unmount();
        return;
    }
    printf("✓ File created: %s\r\n\r\n", filename);

    // Log 10 samples
    printf("Step 3: Logging 10 Environmental Samples...\r\n");
    printf("================================================\r\n");

    DHT22_Data_t dht_data;

    for (uint32_t sample = 1; sample <= 10; sample++)
    {
        printf("\r\n[Sample %02lu/10] Reading DHT22 sensor...\r\n", sample);

        // Read DHT22
        uint8_t read_ok = DHT22_Read(&dht_data);

        if (read_ok && dht_data.checksum_ok)
        {
            // Determine status based on conditions
            const char *status;
            if (dht_data.temperature < 15.0f || dht_data.temperature > 30.0f)
                status = "ALERT";
            else if (dht_data.humidity < 30.0f || dht_data.humidity > 70.0f)
                status = "WARNING";
            else
                status = "NORMAL";

            // Get timestamp (using sample counter for now)
            uint32_t timestamp = HAL_GetTick() / 1000; // Seconds since boot

            // Print to UART
            printf("  Temperature: %.1f°C\r\n", dht_data.temperature);
            printf("  Humidity:    %.1f%%\r\n", dht_data.humidity);
            printf("  Status:      %s\r\n", status);
            printf("  Time:        %lu seconds\r\n", timestamp);

            // Format CSV row
            snprintf(csv_buffer, sizeof(csv_buffer),
                     "%lu,%.1f,%.1f,%s,%lu\r\n",
                     sample,
                     dht_data.temperature,
                     dht_data.humidity,
                     status,
                     timestamp);

            // Write to SD card
            if (sd_append_file(filename, csv_buffer) == 0)
            {
                printf("  ✓ Saved to SD card\r\n");
            }
            else
            {
                printf("  ✗ Failed to write to SD card\r\n");
            }
        }
        else
        {
            printf("  ✗ DHT22 Read Failed (Checksum Error)\r\n");

            // Log error to CSV
            snprintf(csv_buffer, sizeof(csv_buffer),
                     "%lu,ERROR,ERROR,READ_FAIL,%lu\r\n",
                     sample,
                     HAL_GetTick() / 1000);
            sd_append_file(filename, csv_buffer);
        }

        // Wait 2 seconds between readings (DHT22 minimum interval)
        HAL_Delay(2000);
    }

    printf("\r\n================================================\r\n");
    printf("✓ All 10 samples logged successfully!\r\n");
    printf("================================================\r\n\r\n");

    // List files on SD card
    printf("Step 4: Files on SD Card:\r\n");
    sd_list_files();

    // Unmount SD card
    printf("\r\nStep 5: Unmounting SD card...\r\n");
    sd_unmount();

    printf("\r\n");
    printf("================================================\r\n");
    printf("       DATA LOGGING COMPLETE!                   \r\n");
    printf("  Remove SD card and check 'env_log.csv'        \r\n");
    printf("================================================\r\n");
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  // Pre-init DHT22 pin as OUTPUT HIGH (idle state for DHT22)
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT22_PIN;         // GPIO_PIN_12 on GPIOB
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;      // external pull-up handles this
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);  // idle HIGH

  printf("DHT22 pin pre-initialized as OUTPUT HIGH\r\n");
  HAL_Delay(100);  // just to be safe



  printf("\r\n\r\n");
  printf("STM32F767ZI Environmental Data Logger\r\n");
  printf("DHT22 Sensor + SD Card Storage\r\n");
  HAL_Delay(1000);

  // Run data logger once
  DHT22_DataLogger();

  printf("\r\n💡 TIP: Insert SD card in PC to view env_log.csv\r\n");
  printf("💡 TIP: Open CSV in Excel/Google Sheets for graphs\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

      // Continuous monitoring mode (optional)
      DHT22_Data_t dht_data;

      if (DHT22_Read(&dht_data) && dht_data.checksum_ok)
      {
          snprintf(uart_buf, sizeof(uart_buf),
                   "Temp: %.1f°C | Humidity: %.1f%%\r\n",
                   dht_data.temperature,
                   dht_data.humidity);
          HAL_UART_Transmit(&huart3, (uint8_t*)uart_buf, strlen(uart_buf), 200);
      }

      HAL_Delay(5000); // Read every 5 seconds
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PD14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
