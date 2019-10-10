/**
 * The device control driver for Sunwave's fingerprint sensor.
 *
 * Copyright (C) 2016 Sunwave Corporation. <http://www.sunwavecorp.com>
 * Copyright (C) 2016 Langson L. <mailto: liangzh@sunwavecorp.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
**/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/uaccess.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <linux/ioctl.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/fb.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/printk.h>

#include <linux/wakelock.h>
#include <linux/notifier.h> //zhangkaiyuan@wind-mobi.com 20180308

#include "sf_ctl.h"

//zhangkaiyuan@wind-mobi.com 20171213 begin
#ifdef CONFIG_WIND_DEVICE_INFO
#include <../../wind_device_info/wind_device_info.h>
#endif
//zhangkaiyuan@wind-mobi.com 20171213 end

#define MODULE_NAME "sf_ctl"
#define xprintk(level, fmt, args...) printk(level MODULE_NAME": "fmt, ##args)

#define SF_VDD_MIN_UV 2800000
#define SF_VDD_MAX_UV 2800000

#ifndef CONFIG_OF
# error "error: this driver 'MODULE_NAME' only support dts."
#endif

/**
 * Define the driver version string.
 * There is NO need to modify 'rXXXX_yyyymmdd', it should be updated automatically
 * by the building script (see the 'Driver-revision' section in 'build.sh').
 */
#define SF_DRV_VERSION "v0.10.1-20171012"
/* multi fingerprint vendor hal compatible switch */
#define MULTI_HAL_COMPATIBLE 1

//zhangkaiyuan@wind-mobi.com 20171213 begin
#ifdef CONFIG_WIND_DEVICE_INFO
extern wind_device_info_t wind_device_info;
#endif
//zhangkaiyuan@wind-mobi.com 20171213 end

#define ANDROID_WAKELOCK            1 //zhangkaiyuan@wind-mobi.com 20180309

struct sf_ctl_device {
    struct miscdevice miscdev;
    int pwr_num;
    int reset_num;
    int irq_num;
    struct regulator *vdd;
    u8 isPowerOn;
    struct work_struct work_queue;
    struct input_dev *input;
    struct notifier_block notifier; //zhangkaiyuan@wind-mobi.com 20180308
//zhangkaiyuan@wind-mobi.com 20180309 begin	
#if ANDROID_WAKELOCK
    struct wake_lock wakelock;
#endif
//zhangkaiyuan@wind-mobi.com 20180309 end
};

typedef enum {
    SF_PIN_STATE_PWR__ON,
    SF_PIN_STATE_PWR_OFF,
    SF_PIN_STATE_RST_SET,
    SF_PIN_STATE_RST_CLR,
    SF_PIN_STATE_INT_SET,

    /* Array size */
    SF_PIN_STATE_MAX
} sf_pin_state_t;

/*static const char *sf_pinctrl_state_names[SF_PIN_STATE_MAX] = {
    "power_on", "power_off", "reset_low", "reset_high", "eint_set",
};*/

//static int sf_ctl_device_power(bool on);
static int sf_ctl_device_reset(void);
static int sf_ctl_device_pwr(void);
static void sf_ctl_device_event(struct work_struct *ws);
static irqreturn_t sf_ctl_device_irq(int irq, void *dev_id);
static int sf_ctl_report_key_event(struct input_dev *input, sf_key_event_t *kevent);
static const char *sf_ctl_get_version(void);
static long sf_ctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int sf_ctl_open(struct inode *inode, struct file *filp);
static int sf_ctl_release(struct inode *inode, struct file *filp);
static int sf_ctl_init_gpio_pins(void);
static int sf_ctl_init_input(void);
static int __init sf_ctl_driver_init(void);
static void __exit sf_ctl_driver_exit(void);
extern int  sf_spi_clock_enable(bool);


//static struct pinctrl *sf_pinctrl = NULL;
//static struct pinctrl_state *sf_pin_states[SF_PIN_STATE_MAX] = {NULL, };

