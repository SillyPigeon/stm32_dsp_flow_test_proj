#include "sys.h"
#include "delay.h"  
#include "usart.h"  
#include "led.h"
#include "key.h"
#include "timer.h" 
#include "math.h" 
#include "arm_math.h"  

 
//DSP FFT测试实验   -库函数版本
//STM32F4工程-库函数版本
//淘宝店铺：http://mcudev.taobao.com		

#define DATA_LENGTH		    10240 		//FFT长度，默认是1024点FFT
#define FFT_LENGTH		    4096 		//FFT长度，默认是1024点FFT
#define DOWN_FEQ_RATE	    5 		    //下变频，频移倍率，频谱整体往低频平移 DOWN_FEQ_RATE/DOWN_FEQ_RATE_MAX
#define DOWN_FEQ_MAX_RATE	10 		    //最大频移倍率

#define TEST_LENGTH_SAMPLES  1024    /* 采样点数 */
#define BLOCK_SIZE           1         /* 调用一次arm_fir_f32处理的采样点个数 */
#define NUM_TAPS             12      /* 滤波器系数个数 */

uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;            /* 需要调用arm_fir_f32的次数 */


float data_buffer[DATA_LENGTH*2];	//FFT输入数组
float fft_buffer[FFT_LENGTH*2];	//FFT输入数组
float firStateF32[BLOCK_SIZE + NUM_TAPS - 1];   //fir状态缓存

u8 timeout;//定时器溢出次数

/* 低通滤波器系数 通过fadtool获取*/
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

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  //初始化延时函数
	uart_init(115200);		//初始化串口波特率为115200
	
	LED_Init();					//初始化LED
	TIM3_Int_Init(65535,84-1);	//1Mhz计数频率,每1us计数一次，最大计时65ms左右超出

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
 	while(1)
	{
		u32 tranfer_length;
		//************** 初始化计数流程以及生成数据序列 *******************//
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
		TIM_SetCounter(TIM3,0);//重设TIM3定时器的计数器值
		timeout=0;

		//************** DSP处理流程开始 *******************//
		//信号抽样
		for(i=0; i<FFT_LENGTH; i++)
		{
			fft_buffer[2*i] = data_buffer[1024*4 + 4*i];
			fft_buffer[2*i + 1]= data_buffer[1024*4 + 4*i + 1];
		}
		//FFT计算（基4）
		arm_cfft_radix4_f32(&scfft, fft_buffer);	
		//下变频
		tranfer_length =  FFT_LENGTH - (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE ;
		memset(data_buffer, 0 , sizeof(data_buffer));
		memcpy(data_buffer, fft_buffer , sizeof(fft_buffer));
		for(i=0; i < tranfer_length ; i++)
		{
			u32 tranfer_index = (FFT_LENGTH / DOWN_FEQ_MAX_RATE) * DOWN_FEQ_RATE * 2;
			fft_buffer[i] = data_buffer[tranfer_index + 2*i];
			fft_buffer[FFT_LENGTH + i]= data_buffer[tranfer_index + 2*i + 1];
		}
		//转化为幅度谱
		//fir滤波
		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer + (i * blockSize),  data_buffer + (i * blockSize),  blockSize);
		}

		for(i=0; i < numBlocks; i++)
		{
			arm_fir_f32(&scfir, fft_buffer + FFT_LENGTH + (i * blockSize),  data_buffer + FFT_LENGTH + (i * blockSize),  blockSize);
		}
		
		//************** DSP处理流程结束 *******************//
		time=TIM_GetCounter(TIM3)+(u32)timeout*65536; 			//计算所用时间
		//************** 输出结果展示 *******************//
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
		
		//系统延迟
		//delay_ms(1000);
		//LED0=!LED0;
	}
}
 
