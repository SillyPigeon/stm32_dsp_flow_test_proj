/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "arm_math.h"  

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DATA_LENGTH		    10240 		//FFT长度，默认是1024点FFT
#define FFT_LENGTH		    4096 		//FFT长度，默认是1024点FFT
#define DOWN_FEQ_RATE	    5 		    //下变频，频移倍率，频谱整体往低频平移 DOWN_FEQ_RATE/DOWN_FEQ_RATE_MAX
#define DOWN_FEQ_MAX_RATE	10 		    //最大频移倍率
#define TEST_LENGTH_SAMPLES  1024    /* 采样点数 */
#define BLOCK_SIZE           1         /* 调用一次arm_fir_f32处理的采样点个数 */
#define NUM_TAPS             12      /* 滤波器系数个数 */ 	
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
const float firCoeffs32LP[NUM_TAPS] = {
  -0.001822523074f,  -0.001587929321f,  1.226008847e-18f,  0.003697750857f,  0.008075430058f,
  0.008530221879f,   -4.273456581e-18f, -0.01739769801f,   -0.03414586186f,  -0.03335915506f,
  8.073562366e-18f,  0.06763084233f   
};
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	arm_cfft_radix4_instance_f32 scfft;
	arm_fir_instance_f32 scfir;
	uint16_t i; 
	uint32_t blockSize = BLOCK_SIZE;
  uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;            /* 需要调用arm_fir_f32的次数 */
	float fft_buffer_in[FFT_LENGTH*2];	//FFT输入数组
	float fft_buffer_out[FFT_LENGTH*2];	//FFT输入数组
	float firStateF32[BLOCK_SIZE + NUM_TAPS - 1];   //fir状态缓存
	//初始化dsp结构体，设定dsp相关参数
	arm_cfft_radix4_init_f32(&scfft,
						               FFT_LENGTH,
	                         0, //0-正变换 1-反变换
	                         1);
	arm_fir_init_f32(&scfir,                            
                   NUM_TAPS, 
                   (float32_t *)&firCoeffs32LP[0], 
                   &firStateF32[0], 
                   blockSize);
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  //MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  //HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  //SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		uint32_t tranfer_length;
		//************** 初始化计数流程以及生成数据序列 *******************//
		
		uint16_t data_buffer[DATA_LENGTH*2];	//原始数据数组
		for(i=0; i<DATA_LENGTH; i++)//生成初始信号序列
		{
			
			data_buffer[2*i]=100+
										10*arm_sin_f32(2*PI*i/FFT_LENGTH)+
										30*arm_sin_f32(2*PI*i*4/FFT_LENGTH)+
				            50*arm_cos_f32(2*PI*i*8/FFT_LENGTH);	//生成输入信号实部
			data_buffer[2*i+1]=70+
										30*arm_sin_f32(2*PI*i/FFT_LENGTH)+
										10*arm_sin_f32(2*PI*i*4/FFT_LENGTH)+
				            50*arm_cos_f32(2*PI*i*8/FFT_LENGTH);	//生成输入信号虚部
		}

		//************** DSP处理流程开始 *******************//
		//信号抽样
		for(i=0; i<FFT_LENGTH; i++)
		{
			fft_buffer_in[2*i] = data_buffer[1024*4 + 4*i];
			fft_buffer_in[2*i + 1]= data_buffer[1024*4 + 4*i + 1];
		}
	  
		//FFT计算（基4）
		arm_cfft_radix4_f32(&scfft, fft_buffer_in);	
		//下变频
		tranfer_length =  FFT_LENGTH - (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE ;
		memcpy(fft_buffer_out, fft_buffer_in , sizeof(fft_buffer_in));
		for(i=0; i < tranfer_length ; i++)
		{
			uint32_t tranfer_index = (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE * 2;
			fft_buffer_in[i] = fft_buffer_out[tranfer_index + 2*i];
			fft_buffer_in[FFT_LENGTH + i]= fft_buffer_out[tranfer_index + 2*i + 1];
		}
		memset(fft_buffer_out, 0 , sizeof(fft_buffer_out));
		//双通道fir低通滤波
		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer_in + (i * blockSize),  fft_buffer_out + (i * blockSize),  blockSize);
		}

		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer_in + FFT_LENGTH + (i * blockSize),  fft_buffer_out + FFT_LENGTH + (i * blockSize),  blockSize);
		}
		
		memset(fft_buffer_out, 0 , sizeof(fft_buffer_out));;//end tips
		//************** DSP处理流程结束*******************//
    /* USER CODE BEGIN 3 */
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

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
