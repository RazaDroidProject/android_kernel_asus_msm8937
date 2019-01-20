//addde by  liulinsheng@wind-mobi 20171018  for usb_temp_check begin
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/switch.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>


static struct work_struct work_usb_thermal;
static struct switch_dev fps_switch_data;
extern struct smbchg_chip *chip_for_fg; 
extern int  smbchg_charging_enable(struct smbchg_chip *chip, bool en);
extern int smbchg_usb_suspend(struct smbchg_chip *chip, bool suspend);  
extern int disable_otg_usbthermal(void);

extern int usb_to_demoapp_flag;
int 	usb_temp_check_irq;


struct pinctrl *usb_temp_pinctrl;

struct pinctrl_state *usb_temp_state;

struct device_node *usb_temp_check_irq_node;
int cur_usb_check_state  = 0;
int next_usb_check_state = 0;
int open_usb_thermal = 0;
int high_temp_power_on = 0;
int g_usb_temp_check_state = 0;
int power_off_charger = 0;
#define GPIO_USB_TEMP_CHECK_PIN 124
#define GPIO_BOATDID1_PIN 90
#define GPIO_BOATDID2_PIN 59
#define GPIO_BOATDID3_PIN 93




int read_three_times(int gpio,int times)
{
	int time1,time2,time3;
	
	gpio_direction_input(gpio);
	mdelay(times);
	time1 = gpio_get_value(gpio);
	
	gpio_direction_input(gpio);
	mdelay(times);
	time2 = gpio_get_value(gpio);
	
	gpio_direction_input(gpio);
	mdelay(times);
	time3 = gpio_get_value(gpio);
	
	return (time1 || time2 || time3);
}

int get_boardid_status(void)
{
	int boardid1,boardid2,boardid3;
	
	//gpio_direction_input(GPIO_BOATDID1_PIN);
	//msleep(5);
	boardid1 = read_three_times(GPIO_BOATDID1_PIN,5);
	
	//gpio_direction_input(GPIO_BOATDID2_PIN);
	//msleep(5);
	boardid2 = read_three_times(GPIO_BOATDID2_PIN,5);
	
	//gpio_direction_input(GPIO_BOATDID3_PIN);
	//msleep(5);
	boardid3 = read_three_times(GPIO_BOATDID3_PIN,5);
	
	printk("wind get_boardid_status,%d,%d,%d\n",boardid1,boardid2,boardid3);
	return (boardid1 || boardid2 || boardid3);
}




extern bool usb_present;
static void usb_thermal_main(struct work_struct *work)
{
	cur_usb_check_state =  read_three_times(GPIO_USB_TEMP_CHECK_PIN,5);
	next_usb_check_state = !cur_usb_check_state;
	printk("[wind_usb_thermal]: enter usb_temp_check_handler,cur_usb_check_state =%d\n",cur_usb_check_state);
	   if (next_usb_check_state) {
			printk("[wind_usb_thermal]windlog IRQF_LOW\n");
			irq_set_irq_type(usb_temp_check_irq,IRQF_TRIGGER_HIGH| IRQF_ONESHOT);
			if((get_boardid_status() == 0) && open_usb_thermal == 0){
			printk("[wind_usb_thermal]37 board there gpio value 0 close usb thermal"); 	
			}
			else{
				printk("[wind_usb_thermal]wind adapter_id_main");
				g_usb_temp_check_state = 1;
				usb_to_demoapp_flag = 0;
				smbchg_usb_suspend(chip_for_fg, true);
				disable_otg_usbthermal();
				if(usb_present == 1)
					switch_set_state(&fps_switch_data, 2); 
				else
					switch_set_state(&fps_switch_data, 1);
			}	
	   }else{
			g_usb_temp_check_state = 2;
			switch_set_state(&fps_switch_data, 0);
			irq_set_irq_type(usb_temp_check_irq,IRQF_TRIGGER_LOW| IRQF_ONESHOT);
			printk("[wind_usb_thermal]windlog IRQF_TRIGGER_HIGH|\n");
	   }
	   
	enable_irq(usb_temp_check_irq);	   
}


static irqreturn_t usb_temp_check_eint_handler(int irq, void *desc)
{
	disable_irq_nosync(usb_temp_check_irq);
	schedule_work(&work_usb_thermal); 
	return IRQ_HANDLED;
}

