#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdint.h>

#define I2C_Addr  0x68

 uint8_t second, minute, hour, date, month, year;


#define TRIG_PIN GPIO_PIN_9
#define TRIG_PORT GPIOA
#define ECHO_PIN GPIO_PIN_8
#define ECHO_PORT GPIOA


volatile uint8_t hrana_flag = 0;


void SystemClock_Config(void);

float MeasureDistance(void) {
    uint32_t pMillis;
    uint32_t Value1, Value2;
    float Distance;


    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < 10);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);


    pMillis = HAL_GetTick();
    while (!(HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) && (pMillis + 10 > HAL_GetTick()));


    Value1 = __HAL_TIM_GET_COUNTER(&htim1);


    pMillis = HAL_GetTick();
    while ((HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) && (pMillis + 50 > HAL_GetTick()));


    Value2 = __HAL_TIM_GET_COUNTER(&htim1);


    Distance = (Value2 - Value1) * 0.034 / 2;


    return Distance;
}
void Read_RTC_Time(void) {
    uint8_t rtc_buffer[7];


    HAL_I2C_Mem_Read(&hi2c1, I2C_Addr << 1, 0x00, 1, rtc_buffer, 7, HAL_MAX_DELAY);


    second = ((rtc_buffer[0] >> 4) * 10) + (rtc_buffer[0] & 0x0F);
    minute = ((rtc_buffer[1] >> 4) * 10) + (rtc_buffer[1] & 0x0F);
    hour   = ((rtc_buffer[2] >> 4) * 10) + (rtc_buffer[2] & 0x0F);
    date   = ((rtc_buffer[4] >> 4) * 10) + (rtc_buffer[4] & 0x0F);
    month  = ((rtc_buffer[5] >> 4) * 10) + (rtc_buffer[5] & 0x0F);
    year   = ((rtc_buffer[6] >> 4) * 10) + (rtc_buffer[6] & 0x0F);

    HAL_Delay(1000);
}

void Set_RTC_Time(uint8_t hour, uint8_t minute, uint8_t second,
                  uint8_t date, uint8_t month, uint8_t year) {
    uint8_t time_buffer[7];


    time_buffer[0] = ((second / 10) << 4) | (second % 10);
    time_buffer[1] = ((minute / 10) << 4) | (minute % 10);
    time_buffer[2] = ((hour / 10) << 4) | (hour % 10);
    time_buffer[3] = 1;
    time_buffer[4] = ((date / 10) << 4) | (date % 10);
    time_buffer[5] = ((month / 10) << 4) | (month % 10);
    time_buffer[6] = ((year / 10) << 4) | (year % 10);


    HAL_I2C_Mem_Write(&hi2c1, I2C_Addr << 1, 0x00, 1, time_buffer, 7, HAL_MAX_DELAY);
}

void Check_Distance_And_Control_LEDs(void) {
    float distance = MeasureDistance();


    if (distance < 1.50 || distance > 50) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    } else if (distance >= 1.50 && distance <= 4) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    }

    HAL_Delay(100);
}
void Pusti_Hranu(void) {

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500);  // Servo na 0°
    HAL_Delay(1500);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500); // Servo na 90°
    HAL_Delay(2000);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1000);  // Servo na 45°
    HAL_Delay(500);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500); // Servo na 90°
    HAL_Delay(1000);


            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500);  // Servo na 0°
            HAL_Delay(1000);


    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
}





int main(void)
{

  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();

 /*Set_RTC_Time(16, 22, 0, 9, 2, 25); // Sat, Minute, Sekunde, Dan, Mjesec, Godina
  HAL_Delay(500);*/


  HAL_TIM_Base_Start(&htim1);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);


  uint8_t last_feed_hour = 255;
  uint8_t last_feed_minute = 255;


  while (1)
  {

	  Check_Distance_And_Control_LEDs();
	  	      Read_RTC_Time();


	  	      if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET) {
	  	          HAL_Delay(1000);
	  	          continue;
	  	      }


	  	      if (((hour == 06 && minute == 30) || (hour == 12 && minute == 30) || (hour == 18 && minute == 30))
	  	    		  && (hour != last_feed_hour || minute != last_feed_minute))
	  	      {
	  	          Pusti_Hranu();
	  	          last_feed_hour = hour;
	  	          last_feed_minute = minute;
	  	      }

	  	    if (hrana_flag)
	  	        {
	  	            hrana_flag = 0;
	  	            Pusti_Hranu();
	  	        }

	  	      HAL_Delay(100);

  }

}


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