static struct file_operations sf_ctl_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = sf_ctl_ioctl,
    .open           = sf_ctl_open,
    .release        = sf_ctl_release,
};

static struct sf_ctl_device sf_ctl_dev = {
    .miscdev = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "sunwave_fp",
        .fops   = &sf_ctl_fops,
    }, 0,
};

static int sf_ctl_init_gpio(void)
{
    int err = 0;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);

    if (gpio_is_valid(sf_ctl_dev.pwr_num)) {
        err = gpio_request(sf_ctl_dev.pwr_num, "sf-pwr");

        if (err) {
            xprintk(KERN_ERR, "Could not request pwr gpio.\n");
            return err;
        }
    }
    else {
        xprintk(KERN_ERR, "not valid pwr gpio\n");
        return -EIO;
    }

    if (gpio_is_valid(sf_ctl_dev.reset_num)) {
        err = gpio_request(sf_ctl_dev.reset_num, "sf-reset");

        if (err) {
            xprintk(KERN_ERR, "Could not request reset gpio.\n");
            return err;
        }
    }
    else {
        xprintk(KERN_ERR, "not valid reset gpio\n");
        return -EIO;
    }

    if (gpio_is_valid(sf_ctl_dev.irq_num)) {
        err = pinctrl_request_gpio(sf_ctl_dev.irq_num);

        if (err) {
            xprintk(KERN_ERR, "Could not request irq gpio.\n");
            gpio_free(sf_ctl_dev.reset_num);
            return err;
        }
    }
    else {
        xprintk(KERN_ERR, "not valid irq gpio\n");
        gpio_free(sf_ctl_dev.reset_num);
        return -EIO;
    }

    pinctrl_gpio_direction_input(sf_ctl_dev.irq_num);
    xprintk(KERN_DEBUG, "%s(..) ok! exit.\n", __FUNCTION__);
    return err;
}


#if MULTI_HAL_COMPATIBLE
static int sf_ctl_free_gpio(void)
{
    int err = 0;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);

    if (gpio_is_valid(sf_ctl_dev.irq_num)) {
        pinctrl_free_gpio(sf_ctl_dev.irq_num);
        free_irq(gpio_to_irq(sf_ctl_dev.irq_num), (void *)&sf_ctl_dev);
    }

    if (gpio_is_valid(sf_ctl_dev.reset_num)) {
        gpio_free(sf_ctl_dev.reset_num);
    }

    if (gpio_is_valid(sf_ctl_dev.pwr_num)) {
        gpio_free(sf_ctl_dev.pwr_num);
    }

    xprintk(KERN_DEBUG, "%s(..) ok! exit.\n", __FUNCTION__);
    return err;
}
#endif

static int sf_ctl_init_irq(void)
{
    int err = 0;
    //struct device_node* dev_node = NULL;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    /* Register interrupt callback. */
    err = request_irq(gpio_to_irq(sf_ctl_dev.irq_num), sf_ctl_device_irq,
                      IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "sf-irq", (void *)&sf_ctl_dev);

    if (err) {
        xprintk(KERN_ERR, "request_irq(..) = %d.\n", err);
    }

    enable_irq_wake(gpio_to_irq(sf_ctl_dev.irq_num));
    return err;
}

#if 0
static int sf_ctl_device_power(bool on)
{
    int err = 0;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    /*if (on && (!sf_ctl_dev.isPowerOn)) {
        err = regulator_enable(sf_ctl_dev.vdd);

        if (err) {
            xprintk(KERN_ERR, "Regulator vdd enable failed err = %d\n", err);
            return err;
        }

        msleep(10);
        sf_ctl_dev.isPowerOn = 1;
    }
    else if (!on && (sf_ctl_dev.isPowerOn)) {
        err = regulator_disable(sf_ctl_dev.vdd);

        if (err) {
            xprintk(KERN_ERR, "Regulator vdd disable failed err = %d\n", err);
            return err;
        }

        sf_ctl_dev.isPowerOn = 0;
    }
    else {
        xprintk(KERN_ERR, "Ignore power status change from %d to %d\n",
                on, sf_ctl_dev.isPowerOn);
    }*/
    return err;
}
#endif

