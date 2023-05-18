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


#define FFT_LENGTH		4096 		//FFT���ȣ�Ĭ����1024��FFT

float fft_inputbuf[FFT_LENGTH*2];	//FFT��������
//float fft_outputbuf[FFT_LENGTH];	//FFT�������

u8 timeout;//��ʱ���������


int main(void)
{ 
	arm_cfft_radix4_instance_f32 scfft;
	float time;
	u16 i; 

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  //��ʼ����ʱ����
	uart_init(115200);		//��ʼ�����ڲ�����Ϊ115200
	
	LED_Init();					//��ʼ��LED
	TIM3_Int_Init(65535,84-1);	//1Mhz����Ƶ��,ÿ1us����һ�Σ�����ʱ65ms���ҳ���


	arm_cfft_radix4_init_f32(&scfft,FFT_LENGTH,0,1);//��ʼ��scfft�ṹ�壬�趨FFT��ز���
 	while(1)
	{
		//************** ��ʼ�����������Լ������������� *******************//
		for(i=0;i<FFT_LENGTH;i++)//�����ź�����
		{
			fft_inputbuf[2*i]=100+
							  10*arm_sin_f32(2*PI*i/FFT_LENGTH)+
							  30*arm_sin_f32(2*PI*i*4/FFT_LENGTH)+
				              50*arm_cos_f32(2*PI*i*8/FFT_LENGTH);	//���������ź�ʵ��
			fft_inputbuf[2*i+1]=0;//�鲿ȫ��Ϊ0
		}
		TIM_SetCounter(TIM3,0);//����TIM3��ʱ���ļ�����ֵ
		timeout=0;
		
		//************** DSP�������̿�ʼ *******************//
		arm_cfft_radix4_f32(&scfft, fft_inputbuf);	//FFT���㣨��4��
		
		
		time=TIM_GetCounter(TIM3)+(u32)timeout*65536; 			//��������ʱ��
		//************** DSP�������̽��� *******************//
		
		//************** ������չʾ *******************//
		//arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, FFT_LENGTH);	//��������������ģ�÷�ֵ 
		printf("\r\n%d point FFT runtime:%0.3fms\r\n", FFT_LENGTH, time/1000);
		printf("FFT Result:\r\n");
		//for(i=0;i<FFT_LENGTH;i++)
		//{
		//	printf("fft_outputbuf[%d]:%f\r\n",i,fft_outputbuf[i]);
		//}
		
		//ϵͳ�ӳ�
		delay_ms(1000);
		LED0=!LED0;	
	}
}
 
