//addde by yutao@wind-mobi.com

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/slab.h>


static void  rf_temp_check_handler(unsigned long data);
int 	rf_temp_check_irq;
static DECLARE_TASKLET(rf_temp_check,rf_temp_check_handler,0);
struct device_node *rf_temp_check_irq_node;
int cur_rf_check_state  = 0;
int next_rf_check_state = 0;
unsigned int g_rf_temp_check_state = 0;
struct input_dev *rf_input;
#define GPIO_RF_STATUS_CHECK_PIN 110

//
int           rf_status_flag=0;
#define   OTG_CHG_SWITCH_CLASS_NAME   "rf_status"
#define   OTG_CHG_DEVICE_NODE_NAME   "rf_status"
#define   OTG_CHG_DEVICE_CLASS_NAME  "rf_status" 
#define   OTG_CHG_DEVICE_FILE_NAME   "rf_status"
static struct class *rf_status_class = NULL;
static struct device* temp = NULL;  
static int rf_status_major=0;
static int rf_status_minor=0;
static DEFINE_MUTEX(rf_status_mutex);
extern void msleep(unsigned int msecs);
struct task_struct *rf_status_thread=NULL;


struct rf_status_device
{
   
   int rf_status_val;
   struct semaphore sem;
   struct cdev dev;

};

struct rf_status_device* rf_status_dev =NULL;

static struct file_operations rf_status_fops={
	.owner=THIS_MODULE,
	
};

void  rf_temp_check_handler(unsigned long data)
{
	   
	cur_rf_check_state =  gpio_get_value(GPIO_RF_STATUS_CHECK_PIN); 
	next_rf_check_state = !cur_rf_check_state;
	
	printk("yutao : enter rf_temp_check_handler,cur_rf_check_state =%d\n",cur_rf_check_state);
	   if (next_rf_check_state) {
		 // BMT_status.bat_charging_state = CHR_ERROR;
		  //smbchg_charging_enable(chip_for_fg,false);
		  g_rf_temp_check_state = 1;		  
	   
		input_report_key(rf_input, KEY_F12, 1);
	    input_sync(rf_input);
		input_report_key(rf_input, KEY_F12, 0);
	    input_sync(rf_input);
	    irq_set_irq_type(rf_temp_check_irq,IRQF_TRIGGER_RISING);
		  //mt_battery_update_status();
	   	printk("yutao IRQF_TRIGGER_RISING\n");
	   } else {
		   
		  //BMT_status.bat_charging_state = CHR_PRE;
		  //smbchg_charging_enable(chip_for_fg,true);
		  g_rf_temp_check_state = 0;
	   	 
		  //mt_battery_update_status();
		input_report_key(rf_input, KEY_F11, 1);
	    input_sync(rf_input);
		input_report_key(rf_input, KEY_F11, 0);
	    input_sync(rf_input);
		irq_set_irq_type(rf_temp_check_irq,IRQF_TRIGGER_FALLING);
	   	printk("yutao IRQF_TRIGGER_FALLING\n");
	   }
	
	
}
static irqreturn_t rf_temp_check_eint_handler(int irq, void *desc)
{
	tasklet_schedule(&rf_temp_check);
	printk("yutao: enter rf_temp_check_eint_handler, start tasklet_schedule->rf_temp_check_handler\n");
	return IRQ_HANDLED;
}

int rf_temp_check_setup_eint(void)
{   
	if (rf_temp_check_irq_node)
	{ 
		rf_temp_check_irq= irq_of_parse_and_map(rf_temp_check_irq_node, 0);
		printk("yutao:rf_temp_check_irq = %d\n", rf_temp_check_irq);
		if (!rf_temp_check_irq)
			{
				printk("yutao:irq_of_parse_and_map fail!!\n");
				return -EINVAL;
			}
		// wangjun@wind-mobi.com 20180301 begin
		if(request_irq(rf_temp_check_irq, rf_temp_check_eint_handler, IRQF_TRIGGER_RISING, "rf_tem_det-eint", NULL))
		// wangjun@wind-mobi.com 20180301 end
			{
				printk("IRQ LINE NOT AVAILABLE!!\n");
				return -EINVAL;
			}
	
			enable_irq(rf_temp_check_irq);
	}
	else
	{
	printk("yutao:null irq node!!\n");
	return -EINVAL;
	}
	  return 0;
}