static int sf_ctl_device_pwr(void)
{
    int err = 0;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);

    if (sf_ctl_dev.pwr_num == 0) {
        xprintk(KERN_ERR, "sf_ctl_dev.reset_num is not get.\n");
        return -1;
    }

    gpio_direction_output(sf_ctl_dev.pwr_num, 1);
    msleep(1);
    gpio_set_value(sf_ctl_dev.pwr_num, 0);
    msleep(100);
    gpio_set_value(sf_ctl_dev.pwr_num, 1);
    return err;
}

static int sf_ctl_device_reset(void)
{
    int err = 0;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);

    if (sf_ctl_dev.reset_num == 0) {
        xprintk(KERN_ERR, "sf_ctl_dev.reset_num is not get.\n");
        return -1;
    }

    gpio_direction_output(sf_ctl_dev.reset_num, 1);
    msleep(1);
    gpio_set_value(sf_ctl_dev.reset_num, 0);
    msleep(10);
    gpio_set_value(sf_ctl_dev.reset_num, 1);
    return err;
}

static void sf_ctl_device_event(struct work_struct *ws)
{
    struct sf_ctl_device *sf_ctl_dev =
        container_of(ws, struct sf_ctl_device, work_queue);
    char *uevent_env[2] = { "SPI_STATE=finger", NULL };
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    kobject_uevent_env(&sf_ctl_dev->miscdev.this_device->kobj,
                       KOBJ_CHANGE, uevent_env);
}

static irqreturn_t sf_ctl_device_irq(int irq, void *dev_id)
{
    struct sf_ctl_device *sf_ctl_dev = (struct sf_ctl_device *)dev_id;
    disable_irq_nosync(irq);
    xprintk(KERN_ERR, "%s(irq = %d, ..) toggled.\n", __FUNCTION__, irq);
    schedule_work(&sf_ctl_dev->work_queue);
//zhangkaiyuan@wind-mobi.com 20180309 begin	
#if ANDROID_WAKELOCK
    wake_lock_timeout(&sf_ctl_dev->wakelock, msecs_to_jiffies(5000));
#endif
//zhangkaiyuan@wind-mobi.com 20180309 end
    enable_irq(irq);
    return IRQ_HANDLED;
}

static int sf_ctl_report_key_event(struct input_dev *input, sf_key_event_t *kevent)
{
    int err = 0;
    unsigned int key_code = KEY_UNKNOWN;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);

    switch (kevent->key) {
        case SF_KEY_HOME:
            key_code = KEY_HOMEPAGE;
            break;

        case SF_KEY_MENU:
            key_code = KEY_MENU;
            break;

        case SF_KEY_BACK:
            key_code = KEY_BACK;
            break;

        case SF_KEY_F19:
            key_code = KEY_FN_F5; //KEY_F19;single tap
			//printk("wind--sunwave input single tap\n");
            break;

        case SF_KEY_F20:
            key_code = KEY_FN_F6; //KEY_F20;double tap
			//printk("wind--sunwave input double tap\n");
            break;

        case SF_KEY_F21:
            key_code = KEY_FN_F7; //KEY_F21;long press
			//printk("wind--sunwave input long tap\n");
            break;

        case SF_KEY_ENTER:
            key_code = KEY_ENTER;
            break;

        case SF_KEY_UP:
            key_code = KEY_FN_F1; //KEY_UP;
            break;

        case SF_KEY_LEFT:
            key_code = KEY_FN_F3; //KEY_LEFT;
            break;

        case SF_KEY_RIGHT:
            key_code = KEY_FN_F4; //KEY_RIGHT;
            break;

        case SF_KEY_DOWN:
            key_code = KEY_FN_F2; //KEY_DOWN;
            break;

        case SF_KEY_WAKEUP:
            key_code = KEY_WAKEUP;
            break;
		//zhangkaiyuan@wind-mobi.com 20180208 begin
        case SF_KEY_EWAKEUP:
            key_code = KEY_FN_F8;
			printk("wind-sf: input key KEY_FN_F8, early wakeup\n");
            break;
		//zhangkaiyuan@wind-mobi.com 20180208 end	
        default:
            break;
    }

    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    input_report_key(input, key_code, kevent->value);
    input_sync(input);
    xprintk(KERN_DEBUG, "%s(..) leave.\n", __FUNCTION__);
    return err;
}

