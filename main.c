#include "exynos_4412.h"

/****************peripheral********************
 * led2==>GPX2_7
 * led3==>GPX1_0
 * led4==>GPF3_4
 * led5==>GPF3_5
 * k2==>GPX1_1(中断ID：57)
 * k3==>GPX1_2(中断ID：58)
 * k4==>GPX3_2
 * beep==>GPD0_0
 * *******************************************/

/*********interrupt vector table********************/
void (*v[160])(void);

/***************function statement*****************/
void init_led5(void);
void init_key(void);
void init_timer0(void);
void init_WDT(void);
void init_RTC(void);

void mydelay_ms(int ms);

void do_irq(void);
void int_key2(void);
void int_key3(void);
void int_alam(void);

/****************************************************
 *function name：	int main()
 *arguments：		none
 *return value：	0：success
 *				   -1：failure
 * *************************************************/
int main()
{
	unsigned int flag,count=0;
	unsigned int v,vsum=0;

	ADCCON|=1<<16;			//选择分辨率为12-bits
	ADCCON=(ADCCON&(~(0xff<<6)))|(99<<6);	//设置分频率为(99+1)=>100MHz/100=1MHz
	ADCCON|=1<<14;			//开启预分频
	ADCMUX|=0b0011;			//选择通道3
	ADCCON&=(~(1<<2));			//开启正常操作模式
	ADCCON|=1;				//开启单次转换

	while (1)
	{
		flag=ADCCON;
		flag&=1<<15;
		if( 0 != flag )
		{
			v=ADCDAT&0xfff;
			v*=1800;
			v/=1<<12;
		//	printf(">ADC[%d]:%d mv ",count+1,v);
			ADCCON|=1<<1;
			vsum+=v;
			count++;
		}
		if( 3 ==count )
		{
			count=0;
			vsum/=3;
			printf("\n>ADC[sum]:%d mv\n",vsum);
			vsum=0;
		}
		mydelay_ms(500);
	};
	return 0;
}

/**********************function define**********************************/

/**********************************************************************
 * @brief		mydelay_ms program body
 * @param[in]	int (ms)
 * @return 		None
 **********************************************************************/
