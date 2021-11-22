#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "adc.h"
#include <math.h>
#include "arm_math.h"

#define FFT_LENGTH		256 		//FFT���ȣ�Ĭ����1024��FFT

uint32_t ADC_DataNum=0;			//ADC���������ֵ
extern __IO uint16_t ADCConvertedValue;		//ADC����ֵ(������)
volatile uint8_t ADC_TimeOutFlag=1;			  //ADC��ʱ����ʱ�䵽Flag

float fft_inputbuf[FFT_LENGTH*2];	//FFT��������
float fft_outputbuf[FFT_LENGTH];	//FFT�������

u16 colorValue[35];//������ɫ
u16 a[FFT_LENGTH][2];

void display(void);
void block_drop(int x_start, int y_start, int x_end, u16 block_color, int next_y);
 
int main(void)
{
	arm_cfft_radix4_instance_f32 scfft;
	u16 index=0;
	
	delay_init();//��ʱ������ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uart_init(115200);//���ڳ�ʼ����������115200bps
	LED_Init();//LED��ʼ��
	LCD_Init();//LCD��ʼ��
	
	TIM2_Configuration();//��ʱ��2��ʼ��
	TIM2_NVIC_Configuration();//��ʱ��2�ж�����
	FFT_RCC_Configuration();//FFT��ص�ʱ�ӳ�ʼ��
	FFT_GPIO_Configuration();//FFT��ص���������
	FFT_DMA_Init();//DMA��ʼ��
	FFT_ADC_Init();//ADC��ʼ��
	
	TIM_Cmd(TIM2, ENABLE);//��ʱ��2ʹ��
	ADC_SoftwareStartConvCmd(ADC1, DISABLE);
	
	LCD_ShowString(10,10,200,12,12,"Hello World!");
	//������ʾ����
	delay_ms(1000);
  LCD_Clear(WHITE);
	
	arm_cfft_radix4_init_f32(&scfft, FFT_LENGTH, 0, 1);
	for(index=0;index<35;index++)
	{
		colorValue[index]=rand();
	}
	while(1)
	{
		if(ADC_TimeOutFlag)
		{
			ADC_TimeOutFlag = 0;
			if(ADC_DataNum < FFT_LENGTH)
			{
				ADC_SoftwareStartConvCmd(ADC1, ENABLE);
				while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));				
				DMA_ClearFlag(DMA1_FLAG_TC1);            
				ADC_SoftwareStartConvCmd(ADC1, DISABLE);				
				fft_inputbuf[2*ADC_DataNum]=ADCConvertedValue;
				fft_inputbuf[2*ADC_DataNum+1]=0;
				ADC_DataNum++;			
			}
			else
			{
				TIM_Cmd(TIM2, DISABLE);
				ADC_DataNum = 0;
				//FFT�任
				arm_cfft_radix4_f32(&scfft, fft_inputbuf);
				//���ֵ
				arm_cmplx_mag_f32(fft_inputbuf,fft_outputbuf,FFT_LENGTH);				
				//��ʾ��TFT��LCD
				display();
				TIM_Cmd(TIM2, ENABLE);			
			}		
		}	
	}
}
void block_drop(int x_start, int y_start, int x_end, u16 block_color, int next_y)
{
   int i1=0;
//	 int i2=next_y;
	 if(next_y<=y_start)
		 LCD_Fill(x_start,y_start,x_end,y_start+3,block_color);
	 else
	 {
	    LCD_Fill(x_start,next_y,x_end,next_y+3,block_color);
		 for(i1=next_y;i1>=next_y+10;)
		 {
		   LCD_Fill(x_start,i1,x_end,i1+3,0xffff);
			 i1++;
			 LCD_Fill(x_start,i1,x_end,i1+3,block_color);
			 LCD_Fill(10,y_start+5,20,240,0xf800);
			 delay_ms(50);
			 if(!(i1>=y_start))i1=next_y+11;
		 }		 
	 }
}
//Ƶ����ʾ��LCD��
void display(void)
{
   u16 i,j=305;
	 for(i=1;i<32;i++)
	   {   
   			  a[i][0]=fft_outputbuf[i]/100;
			    
					LCD_Fill(j,2,j+10,240,WHITE   );
			    block_drop(j, a[i][0]-5, j+10, colorValue[i], a[i][1]);
          LCD_Fill(j,0,j+10,fft_outputbuf[i]/100,colorValue[i]);
			    j=j-10;	 
			 
			    a[i][1]=a[i][0];
		 }
		 delay_ms(25);
		
}
//��ʱ��2����жϣ���λADCת����־ 
void TIM2_IRQHandler(void)
{
   	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)
			{
         //TIM2->SR = (uint16_t)~TIM_FLAG_Update;
			   TIM_ClearITPendingBit(TIM2,TIM_FLAG_Update);	//���ж�
	       ADC_TimeOutFlag=1;
	    }
}