static const char *sf_ctl_get_version(void)
{
    static char version[SF_DRV_VERSION_LEN] = {'\0', };
    strncpy(version, SF_DRV_VERSION, SF_DRV_VERSION_LEN);
    version[SF_DRV_VERSION_LEN - 1] = '\0';
    return (const char *)version;
}

////////////////////////////////////////////////////////////////////////////////
// struct file_operations fields.

static long sf_ctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct miscdevice *dev = (struct miscdevice *)filp->private_data;
    struct sf_ctl_device *sf_ctl_dev =
        container_of(dev, struct sf_ctl_device, miscdev);
    int err = 0;
    sf_key_event_t kevent;
	//zhangkaiyuan@wind-mobi.com 20171213 begin
	#ifdef CONFIG_WIND_DEVICE_INFO
	sf_ic_info_t ic_info_data;
	#endif
	//zhangkaiyuan@wind-mobi.com 20171213 end
    xprintk(KERN_DEBUG, "%s(cmd = 0x%08x, ..)\n", __FUNCTION__, cmd);

    switch (cmd) {
        case SF_IOC_INIT_DRIVER: {
#if MULTI_HAL_COMPATIBLE
            xprintk(KERN_INFO, "SF_IOC_INIT_DRIVER.\n");
            sf_ctl_init_gpio();
#endif
            break;
        }

        case SF_IOC_DEINIT_DRIVER: {
#if MULTI_HAL_COMPATIBLE
            xprintk(KERN_INFO, "SF_IOC_DEINIT_DRIVER.\n");
            sf_ctl_free_gpio();
#endif
            break;
        }

        case SF_IOC_RESET_DEVICE: {
            sf_ctl_device_reset();
            break;
        }

        case SF_IOC_ENABLE_IRQ: {
            // TODO:
            break;
        }

        case SF_IOC_DISABLE_IRQ: {
            // TODO:
            break;
        }

        case SF_IOC_REQUEST_IRQ: {
#if MULTI_HAL_COMPATIBLE
            xprintk(KERN_INFO, "SF_IOC_REQUEST_IRQ.\n");
            sf_ctl_init_irq();
#endif
            break;
        }

        case SF_IOC_ENABLE_SPI_CLK: {
            // TODO:
            sf_spi_clock_enable(true);
            break;
        }

        case SF_IOC_DISABLE_SPI_CLK: {
            sf_spi_clock_enable(false);
            // TODO:
            break;
        }

        case SF_IOC_ENABLE_POWER: {
            // TODO:
            break;
        }

        case SF_IOC_DISABLE_POWER: {
            // TODO:
            break;
        }

        case SF_IOC_REPORT_KEY_EVENT: {
            if (copy_from_user(&kevent, (sf_key_event_t *)arg, sizeof(sf_key_event_t))) {
                xprintk(KERN_ERR, "copy_from_user(..) failed.\n");
                err = (-EFAULT);
                break;
            }

            err = sf_ctl_report_key_event(sf_ctl_dev->input, &kevent);
            break;
        }

        case SF_IOC_SYNC_CONFIG: {
            // TODO:
            break;
        }

        case SF_IOC_GET_VERSION: {
            if (copy_to_user((void *)arg, sf_ctl_get_version(), SF_DRV_VERSION_LEN)) {
                xprintk(KERN_ERR, "copy_to_user(..) failed.\n");
                err = (-EFAULT);
                break;
            }

            break;
        }
	/*zhangkaiyuan@wind-mobi.com 20171213 begin*/
	    #ifdef CONFIG_WIND_DEVICE_INFO
	    case SF_IOC_SET_IC_INFO: {
		if (copy_from_user(&ic_info_data, (sf_ic_info_t *)arg, sizeof(sf_ic_info_t))) {
            xprintk(KERN_ERR, "copy_from_user(..) failed.\n");
            err = (-EFAULT);
            break;
        }
		sprintf(wind_device_info.fp_module_info.ic_name, "%s", ic_info_data.ic_name);
		sprintf(wind_device_info.fp_module_info.fwvr, "%s", ic_info_data.fwvr);
		wind_device_info.fp_module_info.vendor = ic_info_data.vendor; 
		xprintk(KERN_INFO, "ic_name = %s, fp_module_info.vendor = %02x, fp_module_info.fwvr = %s\n", 
				wind_device_info.fp_module_info.ic_name, wind_device_info.fp_module_info.vendor, wind_device_info.fp_module_info.fwvr);
		break;
	    }
	    #endif
	/*zhangkaiyuan@wind-mobi.com 20171213 end.*/
        default:
            err = (-EINVAL);
            break;
    }

    return err;
}

