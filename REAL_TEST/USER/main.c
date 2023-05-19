#include "sys.h"
#include "delay.h"  
#include "usart.h"  
#include "led.h"
#include "key.h"
#include "timer.h" 
#include "math.h" 
#include "arm_math.h"  

 
//DSP FFT����ʵ��   -�⺯���汾
//STM32F4����-�⺯���汾
//�Ա����̣�http://mcudev.taobao.com		

#define DATA_LENGTH		    10240 		//FFT���ȣ�Ĭ����1024��FFT
#define FFT_LENGTH		    4096 		//FFT���ȣ�Ĭ����1024��FFT
#define DOWN_FEQ_RATE	    5 		    //�±�Ƶ��Ƶ�Ʊ��ʣ�Ƶ����������Ƶƽ�� DOWN_FEQ_RATE/DOWN_FEQ_RATE_MAX
#define DOWN_FEQ_MAX_RATE	10 		    //���Ƶ�Ʊ���

#define TEST_LENGTH_SAMPLES  1024    /* �������� */
#define BLOCK_SIZE           1         /* ����һ��arm_fir_f32����Ĳ�������� */
#define NUM_TAPS             12      /* �˲���ϵ������ */

uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;            /* ��Ҫ����arm_fir_f32�Ĵ��� */


float data_buffer[DATA_LENGTH*2];	//FFT��������
float fft_buffer[FFT_LENGTH*2];	//FFT��������
float firStateF32[BLOCK_SIZE + NUM_TAPS - 1];   //fir״̬����

u8 timeout;//��ʱ���������

/* ��ͨ�˲���ϵ�� ͨ��fadtool��ȡ*/
const float firCoeffs32LP[NUM_TAPS] = {
  -0.001822523074f,  -0.001587929321f,  1.226008847e-18f,  0.003697750857f,  0.008075430058f,
  0.008530221879f,   -4.273456581e-18f, -0.01739769801f,   -0.03414586186f,  -0.03335915506f,
  8.073562366e-18f,  0.06763084233f   
};

int main(void)
{ 
	arm_cfft_radix4_instance_f32 scfft;
	arm_fir_instance_f32 scfir;
	float time;
	u16 i; 

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  //��ʼ����ʱ����
	uart_init(115200);		//��ʼ�����ڲ�����Ϊ115200
	
	LED_Init();					//��ʼ��LED
	TIM3_Int_Init(65535,84-1);	//1Mhz����Ƶ��,ÿ1us����һ�Σ�����ʱ65ms���ҳ���

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
 	while(1)
	{
		u32 tranfer_length;
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
		TIM_SetCounter(TIM3,0);//����TIM3��ʱ���ļ�����ֵ
		timeout=0;

		//************** DSP�������̿�ʼ *******************//
		//�źų���
		for(i=0; i<FFT_LENGTH; i++)
		{
			fft_buffer[2*i] = data_buffer[1024*4 + 4*i];
			fft_buffer[2*i + 1]= data_buffer[1024*4 + 4*i + 1];
		}
		//FFT���㣨��4��
		arm_cfft_radix4_f32(&scfft, fft_buffer);	
		//�±�Ƶ
		tranfer_length =  FFT_LENGTH - (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE ;
		memset(data_buffer, 0 , sizeof(data_buffer));
		memcpy(data_buffer, fft_buffer , sizeof(fft_buffer));
		for(i=0; i < tranfer_length ; i++)
		{
			u32 tranfer_index = (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE * 2;
			fft_buffer[i] = data_buffer[tranfer_index + 2*i];
			fft_buffer[FFT_LENGTH + i]= data_buffer[tranfer_index + 2*i + 1];
		}
		//ת��Ϊ������
		//fir�˲�
		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer + (i * blockSize),  data_buffer + (i * blockSize),  blockSize);
		}

		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer + FFT_LENGTH + (i * blockSize),  data_buffer + FFT_LENGTH + (i * blockSize),  blockSize);
		}
		
		//************** DSP�������̽��� *******************//
		time=TIM_GetCounter(TIM3)+(u32)timeout*65536; 			//��������ʱ��
		//************** ������չʾ *******************//
		//printf("\r\n%d point FFT runtime:%0.3fms\r\n", FFT_LENGTH, time/1000);
		//printf("FFT Real Result:\r\n");
		//printf("[");
		//for(i=0;i<FFT_LENGTH;i++)
		//{
		//	printf("%f ",fft_buffer[i]);
		//}
		//printf("]\r\n\r\n");
		
		//printf("FFT Imz Result:\r\n");
		//printf("[");
		//for(i=0;i<FFT_LENGTH;i++)
		//{
		//	printf("%f ",fft_buffer[FFT_LENGTH + i]);
		//}
		//printf("]\r\n");
		
		//ϵͳ�ӳ�
		//delay_ms(1000);
		//LED0=!LED0;
	}
}
 