static int rf_temp_check_probe(struct platform_device *dev)
{
     int	ret = 0;
	rf_input = input_allocate_device();
	if (rf_input == NULL) {
		printk(" yutao failed to allocate input device\n" );
		
		goto error_dev;
	}
	__set_bit(EV_KEY, rf_input->evbit);
	input_set_capability(rf_input,EV_KEY ,KEY_F11 );
	input_set_capability(rf_input,EV_KEY ,KEY_F12 );
    __set_bit(KEY_F11, rf_input->evbit);
	__set_bit(KEY_F12, rf_input->evbit);
	rf_input->name = "rf-keys";
	ret = input_register_device(rf_input);
	if (ret) {
		printk(" yutao failed to register input device\n");
		goto error_dev;
	}
	 
	 
   	 rf_temp_check_irq_node = of_find_compatible_node(NULL, NULL, "qcom, rf_tem_det-eint");
	 rf_temp_check_setup_eint();
	printk("yutao:enter rf_temp_check_probe \n");
	
	error_dev:
	if (rf_input!= NULL)
		input_free_device(rf_input);
	return 0;
}

static int rf_temp_check_remove(struct platform_device *dev)
{
	 
	return 0;
}



struct platform_device rf_temp_check_device = {
	.name = "rf_temp_check",
	.id = -1,
};


static struct platform_driver rf_temp_check_driver = {
	.probe = rf_temp_check_probe,
	.remove =rf_temp_check_remove,
	.shutdown = NULL,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = "rf_temp_check",
		   },
};


static int  rf_temp_check_init(void)
{
    int ret1,ret2;

	
	ret1 = platform_device_register(&rf_temp_check_device);
	if(ret1){
		printk("Unable to rf_temp_check_device register(%d)\n",ret1);
		return ret1;
	}
	
	ret2 = platform_driver_register(&rf_temp_check_driver);
	if(ret2){
		printk("Unable to rf_temp_check_driver register(%d)\n",ret2);
		return ret2;
	}
	

	
	return 0;
}

static void __exit rf_temp_check_exit(void)
{
	platform_driver_unregister(&rf_temp_check_driver);
}



//
static ssize_t _rf_status_show(struct rf_status_device*dev,char*buf)
{
    int val=0;
	if(down_interruptible(&(dev->sem)))
		{
			return -ERESTARTSYS;
		}
	val=dev->rf_status_val;


	up(&(dev->sem));

	return snprintf(buf,PAGE_SIZE,"%d\n",val);  //from kernel space to user space

}

static ssize_t _rf_status_store(struct rf_status_device *dev,const char * buf,size_t count)
{
	      int val=0;
	      val=simple_strtoul(buf, NULL, 10);
		if(down_interruptible(&(dev->sem)))
		 {
			return -ERESTARTSYS;
		 }				   
	      dev->rf_status_val=val;
		  if(val==1)
		  {
		     rf_status_flag=1;
		    	     
		   }
		  else 
		  {
			 rf_status_flag=0;
		
		  }
		 
		
	      up(&(dev->sem));
		
              return count;

}

static ssize_t rf_status_show(struct device *dev, struct device_attribute *attr,char *buf)
{


	struct rf_status_device *gdev=(struct rf_status_device*)dev_get_drvdata(dev);
	return _rf_status_show(gdev,buf);
}

