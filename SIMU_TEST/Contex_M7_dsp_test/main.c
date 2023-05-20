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
#include "arm_math.h"  
#include "Board_LED.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DATA_LENGTH		    10240 		//FFT���ȣ�Ĭ����1024��FFT
#define FFT_LENGTH		    4096 		//FFT���ȣ�Ĭ����1024��FFT
#define DOWN_FEQ_RATE	    5 		    //�±�Ƶ��Ƶ�Ʊ��ʣ�Ƶ����������Ƶƽ�� DOWN_FEQ_RATE/DOWN_FEQ_RATE_MAX
#define DOWN_FEQ_MAX_RATE	10 		    //���Ƶ�Ʊ���
#define TEST_LENGTH_SAMPLES  1024    /* �������� */
#define BLOCK_SIZE           1         /* ����һ��arm_fir_f32����Ĳ�������� */
#define NUM_TAPS             12      /* �˲���ϵ������ */ 	
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static float fft_buffer_in[FFT_LENGTH*2];	//FFT��������
static float fft_buffer_out[FFT_LENGTH*2];	//FFT��������
static uint16_t data_buffer[DATA_LENGTH*2];	//ԭʼ��������
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static float firCoeffs32LP[NUM_TAPS] = {
  -0.001822523074f,  -0.001587929321f,  1.226008847e-18f,  0.003697750857f,  0.008075430058f,
  0.008530221879f,   -4.273456581e-18f, -0.01739769801f,   -0.03414586186f,  -0.03335915506f,
  8.073562366e-18f,  0.06763084233f   
};
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main()
{
  /* USER CODE BEGIN 1 */
	arm_cfft_radix4_instance_f32 scfft;
	arm_fir_instance_f32 scfir;
	uint16_t i; 
	uint32_t blockSize = BLOCK_SIZE;
  uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;            /* ��Ҫ����arm_fir_f32�Ĵ��� */
	float firStateF32[BLOCK_SIZE + NUM_TAPS - 1];   //fir״̬����
	//��ʼ��dsp�ṹ�壬�趨dsp��ز���
	arm_cfft_radix4_init_f32(&scfft,
						               FFT_LENGTH,
	                         0, //0-���任 1-���任
	                         1);
	arm_fir_init_f32(&scfir,                            
                   NUM_TAPS, 
                   (float32_t *)&firCoeffs32LP[0], 
                   &firStateF32[0], 
                   blockSize);
	LED_Initialize();
	LED_On(0);
  /* USER CODE END 1 */

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
		//************** ��ʼ�����������Լ������������� *******************//
		for(i=0; i<DATA_LENGTH; i++)//���ɳ�ʼ�ź�����
		{
			
			data_buffer[2*i]=100+
										10*arm_sin_f32(2*PI*i/FFT_LENGTH)+
										30*arm_sin_f32(2*PI*i*4/FFT_LENGTH)+
				            50*arm_cos_f32(2*PI*i*8/FFT_LENGTH);	//���������ź�ʵ��
			data_buffer[2*i+1]=70+
										30*arm_sin_f32(2*PI*i/FFT_LENGTH)+
										10*arm_sin_f32(2*PI*i*4/FFT_LENGTH)+
				            50*arm_cos_f32(2*PI*i*8/FFT_LENGTH);	//���������ź��鲿
		}

		//************** DSP�������̿�ʼ *******************//
		//�źų���
		for(i=0; i<FFT_LENGTH; i++)
		{
			fft_buffer_in[2*i] = data_buffer[1024*4 + 4*i];
			fft_buffer_in[2*i + 1]= data_buffer[1024*4 + 4*i + 1];
		}
	  
		//FFT���㣨��4��
		arm_cfft_radix4_f32(&scfft, fft_buffer_in);	
		//�±�Ƶ
		tranfer_length =  FFT_LENGTH - (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE ;
		memcpy(fft_buffer_out, fft_buffer_in , sizeof(fft_buffer_in));
		for(i=0; i < tranfer_length ; i++)
		{
			uint32_t tranfer_index = (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE * 2;
			fft_buffer_in[i] = fft_buffer_out[tranfer_index + 2*i];
			fft_buffer_in[FFT_LENGTH + i]= fft_buffer_out[tranfer_index + 2*i + 1];
		}
		memset(fft_buffer_out, 0 , sizeof(fft_buffer_out));
		//˫ͨ��fir��ͨ�˲�
		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer_in + (i * blockSize),  fft_buffer_out + (i * blockSize),  blockSize);
		}

		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer_in + FFT_LENGTH + (i * blockSize),  fft_buffer_out + FFT_LENGTH + (i * blockSize),  blockSize);
		}
		
		memset(fft_buffer_out, 0 , sizeof(fft_buffer_out));//end tips
		//************** DSP�������̽���*******************//
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
