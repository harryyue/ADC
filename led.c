#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/module.h>

#define MASTER  290
#define SLAVE   0
#define DEVICE_NUM   1

dev_t devno;
unsigned int *GPIOX;
unsigned int *GPIOF;


static int led_open ( struct inode * inode , struct file * file )
{

	printk(">led_open ... \n");

	unsigned int * gpio_X1_con,*gpio_X1_dat;

	gpio_X1_con=GPIOX+0x8;
	gpio_X1_dat=gpio_X1_con+1;

	*gpio_X1_con=(*gpio_X1_con&(~(0xf<<0)))|(1<<0);
	*gpio_X1_dat|=(1<<0);


	printk("GPIOX:%x,GPIOF:%x\n",GPIOX,GPIOF);
	return 0;
}

static ssize_t led_read ( struct file * file , char __user * user , size_t size , loff_t * o )
{
	printk(">led_read... \n");	
	unsigned int * gpio_f3_4_con,*gpio_f3_4_dat;

	gpio_f3_4_con=GPIOF+0x18;
	gpio_f3_4_dat=gpio_f3_4_con+1;

	*gpio_f3_4_con=(*gpio_f3_4_con&(~(0xf<<16)))|(1<<16);
	*gpio_f3_4_dat|=(1<<4);

	printk("GPIOX:%x,GPIOF:%x\n",GPIOX,GPIOF);
	return 0;
}

static ssize_t led_write ( struct file * file , char __user * user , size_t size , loff_t * o )
{
	printk(">led_write... \n");
	unsigned int * gpio_f3_5_con,*gpio_f3_5_dat;

	gpio_f3_5_con=GPIOF+0x18;
	gpio_f3_5_dat=gpio_f3_5_con+1;

	*gpio_f3_5_con=(*gpio_f3_5_con&(~(0xf<<20)))|(1<<20);
	*gpio_f3_5_dat|=(1<<5);

	printk("GPIOX:%x,GPIOF:%x\n",GPIOX,GPIOF);

	return 0;
}
struct file_operations fops={
	.open=led_open,
	.read=led_read,
	.write=led_write,
};
struct cdev cdev;

int led_init(void)
{
	//1、申请设备号
	//2、注册设备号
	//3、初始化换cdev结构体
	//4、添加字符设备到系统
	
	int register_ret,add_ret;
	unsigned int * gpio_X2_con,*gpio_X2_dat;

	printk(">led_init \n");
	devno = MKDEV(MASTER,SLAVE);
	register_ret = register_chrdev_region(devno,DEVICE_NUM,"hello");
	if ( 0 != register_ret )
	{
		printk(">[error] register device fail ...\n");
		return -EBUSY;
	}
 	cdev_init ( &cdev , &fops );
	add_ret=cdev_add ( &cdev , devno , DEVICE_NUM );
	if ( 0 != add_ret )
	{
		printk(">[error] add to system fail...\n");
		return -EBUSY;
	}

	GPIOX=ioremap(0x11000c00,0x6c);
	GPIOF=ioremap(0x11400180,0x74);

	gpio_X2_con=GPIOX+0x10;
	gpio_X2_dat=gpio_X2_con+1;

	*gpio_X2_con=(*gpio_X2_con&(~(0xf<<28)))|(1<<28);
	*gpio_X2_dat&=~(1<<7);

	printk("GPIOX:%x,GPIOF:%x\n",GPIOX,GPIOF);
	printk("gpio_X2_con:%x,gpio_X2_dat:%x\n",gpio_X2_con,gpio_X2_dat);
	return 0;
}

void led_exit(void)
{
	//1、释放设备号
	//2、将字符设备从系统中移除
	
	cdev_del ( &cdev );
	unregister_chrdev_region(devno,DEVICE_NUM);
	
	printk(">led_exit \n");


}

/*将内核加载函数与用户定义的加载函数关联*/
module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("harry");
MODULE_LICENSE("GPL");
MODULE_VERSION("led driver-v1.0.0");
