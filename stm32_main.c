/**
  * Car Park Access Control System - STM32
  * 
  * This program:
  * 1. Detects cars using a sensor on GPIO_PIN_1
  * 2. Sends car detection events to Raspberry Pi via UART
  * 3. Controls a servo for barrier gate operation
  * 4. Displays status messages on SSD1306 OLED display
  */

  #include "main.h"
  #include "ssd1306.h"
  #include "ssd1306_fonts.h"
  #include <string.h>
  
  I2C_HandleTypeDef hi2c1;
  TIM_HandleTypeDef htim2;
  UART_HandleTypeDef huart1;
  
  void SystemClock_Config(void);
  static void MX_GPIO_Init(void);
  static void MX_TIM2_Init(void);
  static void MX_I2C1_Init(void);
  static void MX_USART1_UART_Init(void);
  
  // States for car detection
  typedef enum {
    NO_CAR,
    WAITING_RESPONSE,
    OPEN_GATE,
    CLOSE_GATE
  } SystemState;
  
  // Constants for servo positions
  #define SERVO_CLOSED 250
  #define SERVO_OPEN   750
  
  // UART message buffer
  #define UART_BUFFER_SIZE 20
  
  int main(void)
  {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
  
    // Start PWM for servo control
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    
    // Set initial servo position (gate closed)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, SERVO_CLOSED);
    
    // Configure UART with longer timeout
    huart1.Init.BaudRate = 115200; // Ensure baud rate matches Raspberry Pi
    HAL_UART_Init(&huart1);
  
    // Initialize OLED display
    ssd1306_Init();
    ssd1306_Fill(White);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Car Park System", Font_6x8, Black);
    ssd1306_SetCursor(0, 20);
    ssd1306_WriteString("Ready", Font_6x8, Black);
    ssd1306_UpdateScreen();
    
    // Variables
    SystemState state = NO_CAR;
    uint8_t carDetectedPrevious = 0;
    char uartRxBuffer[UART_BUFFER_SIZE];
    uint32_t lastCarDetectionTime = 0;
    
    while (1)
    {
      // Check car sensor
      GPIO_PinState carSensor = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
      uint8_t carDetected = (carSensor == GPIO_PIN_SET) ? 1 : 0;
      uint32_t currentTime = HAL_GetTick();
      
      // Simplified UART receive logic - receive 1-2 bytes at a time with shorter timeout
      memset(uartRxBuffer, 0, UART_BUFFER_SIZE);
      HAL_StatusTypeDef rxStatus = HAL_UART_Receive(&huart1, (uint8_t*)uartRxBuffer, 2, 100);
      
      // State machine
      switch(state) {
        case NO_CAR:
          if (carDetected && !carDetectedPrevious) {
            // Car just arrived
            ssd1306_Fill(White);
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString("Car Detected", Font_6x8, Black);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString("Checking...", Font_6x8, Black);
            ssd1306_UpdateScreen();
            
            // Send car detection event to Raspberry Pi
            char *carDetectMsg = "CAR_DETECTED\n";
            
            // Retry mechanism with shorter messages and more reliable transmission
            for (int retry = 0; retry < 5; retry++) {
              HAL_StatusTypeDef txStatus = HAL_UART_Transmit(&huart1, (uint8_t*)carDetectMsg, strlen(carDetectMsg), 100);
              if (txStatus == HAL_OK) break;
              HAL_Delay(20);  // Short delay between retries
            }
            
            state = WAITING_RESPONSE;
            lastCarDetectionTime = currentTime;
          }
          break;
          
        case WAITING_RESPONSE:
          // Check if we received a response from Raspberry Pi
          if (rxStatus == HAL_OK) {
            // For debugging - show what was received
            ssd1306_SetCursor(0, 40);
            ssd1306_WriteString(uartRxBuffer, Font_6x8, Black);
            ssd1306_UpdateScreen();
            
            // Check first characters only
            if (uartRxBuffer[0] == 'O' && uartRxBuffer[1] == 'K') {
              // Authorized car, open gate
              ssd1306_Fill(White);
              ssd1306_SetCursor(10, 10);
              ssd1306_WriteString("Access", Font_6x8, Black);
              ssd1306_SetCursor(10, 30);
              ssd1306_WriteString("Granted", Font_6x8, Black);
              ssd1306_UpdateScreen();
              
              // Open the gate
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, SERVO_OPEN);
              
              state = OPEN_GATE;
            } else if (uartRxBuffer[0] == 'N' && uartRxBuffer[1] == 'O') {
              // Unauthorized car
              ssd1306_Fill(White);
              ssd1306_SetCursor(10, 10);
              ssd1306_WriteString("Access", Font_6x8, Black);
              ssd1306_SetCursor(10, 30);
              ssd1306_WriteString("Denied", Font_6x8, Black);
              ssd1306_UpdateScreen();
              
              // Keep gate closed
              state = NO_CAR;
            }
          }
          
          // Timeout handling
          if (currentTime - lastCarDetectionTime > 5000) {
            // No response for 5 seconds
            ssd1306_Fill(White);
            ssd1306_SetCursor(0, 10);
            ssd1306_WriteString("Timeout", Font_6x8, Black);
            ssd1306_UpdateScreen();
            state = NO_CAR;
          }
          break;
          
        case OPEN_GATE:
          // Check if car has passed (sensor no longer detecting)
          if (!carDetected && carDetectedPrevious) {
            // Car has passed through the gate
            ssd1306_Fill(White);
            ssd1306_SetCursor(10, 20);
            ssd1306_WriteString("Closing", Font_6x8, Black);
            ssd1306_UpdateScreen();
            
            // Wait a bit before closing gate
            HAL_Delay(1000);
            
            // Close the gate
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, SERVO_CLOSED);
            
            state = CLOSE_GATE;
          }
          break;
          
        case CLOSE_GATE:
          // After gate closed, display ready message
          ssd1306_Fill(White);
          ssd1306_SetCursor(10, 20);
          ssd1306_WriteString("Ready", Font_6x8, Black);
          ssd1306_UpdateScreen();
          
          state = NO_CAR;
          break;
      }
      
      // Save current car detection state
      carDetectedPrevious = carDetected;
      
      HAL_Delay(100);
    }
  }
  


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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
