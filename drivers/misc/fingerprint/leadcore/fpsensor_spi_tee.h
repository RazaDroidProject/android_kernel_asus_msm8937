/* add by leadcore start  */
#ifndef __FPSENSOR_SPI_TEE_H
#define __FPSENSOR_SPI_TEE_H

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#define FP_NOTIFY         1
#if FP_NOTIFY
#include <linux/fb.h>
#include <linux/notifier.h>
#endif

#define FPSENSOR_DEV_NAME           "fpsensor"
#define FPSENSOR_CLASS_NAME         "fpsensor"
#define FPSENSOR_DEV_MAJOR          255
#define N_SPI_MINORS                32    /* ... up to 256 */
#define FPSENSOR_NR_DEVS            1
#define FPSENSOR_INPUT_NAME         "fpsensor_keys"
#define ERR_LOG     (0)
#define INFO_LOG    (0)
#define DEBUG_LOG   (0)
#define fpsensor_debug(level, fmt, args...) do { \
        if (fpsensor_debug_level >= level) {\
            printk( "[fpsensor] " fmt, ##args); \
        } \
    } while (0)
#define FUNC_ENTRY()  fpsensor_debug(DEBUG_LOG, "%s, %d, entry\n", __func__, __LINE__)
#define FUNC_EXIT()   fpsensor_debug(DEBUG_LOG, "%s, %d, exit\n", __func__, __LINE__)

/**********************IO Magic**********************/
#define FPSENSOR_IOC_MAGIC    0xf0    //CHIP

typedef enum fpsensor_key_event {
    FPSENSOR_KEY_NONE = 0,
    FPSENSOR_KEY_HOME,
    FPSENSOR_KEY_POWER,
    FPSENSOR_KEY_MENU,
    FPSENSOR_KEY_BACK,
    FPSENSOR_KEY_CAPTURE,
    FPSENSOR_KEY_UP,
    FPSENSOR_KEY_DOWN,
    FPSENSOR_KEY_RIGHT,
    FPSENSOR_KEY_LEFT,
    FPSENSOR_KEY_TAP,
    FPSENSOR_KEY_HEAVY
} fpsensor_key_event_t;

struct fpsensor_key {
    enum fpsensor_key_event key;
    uint32_t value;   /* key down = 1, key up = 0 */
};

//zhangkaiyuan@wind-mobi.com 20171213 begin
#ifdef CONFIG_WIND_DEVICE_INFO
typedef struct {
    char ic_name[30];
    unsigned int vendor; 
	char fwvr[8]; 
} lc_ic_info_t;
#endif
//zhangkaiyuan@wind-mobi.com 20171213 end

/* define commands */
#define FPSENSOR_IOC_INIT                       _IOWR(FPSENSOR_IOC_MAGIC,0,unsigned int)
#define FPSENSOR_IOC_EXIT                       _IOWR(FPSENSOR_IOC_MAGIC,1,unsigned int)
#define FPSENSOR_IOC_RESET                      _IOWR(FPSENSOR_IOC_MAGIC,2,unsigned int)
#define FPSENSOR_IOC_ENABLE_IRQ                 _IOWR(FPSENSOR_IOC_MAGIC,3,unsigned int)
#define FPSENSOR_IOC_DISABLE_IRQ                _IOWR(FPSENSOR_IOC_MAGIC,4,unsigned int)
#define FPSENSOR_IOC_GET_INT_VAL                _IOWR(FPSENSOR_IOC_MAGIC,5,unsigned int)
#define FPSENSOR_IOC_DISABLE_SPI_CLK            _IOWR(FPSENSOR_IOC_MAGIC,6,unsigned int)
#define FPSENSOR_IOC_ENABLE_SPI_CLK             _IOWR(FPSENSOR_IOC_MAGIC,7,unsigned int)
#define FPSENSOR_IOC_ENABLE_POWER               _IOWR(FPSENSOR_IOC_MAGIC,8,unsigned int)
#define FPSENSOR_IOC_DISABLE_POWER              _IOWR(FPSENSOR_IOC_MAGIC,9,unsigned int)
#define FPSENSOR_IOC_INPUT_KEY_EVENT            _IOWR(FPSENSOR_IOC_MAGIC,10,struct fpsensor_key)
/* fp sensor has change to sleep mode while screen off */
#define FPSENSOR_IOC_ENTER_SLEEP_MODE           _IOWR(FPSENSOR_IOC_MAGIC,11,unsigned int)
#define FPSENSOR_IOC_REMOVE                     _IOWR(FPSENSOR_IOC_MAGIC,12,unsigned int)
#define FPSENSOR_IOC_CANCEL_WAIT                _IOWR(FPSENSOR_IOC_MAGIC,13,unsigned int)
#define FPSENSOR_IOC_GET_FP_STATUS              _IOWR(FPSENSOR_IOC_MAGIC,19,unsigned int*)

//zhangkaiyuan@wind-mobi.com 20171213 begin
#ifdef CONFIG_WIND_DEVICE_INFO
#define FPSENSOR_IOC_GET_IC_INFO      _IOWR(FPSENSOR_IOC_MAGIC, 0x30, lc_ic_info_t *)
#endif
//zhangkaiyuan@wind-mobi.com 20171213 end

#define FPSENSOR_IOC_MAXNR    24  /* THIS MACRO IS NOT USED NOW... */

typedef struct {
    dev_t devno;
    struct class *class;
    struct cdev cdev;
    struct platform_device *spi;
    struct input_dev *input;

    unsigned int users;
    u8 device_available;    /* changed during fingerprint chip sleep and wakeup phase */
    // struct early_suspend early_suspend;
    u8 probe_finish;
    u8 irq_count;
    /* bit24-bit32 of signal count */
    /* bit16-bit23 of event type, 1: key down; 2: key up; 3: fp data ready; 4: home key */
    /* bit0-bit15 of event type, buffer status register */
    u32 event_type;
    u8 sig_count;
    u8 is_sleep_mode;
    volatile unsigned int RcvIRQ;
    //irq
    int irq;
    int irq_gpio;
    int reset_gpio;
    int power_gpio;
    struct wake_lock ttw_wl;
    //wait queue
    wait_queue_head_t wq_irq_return;
    int cancel;
#if  FP_NOTIFY
    struct notifier_block notifier;
    u8 fb_status;
#endif
} fpsensor_data_t;

//static void fpsensor_input_cleanup(fpsensor_data_t *fpsensor);
static void fpsensor_dev_cleanup(fpsensor_data_t *fpsensor);
static void fpsensor_hw_reset(int delay);
static void setRcvIRQ(int val);

#endif    /* __FPSENSOR_SPI_TEE_H */
/* add by leadcore end  */