int usb_temp_check_setup_eint(void)
{
	if (usb_temp_check_irq_node)
	{ 
		usb_temp_check_irq= irq_of_parse_and_map(usb_temp_check_irq_node, 0);
		printk("windlog:usb_temp_check_irq = %d\n", usb_temp_check_irq);
		if (!usb_temp_check_irq)
		{
			printk("windlog:irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if(0){
		    if (request_irq(usb_temp_check_irq, usb_temp_check_eint_handler, IRQF_TRIGGER_RISING, "usb_tem_det-eint", NULL)) 
			{
				printk("IRQ LINE NOT AVAILABLE!!\n");
				return -EINVAL;
			}
			high_temp_power_on = 1;
			printk("wind_logGPIO_USB_TEMP_CHECK_PIN");
		}
		else {
			if (request_irq(usb_temp_check_irq, usb_temp_check_eint_handler, IRQF_TRIGGER_LOW | IRQF_ONESHOT, "usb_tem_det-eint", NULL)) 
			{
				printk("IRQ LINE NOT AVAILABLE!!\n");
				return -EINVAL;
			}
		}
	}else
	{
	printk("windlog:null irq node!!\n");
	return -EINVAL;
	}
	  return 0;
}


static int usb_temp_check_probe(struct platform_device *dev)
{

   	 usb_temp_check_irq_node = of_find_compatible_node(NULL, NULL, "qcom, usb_tem_det-eint");
	 
	
	
	usb_temp_pinctrl = devm_pinctrl_get(&dev->dev);

		if (IS_ERR(usb_temp_pinctrl))
		{	
		  printk("Cannot find  usb_temp pinctrl!");	
		}
		else
		usb_temp_state = pinctrl_lookup_state(usb_temp_pinctrl, "default");

	if(IS_ERR_OR_NULL(usb_temp_state)){
		printk("windlog:usb_temp_state NULL \n");
	}
	else
	pinctrl_select_state(usb_temp_pinctrl, usb_temp_state);
	
	printk("windlog:usb_temp_check_pin = %d\n",gpio_get_value(GPIO_USB_TEMP_CHECK_PIN));
	INIT_WORK(&work_usb_thermal, usb_thermal_main); 
	usb_temp_check_setup_eint();
	
	printk("windlog:enter usb_temp_check_probe \n");
	return 0;
}

static int usb_temp_check_remove(struct platform_device *dev)
{
	 
	return 0;
}




struct of_device_id usb_temp_of_match[] = {
	{ .compatible = "qcom, usb_tem_det-eint" },	
	{},
};

static struct platform_driver usb_temp_check_driver = {
	.probe = usb_temp_check_probe,
	.remove =usb_temp_check_remove,
	.shutdown = NULL,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = "usb_temp_check",
		   	.of_match_table = usb_temp_of_match,
		   },
};

static int __init set_androidboot_mode(char *str)
{
	
	if (!strcmp(str,"charger"))
    {
       power_off_charger = 1;
    }   		
    else
    {
       power_off_charger = 0; 
    }
	return 0;
}
__setup("androidboot.mode=", set_androidboot_mode);

static int __init set_usb_thermal(char *str)
{
	
	if (!strcmp(str,"msm8917"))
    {
       open_usb_thermal = 1;
    }   		
    else
    {
       open_usb_thermal = 0; 
    }
	return 0;
}
__setup("androidboot.platformtype=", set_usb_thermal);

static int  usb_temp_check_init(void)
{
    int ret2;
	
	
	ret2 = platform_driver_register(&usb_temp_check_driver);
	if(ret2){
		printk("Unable to usb_temp_check_driver register(%d)\n",ret2);
		return ret2;
	}
	fps_switch_data.name  = "usb_connector";
	fps_switch_data.index = 0;
	fps_switch_data.state = 0;  /* original 60 frames */
	switch_dev_register(&fps_switch_data);
	return 0;
}

static void __exit usb_temp_check_exit(void)
{
	platform_driver_unregister(&usb_temp_check_driver);
}


late_initcall(usb_temp_check_init);
module_exit(usb_temp_check_exit);
MODULE_AUTHOR("lvwenkang");
MODULE_DESCRIPTION("Usb_temp_check Driver");
MODULE_LICENSE("GPL");

//addde by  liulinsheng@wind-mobi 20171018  for usb_temp_check end
