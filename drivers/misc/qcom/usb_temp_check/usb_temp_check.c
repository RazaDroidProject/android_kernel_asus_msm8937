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
#include <linux/power_supply.h>
#include <linux/of_batterydata.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#define ASUS_CONTROL_USB_TEMP_SWITCH
static unsigned int asus_usb_thermal_switch = 1;

extern struct smbchg_chip *chip_for_fg; 
extern bool usb_present;
extern bool g_otg_status;
extern int usb_to_demoapp_flag;

extern int smbchg_usb_suspend(struct smbchg_chip *chip, bool suspend);  
extern int disable_otg_usbthermal(void);

struct delayed_work usb_thermal_polling_work;
static struct work_struct work_usb_thermal;
static struct switch_dev fps_switch_data;

struct pinctrl *usb_temp_pinctrl;
struct pinctrl_state *usb_temp_state;

int g_usb_temp_check_state = 0;
int power_off_charger = 0;

int usb_temp_check_irq = 0;
struct device_node *usb_temp_check_irq_node;
int cur_usb_check_state  = 0;
int next_usb_check_state = 0;
int open_usb_thermal = 0;
int high_temp_power_on = 0;

#ifdef CONFIG_WIND_PRO_A306
#define GPIO_USB_TEMP_CHECK_PIN 102
#else
#define GPIO_USB_TEMP_CHECK_PIN 124
#endif

#define GPIO_BOATDID1_PIN 90
#define GPIO_BOATDID2_PIN 59
#define GPIO_BOATDID3_PIN 93

#ifdef ASUS_CONTROL_USB_TEMP_SWITCH
#include <linux/proc_fs.h>
static int asus_usb_thermal_show(struct seq_file *m, void *v) {
    return seq_printf(m, "%u\n", asus_usb_thermal_switch);
}
static int asus_usb_thermal_read(struct inode *inode, struct file *file) {
    return single_open(file, asus_usb_thermal_show, NULL);
}
static ssize_t asus_usb_thermal_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos) {
    unsigned long temp = 0;
    int err = kstrtoul_from_user(buffer, count, 0, &temp);
    if (err) {
        printk("[wind_usb_thermal] %s: no know from user: %s\n", __func__, buffer);
        return err;
    }
    asus_usb_thermal_switch = (unsigned int)temp;

    // updata work_usb_thermal
    schedule_work(&work_usb_thermal);

    return count;
}
static const struct file_operations asus_usb_thermal_proc_ops = {
    .open       = asus_usb_thermal_read,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .write      = asus_usb_thermal_write,
    .release    = single_release,
};

static int asus_usb_thermal_proc_init(void) {
    struct proc_dir_entry *res = NULL;

    res = proc_create("asus_usb_thermal_switch", (S_IRUGO | S_IWUSR), NULL, &asus_usb_thermal_proc_ops);
    if (!res) {
        printk("[wind_usb_thermal] %s: proc file create failed !\n", __func__);
        return -ENOMEM;
    }
    return 0;
}
#endif

int read_three_times(int gpio,int times) {
    int time1,time2,time3;

    gpio_direction_input(gpio);
    msleep(times);
    time1 = gpio_get_value(gpio);

    //gpio_direction_input(gpio);
    msleep(times);
    time2 = gpio_get_value(gpio);

    if (time1 == time2)
        return time2;

    //gpio_direction_input(gpio);
    msleep(times);
    time3 = gpio_get_value(gpio);

    return time3;
}

int get_boardid_status(void) {
    int boardid1, boardid2, boardid3;

    //gpio_direction_input(GPIO_BOATDID1_PIN);
    //msleep(5);
    boardid1 = read_three_times(GPIO_BOATDID1_PIN, 10);

    //gpio_direction_input(GPIO_BOATDID2_PIN);
    //msleep(5);
    boardid2 = read_three_times(GPIO_BOATDID2_PIN, 10);

    //gpio_direction_input(GPIO_BOATDID3_PIN);
    //msleep(5);
    boardid3 = read_three_times(GPIO_BOATDID3_PIN, 10);

    printk("[wind_usb_thermal] wind get_boardid_status, %d, %d, %d\n", boardid1, boardid2, boardid3);
    return (boardid1 || boardid2 || boardid3);
}

static void usb_thermal_main(struct work_struct *work)
{
    if (open_usb_thermal == 0) {
        if (get_boardid_status() != 0)
            open_usb_thermal = 1;
        else
            printk("[wind_usb_thermal] msm8937 board there gpio value 0, close usb thermal function\n");
    }

#ifdef ASUS_CONTROL_USB_TEMP_SWITCH
    if (asus_usb_thermal_switch == 0) {
        printk("[wind_usb_thermal] asus_usb_thermal_switch: Force close usb thermal function\n");
        open_usb_thermal = 0;
    }
#endif

    cur_usb_check_state =  read_three_times(GPIO_USB_TEMP_CHECK_PIN, 10);
    next_usb_check_state = !cur_usb_check_state;
    printk("[wind_usb_thermal]: enter usb_temp_check_handler, cur_usb_check_state =%d\n", cur_usb_check_state);

    if (next_usb_check_state) {
        printk("[wind_usb_thermal] windlog IRQF_LOW\n");
        irq_set_irq_type(usb_temp_check_irq,IRQF_TRIGGER_HIGH| IRQF_ONESHOT);
        if (open_usb_thermal == 0) {
            g_usb_temp_check_state = 0;
            switch_set_state(&fps_switch_data, 0);
        } else {
            if(usb_present == 1 || g_otg_status == 0) {
                printk("[wind_usb_thermal] disable charge and otg function, usb_connector = 2\n");
                switch_set_state(&fps_switch_data, 2);
            } else {
                printk("[wind_usb_thermal] usb port temp high, usb_connector = 1\n");
                switch_set_state(&fps_switch_data, 1);
            }
            g_usb_temp_check_state = 1;
            usb_to_demoapp_flag = 0;
            smbchg_usb_suspend(chip_for_fg, true);
            disable_otg_usbthermal();
        }	
    } else {
        g_usb_temp_check_state = 2;
        switch_set_state(&fps_switch_data, 0);
        irq_set_irq_type(usb_temp_check_irq,IRQF_TRIGGER_LOW| IRQF_ONESHOT);
        printk("[wind_usb_thermal] windlog IRQF_TRIGGER_HIGH\n");
    }

    enable_irq(usb_temp_check_irq);	   
}


