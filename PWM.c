
#include<reg52.h>

#define CRYSTAL	   22118400
#define TIMEUS 	   30	
#define TIMER_TH0 (65536 - (CRYSTAL / 12000000) * TIMEUS / 256)
#define TIMER_TL0 (65536 - (CRYSTAL / 12000000) * TIMEUS % 256)
#define DELAYUS(x) (x)/ TIMEUS

typedef unsigned char uchar;
typedef unsigned int uint;

uchar trig_h = 0;
uchar trig_l = 0;
uint cnt_h = 0;
uint cnt_l = 0;

int  freq;		//频率Hz
int  cycle;	//周期ms
int duty;		//占空比
int antiduty;

sbit key1 = P1^4;	//按键相关
sbit key2 = P1^5;
uchar trig_key;
uint cnt_key;
uchar flag_key;

char buff;
uchar ser_flag = 0;

void Timer0Init(void){		//30微秒@22.1184MHz
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xFf;		//设置定时器模式
	TMOD |= 0x01;		//设置定时器模式
	TL0 = TIMER_TL0;		//设置定时初值
	TH0 = TIMER_TH0;		//设置定时初值
	TF0 = 0;		//清除TF0标志
}
void UartInit(void)		//19200bps@22.1184MHz
{
	PCON &= 0x7F;		//波特率不倍速
	SCON = 0x50;		//8位数据,可变波特率
	AUXR &= 0xBF;		//定时器1时钟为Fosc/12,即12T
	AUXR &= 0xFE;		//串口1选择定时器1为波特率发生器
	TMOD &= 0x0F;		//清除定时器1模式位
	TMOD |= 0x20;		//设定定时器1为8位自动重装方式
	TL1 = 0xFD;		//设定定时初值
	TH1 = 0xFD;		//设定定时器重装值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
	EA = 1;		//打开总中断
	ES = 1;		//打开串口中断
}

void interrupt_init(void){
	EA = 1;		    //开启总中断
	ET0 = 1;		//定时器0中断允许，0为不允许
	TR0 = 1;		//定时器0开始计时
}
void io_init(void){
	P2 = 0Xff;
	trig_h = 1;
	trig_l = 0;
}
void other_init(void){
	freq  = 400;	//频率400Hz
	cycle = 2500 / 30;	//周期us ，（1/400）/30s = 83	，为方便计算，这里的周期不是严格定义上的周期
	duty = 30;	//最小为1，范围1-cycle，这里为1-83
	antiduty = cycle - duty;
	
	cnt_h = duty;
	cnt_l = antiduty;
	
	key1 = 1;
	key2 = 1;
	trig_key = 0;
	cnt_key = DELAYUS(60000);
	flag_key = 0;
}

void main(void)
{
	other_init();
	Timer0Init();
	UartInit();
	interrupt_init();
	io_init();
	key1 = 1;
	key2 = 1;
    while(1)
	{	
	/************************
	* 按键检测
	***************************/
		if((key1 == 0 || key2 == 0) && trig_key == 0){
			trig_key = 1;
		}
		if(key1 == 0 && flag_key == 1){
			trig_key = 0;
			flag_key = 0;
			duty += 1;
			if(duty >= cycle){
				duty = cycle;
			}
			ser_flag = 1;
		}
		else if(key2 == 0 && flag_key == 1){
			trig_key = 0;
			flag_key = 0;
			duty -= 1;
			if(duty <= 0){
				duty = 0;
			}
			ser_flag = 1;
		}
		
		/************************
		* PWM生成
		***************************/
		if(duty){
			if(trig_h == 1){
				P2 = 0xff;
				if(cnt_h == 0){
					trig_l = 1;
					trig_h = 0;
					cnt_h = duty;
				}
			} 
			
			if(trig_l == 1){
				P2 = 0x00;				 
				if(cnt_l == 0){
					trig_l = 0;
					trig_h = 1;
					cnt_l = cycle - duty;
				}
			}
		}
		else{
			P2 = 0x00;
		}
		/************************
		* 串口发送
		***************************/
		if(ser_flag == 1){	
			ser_flag = 0;
			ES  = 0;		    
			ET0 = 0;		
			TR0 = 0;				
			SBUF = duty;	
			while(!TI){}	//等待传输完成
			TI = 0;		
			ET0 = 1;		//定时器0中断允许，0为不允许
			TR0 = 1;		//定时器0开始计时
			ES  = 1;		//开启总中断	
		}
		
	}
}

void InterruptT0() interrupt 1
{
	TL0 = 0xb4;		
	TH0 = 0xff;	

	if(trig_key == 1){
		cnt_key--;
		if(cnt_key == 0){
			flag_key = 1;
			cnt_key = DELAYUS(60000);
		}
	}
	if(trig_h == 1){
		cnt_h--;
	}
	if(trig_l == 1){
		cnt_l--;
	}
}

void serial() interrupt 4  //串口中断函数
{	
	buff = SBUF;	//接收数据
	duty = buff > cycle ? cycle : buff;
	duty = buff <= 0 ? 0 : buff;
	RI = 0;
	ser_flag = 1;
}