static int sf_ctl_open(struct inode *inode, struct file *filp)
{
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    return 0;
}

static int sf_ctl_release(struct inode *inode, struct file *filp)
{
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////



// see sf_spi.c
//extern int  sf_spi_platform_init(void);
extern void sf_spi_platform_exit(void);

////////////////////////////////////////////////////////////////////////////////

static int sf_ctl_init_gpio_pins(void)
{
    int err = 0;
    //struct platform_device* pdev = NULL;
    struct device_node *dev_node = NULL;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    dev_node = of_find_compatible_node(NULL, NULL, "sunwave,fingerprint");

    if (!dev_node) {
        xprintk(KERN_ERR, "of_find_compatible_node(..) failed.\n");
        return (-ENODEV);
    }

    sf_ctl_dev.pwr_num = of_get_named_gpio(dev_node, "swfp,gpio_power", 0);
    sf_ctl_dev.reset_num = of_get_named_gpio(dev_node, "swfp,gpio_reset", 0);
    sf_ctl_dev.irq_num = of_get_named_gpio(dev_node, "swfp,gpio_irq", 0);
    xprintk(KERN_INFO, "reset_gpio_number = %d\n", sf_ctl_dev.reset_num);
    xprintk(KERN_INFO, "irq_gpio_number = %d\n", sf_ctl_dev.irq_num);
#if MULTI_HAL_COMPATIBLE
    xprintk(KERN_INFO, "=== Do not request gpio resources!!! ===\n");
#else
    sf_ctl_init_gpio();
#if 0

    if (gpio_is_valid(sf_ctl_dev.reset_num)) {
        err = gpio_request(sf_ctl_dev.reset_num, "sf-reset");

        if (err) {
            xprintk(KERN_ERR, "Could not request reset gpio.\n");
            return err;
        }
    }
    else {
        xprintk(KERN_ERR, "not valid reset gpio\n");
        return -EIO;
    }

    if (gpio_is_valid(sf_ctl_dev.irq_num)) {
        err = pinctrl_request_gpio(sf_ctl_dev.irq_num);

        if (err) {
            xprintk(KERN_ERR, "Could not request irq gpio.\n");
            gpio_free(sf_ctl_dev.reset_num);
            return err;
        }
    }
    else {
        xprintk(KERN_ERR, "not valid irq gpio\n");
        gpio_free(sf_ctl_dev.reset_num);
        return -EIO;
    }

    pinctrl_gpio_direction_input(sf_ctl_dev.irq_num);
#endif
#endif
    /*pdev = of_find_device_by_node(dev_node);

    if (!pdev) {
        xprintk(KERN_ERR, "of_find_device_by_node(..) failed.\n");
        return (-ENODEV);
    }

    sf_ctl_dev.vdd = regulator_get(&pdev->dev, "vdd");

    if (IS_ERR(sf_ctl_dev.vdd)) {
        err = PTR_ERR(sf_ctl_dev.vdd);
        xprintk(KERN_ERR, "Regulator get failed vdd err = %d\n", err);
        gpio_free(sf_ctl_dev.reset_num);
        pinctrl_free_gpio(sf_ctl_dev.irq_num);
        return err;
    }

    if (regulator_count_voltages(sf_ctl_dev.vdd) > 0) {
        err = regulator_set_voltage(sf_ctl_dev.vdd, SF_VDD_MIN_UV,
                                    SF_VDD_MAX_UV);

        if (err) {
            xprintk(KERN_ERR, "Regulator set_vtg failed vdd err = %d\n", err);
            gpio_free(sf_ctl_dev.reset_num);
            pinctrl_free_gpio(sf_ctl_dev.irq_num);
            regulator_put(sf_ctl_dev.vdd);
            return err;
        }
    }*/
    return err;
}

static int sf_ctl_init_input(void)
{
    int err = 0;
    xprintk(KERN_DEBUG, "%s(..) enter.\n", __FUNCTION__);
    sf_ctl_dev.input = input_allocate_device();

    if (!sf_ctl_dev.input) {
        xprintk(KERN_ERR, "input_allocate_device(..) failed.\n");
        return (-ENOMEM);
    }

    sf_ctl_dev.input->name = "sf-keys";
    __set_bit(EV_KEY  , sf_ctl_dev.input->evbit );
    __set_bit(KEY_HOMEPAGE, sf_ctl_dev.input->keybit);
    __set_bit(KEY_MENU, sf_ctl_dev.input->keybit);
    __set_bit(KEY_BACK, sf_ctl_dev.input->keybit);
    __set_bit(KEY_FN_F5, sf_ctl_dev.input->keybit);
    __set_bit(KEY_FN_F6, sf_ctl_dev.input->keybit);
    __set_bit(KEY_FN_F7, sf_ctl_dev.input->keybit);
    __set_bit(KEY_ENTER, sf_ctl_dev.input->evbit );
    __set_bit(KEY_FN_F1, sf_ctl_dev.input->keybit);
    __set_bit(KEY_FN_F3, sf_ctl_dev.input->keybit);
    __set_bit(KEY_FN_F4, sf_ctl_dev.input->keybit);
    __set_bit(KEY_FN_F2, sf_ctl_dev.input->keybit);
    __set_bit(KEY_WAKEUP, sf_ctl_dev.input->keybit);
	__set_bit(KEY_FN_F8, sf_ctl_dev.input->keybit); //zhangkaiyuan@wind-mobi.com 20180208
    err = input_register_device(sf_ctl_dev.input);

    if (err) {
        xprintk(KERN_ERR, "input_register_device(..) = %d.\n", err);
        input_free_device(sf_ctl_dev.input);
        sf_ctl_dev.input = NULL;
        return (-ENODEV);
    }

    xprintk(KERN_DEBUG, "%s(..) leave.\n", __FUNCTION__);
    return err;
}
//zhangkaiyuan@wind-mobi.com 20180308 begin
static int sf_fb_notifier_callback(struct notifier_block *self,
                                   unsigned long event, void *data)
{
    static char screen_status[64] = {'\0'};
    char *screen_env[2] = { screen_status, NULL };
    struct fb_event *evdata = data;
    unsigned int blank;
    int retval = 0;

    if (event != FB_EVENT_BLANK /* FB_EARLY_EVENT_BLANK */) {
        return 0;
    }

    blank = *(int *)evdata->data;
    //printk("wind-sf_fb_notifier_callback enter ...\n");
    switch (blank) {
        case FB_BLANK_UNBLANK:
            sprintf(screen_status, "SCREEN_STATUS=%s", "ON");
            kobject_uevent_env(&sf_ctl_dev.miscdev.this_device->kobj, KOBJ_CHANGE, screen_env);
			//printk("wind-sf_fb_notifier_callback FB_BLANK_UNBLANK ...\n");
            break;

        case FB_BLANK_POWERDOWN:
            sprintf(screen_status, "SCREEN_STATUS=%s", "OFF");
            kobject_uevent_env(&sf_ctl_dev.miscdev.this_device->kobj, KOBJ_CHANGE, screen_env);
			//printk("wind-sf_fb_notifier_callback FB_BLANK_POWERDOWN ...\n");
            break;

        default:
            break;
    }

    return retval;
}
//zhangkaiyuan@wind-mobi.com 20180308 end
static int __init sf_ctl_driver_init(void)
{
    int err = 0;

    /* Initialize the GPIO pins. */
    err = sf_ctl_init_gpio_pins();
	err = sf_ctl_device_pwr();
    if (err) {
        xprintk(KERN_ERR, "sf_ctl_init_gpio_pins failed with %d.\n", err);
        return err;
    }
//zhangkaiyuan@wind-mobi.com 20180309 begin
#if ANDROID_WAKELOCK
    wake_lock_init(&sf_ctl_dev.wakelock, WAKE_LOCK_SUSPEND, "sf_wakelock");
#endif
//zhangkaiyuan@wind-mobi.com 20180309 end

#if MULTI_HAL_COMPATIBLE
    xprintk(KERN_INFO, "=== Do not sf_ctl_init_irq!!! ===\n");
#else
    /* Initialize the interrupt callback. */
    err = sf_ctl_init_irq();

    if (err) {
        xprintk(KERN_ERR, "sf_ctl_init_irq failed with %d.\n", err);
        return err;
    }

#endif
    /* Initialize the input subsystem. */
    err = sf_ctl_init_input();

    if (err) {
        xprintk(KERN_ERR, "sf_ctl_init_input failed with %d.\n", err);
        //free_irq(sf_ctl_dev.irq_num, (void*)&sf_ctl_dev);
        return err;
    }


    /* Register as a miscellaneous device. */
    err = misc_register(&sf_ctl_dev.miscdev);

    if (err) {
        xprintk(KERN_ERR, "misc_register(..) = %d.\n", err);
        input_unregister_device(sf_ctl_dev.input);
        //free_irq(sf_ctl_dev.irq_num, (void*)&sf_ctl_dev);
        return err;
    }

    INIT_WORK(&sf_ctl_dev.work_queue, sf_ctl_device_event);
    //err = sf_spi_platform_init();
    //full_fp_chip_name("sunwave_fp");
//zhangkaiyuan@wind-mobi.com 20180308 begin    
    sf_ctl_dev.notifier.notifier_call = sf_fb_notifier_callback;
    fb_register_client(&sf_ctl_dev.notifier);
//zhangkaiyuan@wind-mobi.com 20180308 end   
    xprintk(KERN_INFO, "sunwave fingerprint device control driver registered.\n");
    xprintk(KERN_INFO, "driver version: '%s'.\n", sf_ctl_get_version());
    return err;
}

static void __exit sf_ctl_driver_exit(void)
{
    if (sf_ctl_dev.input) {
        input_unregister_device(sf_ctl_dev.input);
    }

    if (sf_ctl_dev.irq_num >= 0) {
        pinctrl_free_gpio(sf_ctl_dev.irq_num);
        free_irq(gpio_to_irq(sf_ctl_dev.irq_num), (void *)&sf_ctl_dev);
    }

    if (sf_ctl_dev.reset_num >= 0) {
        gpio_free(sf_ctl_dev.reset_num);
    }

//zhangkaiyuan@wind-mobi.com 20180309 begin	
#if ANDROID_WAKELOCK
    wake_lock_destroy(&sf_ctl_dev.wakelock);
#endif
//zhangkaiyuan@wind-mobi.com 20180309 end
    misc_deregister(&sf_ctl_dev.miscdev);
    sf_spi_platform_exit();
    xprintk(KERN_INFO, "sunwave fingerprint device control driver released.\n");
}

module_init(sf_ctl_driver_init);
module_exit(sf_ctl_driver_exit);

MODULE_DESCRIPTION("The device control driver for Sunwave's fingerprint sensor.");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Langson L. <liangzh@sunwavecorp.com>");