static ssize_t rf_status_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{

	struct rf_status_device *gdev=(struct rf_status_device*)dev_get_drvdata(dev);
	
	 return _rf_status_store(gdev,buf,size);
	
}

 static int _rf_status_setup_dev(struct rf_status_device* dev)
 	{
 	
 	    int rf_gpio_st=0;
 		int err;
		dev_t devno=MKDEV(rf_status_major,rf_status_minor);
		memset(dev,0,sizeof(struct rf_status_device));
		cdev_init(&(dev->dev),&rf_status_fops);
		dev->dev.owner=THIS_MODULE;
		dev->dev.ops=&rf_status_fops;
		err=cdev_add(&(dev->dev),devno,1);
		if(err)
			{
				return err;

		     }
	//add here to init gpio status 
	rf_gpio_st=gpio_get_value(GPIO_RF_STATUS_CHECK_PIN); 
	if(rf_gpio_st)
	{
		dev->rf_status_val=1;
	}
	else
	{
		dev->rf_status_val=0;
	}	
		sema_init(&(dev->sem),1);
		return 0;
 	 }
 static DEVICE_ATTR(rf_status_val, 0664 , rf_status_show, rf_status_store);
static int __init rf_status_driver_init(void)
{
    int err = -1;  
    dev_t dev = 0;  
   

   //printk(KERN_ALERT"Initializing OTG_CHG device.\n");
   
   err = alloc_chrdev_region(&dev, 0, 1, OTG_CHG_DEVICE_NODE_NAME); 
   
       if(err < 0) {  
       printk(KERN_ALERT"Failed to alloc char dev region.\n");  
        goto fail;  
		}
		
	rf_status_major=MAJOR(dev);
	rf_status_minor=MINOR(dev);
	
	rf_status_dev=kmalloc(sizeof(struct rf_status_device),GFP_KERNEL);
	    if(!rf_status_dev) {  
       err = -ENOMEM;  
      // printk(KERN_ALERT"Failed to alloc usb_switch_dev.\n");  
        goto unregister;  
    }

	err=_rf_status_setup_dev(rf_status_dev);
	   if(err) {  
      // printk(KERN_ALERT"Failed to setup dev: %d.\n", err);  
        goto cleanup;  
		} 
		
	rf_status_class = class_create(THIS_MODULE, OTG_CHG_DEVICE_CLASS_NAME);  
       if(IS_ERR(rf_status_class)) {  
        err = PTR_ERR(rf_status_class);  
      // printk(KERN_ALERT"Failed to create OTG_CHG class.\n");  
       goto destroy_cdev;  
    } 

	   
	    temp = device_create(rf_status_class, NULL, dev, "%s", OTG_CHG_DEVICE_FILE_NAME);  
		
  if(IS_ERR(temp)) {  
       err = PTR_ERR(temp);  
       //printk(KERN_ALERT"Failed to create OTG_CHG device.");  
        goto destroy_class;  
   }

	    err = device_create_file(temp, &dev_attr_rf_status_val);  
    if(err < 0) {  
      // printk(KERN_ALERT"Failed to create attribute val.");                  
       goto destroy_device;  
    }  
  
    dev_set_drvdata(temp, rf_status_dev);  
	
  //  printk(KERN_ALERT"Initializing OTG_CHG device  successfully.\n");
	
    return 0;
	
destroy_device:
     device_destroy(rf_status_class,dev);	
destroy_class:
	class_destroy(rf_status_class);
	

destroy_cdev:
	cdev_del(&(rf_status_dev->dev));

cleanup:
	
	kfree(rf_status_dev);
unregister:
     unregister_chrdev_region(MKDEV(rf_status_major, rf_status_minor), 1);    
		
fail:

     return err;


   
   
   

}

static void __exit rf_status_driver_exit(void)
{
	dev_t devno = MKDEV(rf_status_major, rf_status_minor);  
	//printk(KERN_ALERT"Destroy OTG_CHG device.\n");
	
	  if(rf_status_class) {  
        device_destroy(rf_status_class, MKDEV(rf_status_major, rf_status_minor));  
        class_destroy(rf_status_class);  
    } 

    if(rf_status_dev) {  
        cdev_del(&(rf_status_dev->dev));  
        kfree(rf_status_dev);  
   }
unregister_chrdev_region(devno, 1); 
}


	

module_init(rf_status_driver_init);
module_exit(rf_status_driver_exit);

late_initcall(rf_temp_check_init);
module_exit(rf_temp_check_exit);
MODULE_AUTHOR("wind");
MODULE_DESCRIPTION("wind rf status check");
MODULE_LICENSE("GPL");

//add end