void mydelay_ms(int ms)
{
	int i, j;
	while (ms--)
	{
		for (i = 0; i < 5; i++)
			for (j = 0; j < 514; j++)
				;
	}
}
/***************************************************
 * function name：	void init_led5(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void init_led5()
{
	GPF3.CON=(GPF3.CON&(~(0xf<<20)))|(0x1<<20);				//配置GPF3_5为输出模式
//	GPF3.DAT|=0x1<<5;

	return ;
}

/***************************************************
 * function name：	void init_key(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void init_key()
{
	/*1、配置中断源*/

	GPX1.CON |= 0xf << 4; //配置中断源key2为中断模式
	GPX1.CON |= 0xf << 8; //配置中断源key3为中断模式
	EXT_INT41_CON = (EXT_INT41_CON & (~(0xf << 4))) | 0x2 << 4; //配置触发方式为下降沿触发
	EXT_INT41_CON = (EXT_INT41_CON & (~(0xf << 8))) | 0x2 << 8;
	EXT_INT41_FLTCON0 |= 0x1 << 15; //开启延时滤波
	EXT_INT41_FLTCON0 |= 0x1 << 23;
	EXT_INT41_MASK &= (~(0x1 << 1)); //使能中断源
	EXT_INT41_MASK &= (~(0x1 << 2));

	/*2、配置中断控制器GIC*/
	/*2.1、配置ICD*/
	ICDISER.ICDISER1 |= 0x1 << 25; //使能57号中断
	ICDISER.ICDISER1 |= 0x1 << 26; //使能58号中断
	ICDISPR.ICDISPR1 |= 0x1 << 25; //设置57号中断为等待
	ICDISPR.ICDISPR1 |= 0x1 << 26; //设置58号中断为等待

	//设置处理中断的目标CPU为CPU0
	ICDIPTR.ICDIPTR14 = (ICDIPTR.ICDIPTR14 & (~(0xff << 8)))
			| (0b00000001 << 8);
	ICDIPTR.ICDIPTR14 = (ICDIPTR.ICDIPTR14 & (~(0xff << 16)))
				| (0b00000001 << 16);

	v[57]=int_key2;				//配置中断向量表
	v[58]=int_key3;

	EXT_INT41_PEND |= 0x1 << 1; //清除外部中断pending状态
	EXT_INT41_PEND |= 0x1 << 2;
	ICDICPR.ICDICPR1 |= 0x1 << 25; //清除ICD的pending状态
	ICDICPR.ICDICPR1 |= 0x1 << 26;

	ICDDCR |= 0x1; //使能ICD
	/*2.2、配置ICC*/
	CPU0.ICCICR |= 0x1; //使能CPU0

	return ;
}
/***************************************************
 * function name：	void init_time0(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void init_timer0()
{
	/*timer*/
	/*1、配置timer模式*/
	GPD0.CON=(GPD0.CON&(~(0xf)))|0x2;				//配置GPIO0.0位timer模式（PCLK为100MHz）
	/*2、配置分频比*/
	PWM.TCFG0 = (PWM.TCFG0 & (~(0xff))) | 99; //预分频1==>100MHz/(99+1)=1MHz
	PWM.TCFG1 = (PWM.TCFG1 & (~(0xf))) | 0b0010; //预分频2==>1MHz/4=250KHz
	PWM.TCNTB0 = 250 - 1; //频率为:1KHz
	PWM.TCMPB0 = 124; //占比为:50%
	/*3、启动timer*/
	PWM.TCON = 0b01010; //配置为自动装载，手动装载
	PWM.TCON = 0b01001; //配置为自动装载，手动装载，启动定时器

	return ;
}
/***************************************************
 * function name：	void init_WDT(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void init_WDT()
{
	/*watch dog timer*/
	/*1、配置watch dog timer的分频比*/
	/*2、启动watch dog timer*/
	WDT.WTCNT = 12000; //初始化WDT的计数值（WTCNT）
	WDT.WTCON |= (199 << 8); //配置预分频1==>100MHz/(199+1)=500KHz
	//	WDT.WTCON=(WDT.WTCON&(~(0xff)))|0b111001;	//配置分频2==>500KHz/128=3906Hz，开启WDT，关闭中断，开启复位功能
	WDT.WTCON = (WDT.WTCON & (~(0xff))) | 0b111000; //配置分频2==>500KHz/128=3906Hz，开启WDT，开启中断，关闭复位功能

	return ;
}
/***************************************************
 * function name：	void init_RTC(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void init_RTC()
{
	/*Real Time Clock*/
	RTCCON |= 1; 							//开启RTC写使能
	RTC.BCDYEAR = 0x016;
	RTC.BCDMON = 0x04;
	RTC.BCDDAY = 0x19;
	RTC.BCDWEEK = 0x2;
	RTC.BCDHOUR = 0x17;
	RTC.BCDMIN = 0x32;
	RTC.BCDSEC = 0x30;
	RTCCON &= (~1);							//关闭RTC写使能

	/*RTC_ALM ID=76*/
	RTCALM.MIN=0x33;
	RTCALM.ALM=0b1<<1;
	RTCALM.ALM|=0b1<<6;

	ICDISER.ICDISER2|=0x1<<12;
	ICDIPTR.ICDIPTR19|=0b00000001;
	v[76]=int_alam;

	ICDDCR|=0x1;
	CPU0.ICCICR |= 0x1;

	return ;
}
/***************************************************
 * function name：	void do_irq(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void do_irq(void)
{
	int int_ID;

	int_ID = CPU0.ICCIAR & 0x3ff; //读取中断ID号
	printf(">[do_irq]interrupt id:%d\n", int_ID);

	(*v[int_ID])();

	CPU0.ICCEOIR |= (CPU0.ICCEOIR & (~(0x3ff))) | int_ID; //关闭中断

	return ;
}
/***************************************************
 * function name：	void int_key2(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void int_key2()
{
	printf(">[int_key2]\n");

	GPF3.DAT|=(0x1<<5);

	EXT_INT41_PEND |= 0x1 << 1; //清除外部中断pending状态
	ICDICPR.ICDICPR1 |= 0x1 << 25; //清除ICD的pending状态

	return ;
}
/***************************************************
 * function name：	void int_key3(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void int_key3()
{
	printf(">[int_key3]\n");

	GPF3.DAT&=(~(0x1<<5));
	GPD0.CON=(GPD0.CON&(~(0xf)))|0x4;

	EXT_INT41_PEND |= 0x1 << 2; //清除外部中断pending状态
	ICDICPR.ICDICPR1 |= 0x1 << 26; //清除ICD的pending状态

	return ;
}
/***************************************************
 * function name：	void int_alam(void)
 * arguments：		none
 * return value：	none
 **************************************************/
void int_alam()
{
	printf(">[int_alam]\n");

	GPF3.DAT|=(0x1<<5);
	init_timer0();

	RTCINTP|= 0x1 << 1; //清除RTC_alarm中断pending状态
	ICDICPR.ICDICPR2 |= 0x1 << 12; //清除ICD的pending状态

	return ;
}