static irqreturn_t usb_temp_check_eint_handler(int irq, void *desc)
{
    disable_irq_nosync(usb_temp_check_irq);
    schedule_work(&work_usb_thermal); 
    return IRQ_HANDLED;
}

//int usb_temp_check_setup_eint(void)
void usb_temp_check_setup_eint(struct work_struct *work)
{
    if(open_usb_thermal) {
        struct power_supply *psy;
        int rc = 0, batt_id_kohm = 0;
        union power_supply_propval ret = {0, };
        psy = power_supply_get_by_name("bms");
        if (psy) {
            rc = psy->get_property(psy, POWER_SUPPLY_PROP_RESISTANCE_ID, &ret);
            if (rc == 0) {
                batt_id_kohm = ret.intval / 1000;
                if (batt_id_kohm < 45 || batt_id_kohm > 55) {
                    printk("[wind_usb_thermal] Non-standard battery[%d]: write '/proc/asus_usb_thermal_switch' 0\n", batt_id_kohm);
                    asus_usb_thermal_switch = 0;
                }
            }
        }
    }

    if (usb_temp_check_irq_node) {
        int usb_temp_pin_state = 0;

        /* configure usb temp irq gpio */
        gpio_request(GPIO_USB_TEMP_CHECK_PIN, "usb_temp_check_pin");
        gpio_direction_input(GPIO_USB_TEMP_CHECK_PIN);

        //usb_temp_check_irq= irq_of_parse_and_map(usb_temp_check_irq_node, 0);
        usb_temp_check_irq = gpio_to_irq(GPIO_USB_TEMP_CHECK_PIN);
        if (!usb_temp_check_irq) {
            printk("[wind_usb_thermal]: irq_of_parse_and_map fail !\n");
            //return -EINVAL;
        }

        usb_temp_pin_state = gpio_get_value(GPIO_USB_TEMP_CHECK_PIN);
        printk("[wind_usb_thermal]: usb_temp_check_pin=%d, irq=%d, value=%d\n", GPIO_USB_TEMP_CHECK_PIN, usb_temp_check_irq, usb_temp_pin_state);

        if (0) {
            if (request_irq(usb_temp_check_irq, usb_temp_check_eint_handler, IRQF_TRIGGER_RISING, "usb_tem_det-eint", NULL)) 
            {
                printk("IRQ LINE NOT AVAILABLE!!\n");
                //return -EINVAL;
            }
            high_temp_power_on = 1;
            printk("wind_logGPIO_USB_TEMP_CHECK_PIN");
        } else {
            if (request_irq(usb_temp_check_irq, usb_temp_check_eint_handler, IRQF_TRIGGER_LOW | IRQF_ONESHOT, "usb_tem_det-eint", NULL)) 
            {
                printk("IRQ LINE NOT AVAILABLE!!\n");
                //return -EINVAL;
            }
        }
    } else {
        printk("[wind_usb_thermal]: null irq node!!\n");
        //return -EINVAL;
    }
}


static int usb_temp_check_probe(struct platform_device *dev)
{
    usb_temp_check_irq_node = of_find_compatible_node(NULL, NULL, "qcom, usb_tem_det-eint");

    usb_temp_pinctrl = devm_pinctrl_get(&dev->dev);

    if (IS_ERR(usb_temp_pinctrl)) {
        printk("Cannot find  usb_temp pinctrl!");
    } else
        usb_temp_state = pinctrl_lookup_state(usb_temp_pinctrl, "default");

    if (IS_ERR_OR_NULL(usb_temp_state)) {
        printk("windlog:usb_temp_state NULL \n");
    } else
        pinctrl_select_state(usb_temp_pinctrl, usb_temp_state);

    INIT_WORK(&work_usb_thermal, usb_thermal_main); 
    //usb_temp_check_setup_eint();

#ifdef ASUS_CONTROL_USB_TEMP_SWITCH
    asus_usb_thermal_proc_init();
#endif

    INIT_DELAYED_WORK(&usb_thermal_polling_work, usb_temp_check_setup_eint);
    schedule_delayed_work(&usb_thermal_polling_work, msecs_to_jiffies(10000));

    printk("[wind_usb_thermal]: usb_temp_check_probe success.\n");
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

    if (!strcmp(str,"charger")) {
        power_off_charger = 1;
    } else {
        power_off_charger = 0; 
    }
    return 0;
}
__setup("androidboot.mode=", set_androidboot_mode);

static int __init set_usb_thermal(char *str)
{
    if (!strcmp(str,"msm8917")) {
        open_usb_thermal = 1;
    } else {
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
        printk("[wind_usb_thermal] Unable to usb_temp_check_driver register(%d)\n",ret2);
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
