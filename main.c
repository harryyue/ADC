#include "exynos_4412.h"

/****************peripheral********************
 * led2==>GPX2_7
 * led3==>GPX1_0
 * led4==>GPF3_4
 * led5==>GPF3_5
 * k2==>GPX1_1(�ж�ID��57)
 * k3==>GPX1_2(�ж�ID��58)
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
 *function name��	int main()
 *arguments��		none
 *return value��	0��success
 *				   -1��failure
 * *************************************************/
int main()
{
	unsigned int flag,count=0;
	unsigned int v,vsum=0;

	ADCCON|=1<<16;			//ѡ��ֱ���Ϊ12-bits
	ADCCON=(ADCCON&(~(0xff<<6)))|(99<<6);	//���÷�Ƶ��Ϊ(99+1)=>100MHz/100=1MHz
	ADCCON|=1<<14;			//����Ԥ��Ƶ
	ADCMUX|=0b0011;			//ѡ��ͨ��3
	ADCCON&=(~(1<<2));			//������������ģʽ
	ADCCON|=1;				//��������ת��

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
 * function name��	void init_led5(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void init_led5()
{
	GPF3.CON=(GPF3.CON&(~(0xf<<20)))|(0x1<<20);				//����GPF3_5Ϊ���ģʽ
//	GPF3.DAT|=0x1<<5;

	return ;
}

/***************************************************
 * function name��	void init_key(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void init_key()
{
	/*1�������ж�Դ*/

	GPX1.CON |= 0xf << 4; //�����ж�Դkey2Ϊ�ж�ģʽ
	GPX1.CON |= 0xf << 8; //�����ж�Դkey3Ϊ�ж�ģʽ
	EXT_INT41_CON = (EXT_INT41_CON & (~(0xf << 4))) | 0x2 << 4; //���ô�����ʽΪ�½��ش���
	EXT_INT41_CON = (EXT_INT41_CON & (~(0xf << 8))) | 0x2 << 8;
	EXT_INT41_FLTCON0 |= 0x1 << 15; //������ʱ�˲�
	EXT_INT41_FLTCON0 |= 0x1 << 23;
	EXT_INT41_MASK &= (~(0x1 << 1)); //ʹ���ж�Դ
	EXT_INT41_MASK &= (~(0x1 << 2));

	/*2�������жϿ�����GIC*/
	/*2.1������ICD*/
	ICDISER.ICDISER1 |= 0x1 << 25; //ʹ��57���ж�
	ICDISER.ICDISER1 |= 0x1 << 26; //ʹ��58���ж�
	ICDISPR.ICDISPR1 |= 0x1 << 25; //����57���ж�Ϊ�ȴ�
	ICDISPR.ICDISPR1 |= 0x1 << 26; //����58���ж�Ϊ�ȴ�

	//���ô����жϵ�Ŀ��CPUΪCPU0
	ICDIPTR.ICDIPTR14 = (ICDIPTR.ICDIPTR14 & (~(0xff << 8)))
			| (0b00000001 << 8);
	ICDIPTR.ICDIPTR14 = (ICDIPTR.ICDIPTR14 & (~(0xff << 16)))
				| (0b00000001 << 16);

	v[57]=int_key2;				//�����ж�������
	v[58]=int_key3;

	EXT_INT41_PEND |= 0x1 << 1; //����ⲿ�ж�pending״̬
	EXT_INT41_PEND |= 0x1 << 2;
	ICDICPR.ICDICPR1 |= 0x1 << 25; //���ICD��pending״̬
	ICDICPR.ICDICPR1 |= 0x1 << 26;

	ICDDCR |= 0x1; //ʹ��ICD
	/*2.2������ICC*/
	CPU0.ICCICR |= 0x1; //ʹ��CPU0

	return ;
}
/***************************************************
 * function name��	void init_time0(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void init_timer0()
{
	/*timer*/
	/*1������timerģʽ*/
	GPD0.CON=(GPD0.CON&(~(0xf)))|0x2;				//����GPIO0.0λtimerģʽ��PCLKΪ100MHz��
	/*2�����÷�Ƶ��*/
	PWM.TCFG0 = (PWM.TCFG0 & (~(0xff))) | 99; //Ԥ��Ƶ1==>100MHz/(99+1)=1MHz
	PWM.TCFG1 = (PWM.TCFG1 & (~(0xf))) | 0b0010; //Ԥ��Ƶ2==>1MHz/4=250KHz
	PWM.TCNTB0 = 250 - 1; //Ƶ��Ϊ:1KHz
	PWM.TCMPB0 = 124; //ռ��Ϊ:50%
	/*3������timer*/
	PWM.TCON = 0b01010; //����Ϊ�Զ�װ�أ��ֶ�װ��
	PWM.TCON = 0b01001; //����Ϊ�Զ�װ�أ��ֶ�װ�أ�������ʱ��

	return ;
}
/***************************************************
 * function name��	void init_WDT(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void init_WDT()
{
	/*watch dog timer*/
	/*1������watch dog timer�ķ�Ƶ��*/
	/*2������watch dog timer*/
	WDT.WTCNT = 12000; //��ʼ��WDT�ļ���ֵ��WTCNT��
	WDT.WTCON |= (199 << 8); //����Ԥ��Ƶ1==>100MHz/(199+1)=500KHz
	//	WDT.WTCON=(WDT.WTCON&(~(0xff)))|0b111001;	//���÷�Ƶ2==>500KHz/128=3906Hz������WDT���ر��жϣ�������λ����
	WDT.WTCON = (WDT.WTCON & (~(0xff))) | 0b111000; //���÷�Ƶ2==>500KHz/128=3906Hz������WDT�������жϣ��رո�λ����

	return ;
}
/***************************************************
 * function name��	void init_RTC(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void init_RTC()
{
	/*Real Time Clock*/
	RTCCON |= 1; 							//����RTCдʹ��
	RTC.BCDYEAR = 0x016;
	RTC.BCDMON = 0x04;
	RTC.BCDDAY = 0x19;
	RTC.BCDWEEK = 0x2;
	RTC.BCDHOUR = 0x17;
	RTC.BCDMIN = 0x32;
	RTC.BCDSEC = 0x30;
	RTCCON &= (~1);							//�ر�RTCдʹ��

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
 * function name��	void do_irq(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void do_irq(void)
{
	int int_ID;

	int_ID = CPU0.ICCIAR & 0x3ff; //��ȡ�ж�ID��
	printf(">[do_irq]interrupt id:%d\n", int_ID);

	(*v[int_ID])();

	CPU0.ICCEOIR |= (CPU0.ICCEOIR & (~(0x3ff))) | int_ID; //�ر��ж�

	return ;
}
/***************************************************
 * function name��	void int_key2(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void int_key2()
{
	printf(">[int_key2]\n");

	GPF3.DAT|=(0x1<<5);

	EXT_INT41_PEND |= 0x1 << 1; //����ⲿ�ж�pending״̬
	ICDICPR.ICDICPR1 |= 0x1 << 25; //���ICD��pending״̬

	return ;
}
/***************************************************
 * function name��	void int_key3(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void int_key3()
{
	printf(">[int_key3]\n");

	GPF3.DAT&=(~(0x1<<5));
	GPD0.CON=(GPD0.CON&(~(0xf)))|0x4;

	EXT_INT41_PEND |= 0x1 << 2; //����ⲿ�ж�pending״̬
	ICDICPR.ICDICPR1 |= 0x1 << 26; //���ICD��pending״̬

	return ;
}
/***************************************************
 * function name��	void int_alam(void)
 * arguments��		none
 * return value��	none
 **************************************************/
void int_alam()
{
	printf(">[int_alam]\n");

	GPF3.DAT|=(0x1<<5);
	init_timer0();

	RTCINTP|= 0x1 << 1; //���RTC_alarm�ж�pending״̬
	ICDICPR.ICDICPR2 |= 0x1 << 12; //���ICD��pending״̬

	return ;
}
