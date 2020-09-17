/* Copyright (C) MicroArray
 * MicroArray Fprint Driver Code
 * qualcomm-settings.c
 * Date: 2017-3-15
 * Version: v4.0.06
 * Author: guq
 * Contact: guq@microarray.com.cn
 */

//#include "x86-settings.h"
#include "qualcomm-settings.h"
#include "madev.h"
static unsigned int ret;
static unsigned int irq_gpio;

static int pwr_gpio;
//pin control sturct data, define using for the dts settings,
struct pinctrl *mas_finger_pinctrl;
struct pinctrl_state 		*mas_finger_power2v8_on, *mas_finger_power2v8_off, 	//power2v8
							*mas_finger_power1v8_on, *mas_finger_power1v8_off,	//power1v8
							*mas_finger_eint_on, *mas_finger_eint_off,			//eint
							*mas_spi_ck_on, *mas_spi_ck_off,					//for ck
							*mas_spi_cs_on, *mas_spi_cs_off,					//for cs
							*mas_spi_mi_on, *mas_spi_mi_off,					//for mi
							*mas_spi_mo_on, *mas_spi_mo_off,					//for mo
							*mas_spi_default;									//same odms only use default to setting the dts
static struct device_node *node;


/**
 *    the platform struct start,for getting the platform device to set gpio state
 */
//#ifdef CONFIG_OF
#if 0
static struct of_device_id sof_match[] = {
        { .compatible = MA_DTS_NAME, },						//this name is used for matching the dts device for settings the gpios
};
MODULE_DEVICE_TABLE(of, sof_match);
//#endif
static struct platform_driver spdrv = {
        .probe    = mas_plat_probe,
        .remove  = mas_plat_remove,
        .driver = {
                .name  = MA_DRV_NAME,
                .owner = THIS_MODULE,                   
//#ifdef CONFIG_OF
                .of_match_table = sof_match,
//#endif
        }
};
#endif
/**
  *  the platform struct start,for getting the platform device to set gpio state end
  */


/**
 *  the spi struct date start,for getting the spi_device to set the spi clock enable start
 */

static const struct of_device_id madev_of_match[] = {
	{.compatible = "mediatek,mas_finger",},
	{}
};
MODULE_DEVICE_TABLE(of, madev_of_match);
struct spi_device_id sdev_id = {MA_DRV_NAME, 0};
struct spi_driver sdrv = {
		.driver = {
			.name = "madev",
			.bus = &spi_bus_type,
			.owner = THIS_MODULE,
			.of_match_table = madev_of_match,
		},
        .probe = mas_probe,
        .remove = mas_remove,
 //    .id_table = &sdev_id,
};
//driver end
/*static const struct of_device_id sprd_pmic_of_match[] = {
        {.compatible = "sprd,sc2723t",},
        {.compatible = "sprd,sc2723",},
        {.compatible = "sprd,sc2731",},
        {.compatible = "sprd,sc2721",},
        {},
};
static struct spi_driver sprd_pmic_driver = {
        .driver = {
                   .name = "pmic",
                   .bus = &spi_bus_type,
                   .owner = THIS_MODULE,
                   .of_match_table = sprd_pmic_of_match,
                   },
        .probe = mas_probe,
        .remove = mas_remove,
};*/
#if 0
struct spi_board_info smt_info[] __initdata = {
        [0] = {
                .modalias = "madev",
                .max_speed_hz = (6*1000000),
                .bus_num = 2,
                .chip_select = 0,
                .mode = SPI_MODE_0,
        },
};
#endif
//device end

/**
 *  the spi struct date start,for getting the spi_device to set the spi clock enable end
 */


void mas_select_transfer(struct spi_device *spi, int len) {
/*    static int mode = -1;
    int tmp = len>32? DMA_TRANSFER: FIFO_TRANSFER;
    struct mt_chip_conf *conf = NULL;
    if(tmp!=mode) {
        conf = (struct mt_chip_conf *) spi->controller_data;
        conf->com_mod = tmp;
        spi_setup(spi);
        mode = tmp;
    }*/
}

/*
 *  set spi speed, often we must check whether the setting is efficient
 */

void ma_spi_change(struct spi_device *spi, unsigned int  speed, int flag)
{
/*    struct mt_chip_conf *mcc = (struct mt_chip_conf *)spi->controller_data;
    if(flag == 0) { 
        mcc->com_mod = FIFO_TRANSFER;
    } else {
        mcc->com_mod = DMA_TRANSFER;
    }   
    mcc->high_time = speed;
    mcc->low_time = speed;
    if(spi_setup(spi) < 0){
        MALOGE("change the spi error!\n");
    }    */
}


int mas_do_some_for_probe(struct spi_device *spi){
	//int ret = 0;
	node = of_find_compatible_node(NULL, NULL, "microarray,fingerprint");
	irq_gpio =  of_get_named_gpio(node, "mafp,gpio_irq",0);
	pwr_gpio = of_get_named_gpio(node, "mafp,gpio_power", 0);
	printk("MAFP_ __func__ irq_gpio = %d pwr_gpio = %d\n", irq_gpio, pwr_gpio);
	if(!gpio_is_valid(irq_gpio)){
		MALOGE("irq gpio is in valied!\n ");
		return -1;
	}
	//if(!ret) {
		//MALOGE("pwr gpio is invalid\n");
	//	return -2;
	//} 
        //ret = gpio_request(pwr_gpio, "ma_pwr");
        //if (ret)
        //{
	//   MALOGE("pwr gpio request failed\n");
        //    return -3;
        //    }
        //gpio_direction_output(pwr_gpio, 1);
	return 0;
}

int mas_undo_some_for_probe(void);
int mas_undo_some_for_probe(){
	if(gpio_is_valid(irq_gpio)){
		gpio_free(irq_gpio);
	}	
	if(gpio_is_valid(pwr_gpio)){
		gpio_free(pwr_gpio);
	}
	return 0;
}

int mas_get_platform(void) {
	MALOGD("start");
//	ret = platform_driver_register(&spdrv);
//	spdrv.probe = mas_plat_probe;
//	mas_plat_probe(spi);	
	/*if(ret){
		MALOGE("platform_driver_register");
	}*/
//	ret = spi_register_board_info(smt_info, ARRAY_SIZE(smt_info));
/*	if(ret){
		MALOGE("spi_register_board_info");
	}
*/
	ret = spi_register_driver(&sdrv);
	if(ret) {
		MALOGE("spi_register_driver");
	}
	printk("MAFP mas_get_platform  spi_register_driver ret = %d", ret);
	MALOGD("end");
	return ret;
}

int mas_remove_platform(void){
	mas_undo_some_for_probe();
	spi_unregister_driver(&sdrv);
	return 0;
}

int mas_finger_get_gpio_info(struct platform_device *pdev){
    //MALOGD("start!");
//	smas->spi = spi;
/*	node = of_find_compatible_node(NULL, NULL, MA_DTS_NAME);
	mas_finger_pinctrl = devm_pinctrl_get(&spi->dev);
	if (IS_ERR(mas_finger_pinctrl)) {
		ret = PTR_ERR(mas_finger_pinctrl);
		dev_err(&spi->dev, "mas_finger_pinctrl cannot find pinctrl\n");
		return ret;
	}

*//**		this is the demo, setup follow the requirement
 *		mas_finger_eint_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_int_as_int");
 *		if (IS_ERR(mas_finger_eint_on)) {
 *			ret = PTR_ERR(mas_finger_eint_on);
 *			dev_err(&pdev->dev, " Cannot find mas_finger pinctrl mas_finger_eint_on!\n");
 *			return ret;
 *		}
 *      if needed, change the dts label and the pinctrl for the other gpio
 */
 	
#if 0
 	mas_finger_power2v8_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_en1");
	if (IS_ERR(mas_finger_power2v8_on)) {
		ret = PTR_ERR(mas_finger_power2v8_on);
		dev_err(&pdev->dev, " Cannot find mas_finger_power2v8_on pinctrl!\n");
		return ret;
	}
 	mas_finger_power2v8_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_en0");
	if (IS_ERR(mas_finger_power2v8_off)) {
		ret = PTR_ERR(mas_finger_power2v8_off);
		dev_err(&pdev->dev, " Cannot find mas_finger_power2v8_off pinctrl!\n");
		return ret;
	}

	mas_finger_power1v8_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_18v_en1");
	if (IS_ERR(mas_finger_power1v8_on)) {
		ret = PTR_ERR(mas_finger_power1v8_on);
		dev_err(&pdev->dev, " Cannot find mas_finger_power1v8_on pinctrl!\n");
		return ret;
	}

	mas_finger_power1v8_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_18v_en0");
	if (IS_ERR(mas_finger_power1v8_off)) {
		ret = PTR_ERR(mas_finger_power1v8_off);
		dev_err(&pdev->dev, " Cannot find mas_finger_power1v8_off pinctrl!\n");
		return ret;
	}

 	mas_spi_mi_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mi_as_spi0_mi");
	if (IS_ERR(mas_spi_mi_on)) {
		ret = PTR_ERR(mas_spi_mi_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_mi_on pinctrl!\n");
		return ret;
	}
 	mas_spi_mi_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mi_as_gpio");
	if (IS_ERR(mas_spi_mi_off)) {
		ret = PTR_ERR(mas_spi_mi_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_mi_off pinctrl!\n");
		return ret;
	}
 	mas_spi_mo_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mo_as_spi0_mo");
	if (IS_ERR(mas_spi_mo_on)) {
		ret = PTR_ERR(mas_spi_mo_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_mo_on pinctrl!\n");
		return ret;
	}

	mas_spi_mo_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mo_as_gpio");
	if (IS_ERR(mas_spi_mo_off)) {
		ret = PTR_ERR(mas_spi_mo_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_mo_off!\n");
		return ret;
	}

	mas_spi_ck_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_clk_as_spi0_clk");
	if (IS_ERR(mas_spi_ck_on)) {
		ret = PTR_ERR(mas_spi_ck_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_ck_on pinctrl!\n");
		return ret;
	}

	mas_spi_ck_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_clk_as_gpio");
	if (IS_ERR(mas_spi_ck_off)) {
		ret = PTR_ERR(mas_spi_ck_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_ck_off pinctrl !\n");
		return ret;
	}

	mas_spi_cs_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_cs_as_spi0_cs");
	if (IS_ERR(mas_spi_cs_on)) {
		ret = PTR_ERR(mas_spi_cs_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_cs_on pinctrl!\n");
		return ret;
	}

	mas_spi_cs_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_cs_as_gpio");
	if (IS_ERR(mas_spi_cs_off)) {
		ret = PTR_ERR(mas_spi_cs_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_cs_off pinctrl!\n");
		return ret;
	}
#endif

/*	mas_finger_eint_on = pinctrl_lookup_state(mas_finger_pinctrl, "mas,gpio_irq");
	if (IS_ERR(mas_finger_eint_on)) {
		ret = PTR_ERR(mas_finger_eint_on);
		dev_err(&spi->dev, " Cannot find mas_finger_eint_on pinctrl!\n");
		return ret;
	}

  */      return 0;
}



int mas_finger_set_spi(int cmd){
#if 0
	switch(cmd)
	{
		case 0:
			if( (!IS_ERR(mas_spi_cs_off)) & (!IS_ERR(mas_spi_ck_off)) & (!IS_ERR(mas_spi_mi_off)) & (!IS_ERR(mas_spi_mo_off)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_cs_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_ck_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_mi_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_mo_off);
			}else{
				MALOGE("mas_spi_gpio_slect_pinctrl cmd=0 err!");
				return -1;
			}
			break;
		case 1:
			if( (!IS_ERR(mas_spi_cs_on)) & (!IS_ERR(mas_spi_ck_on)) & (!IS_ERR(mas_spi_mi_on)) & (!IS_ERR(mas_spi_mo_on)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_cs_on);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_ck_on);
	        	pinctrl_select_state(mas_finger_pinctrl, mas_spi_mi_on);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_mo_on);
			}else{
				MALOGE("mas_spi_gpio_slect_pinctrl cmd=1 err!");
				return -1;
			}
			break;
	}
#endif
	return 0;
}

int mas_finger_set_power(int cmd)
{
#if 0
	switch (cmd)
		{
		case 0 : 		
			if( (!IS_ERR(mas_finger_power2v8_off)) & (!IS_ERR(mas_finger_power1v8_off)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power2v8_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power1v8_off);
			}else{
				MALOGE("mas_power_gpio_slect_pinctrl cmd=0 err!");
				return -1;
			}
		break;
		case 1 : 		
			if( (!IS_ERR(mas_finger_power2v8_on)) & (!IS_ERR(mas_finger_power1v8_on)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power2v8_on);
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power1v8_on);
			}else{
				MALOGE("mas_power_gpio_slect_pinctrl cmd=1 err!");
				return -1;
			}
		break;
		}
#endif
	return 0;
}

/*
 * this is a demo function,if the power on-off switch by other way
 * modify it as the right way
 * on_off 1 on   0 off
 */
int mas_switch_power(unsigned int on_off){
	mas_finger_set_power(on_off);	//use this fuction directly if the dts way
	return 0;
}

int mas_finger_set_eint(int cmd)
{
	switch (cmd)
		{
		case 0 : 		
			if(!IS_ERR(mas_finger_eint_off)){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_eint_off);
			}else{
				MALOGE("mas_eint_gpio_slect_pinctrl cmd=0 err!");
				return -1;
			}		
			break;
		case 1 : 		
			if(!IS_ERR(mas_finger_eint_on)){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_eint_on);
			}else{
				MALOGE("mas_eint_gpio_slect_pinctrl cmd=1 err!");
				return -1;
			}
			break;
		}
	return 0;
}


int mas_finger_set_gpio_info(int cmd){
	ret = 0;
	ret |= mas_finger_set_spi(cmd);
	ret |= mas_finger_set_power(cmd);
	ret |= mas_finger_set_eint(cmd);
	return ret;
}

void mas_enable_spi_clock(struct spi_device *spi){
//	mt_spi_enable_clk(spi_master_get_devdata(spi->master)); 
}

void mas_disable_spi_clock(struct spi_device *spi){
//	mt_spi_disable_clk(spi_master_get_devdata(spi->master)); 
}

unsigned int mas_get_irq(struct spi_device *spi){
/*    struct device_node *node = NULL;
    //MALOGF("start");
    node = of_find_compatible_node(NULL, NULL, MA_EINT_DTS_NAME);
    return irq_of_parse_and_map(node, 0);*/

	int ret;
	unsigned int irq;
    	/*if(!gpio_is_valid(pwr_gpio)) {
		MALOGE("pwr gpio is invalid\n");
		return -2;
	} else {
		ret = gpio_request(pwr_gpio, "ma_pwr");
		gpio_direction_output(pwr_gpio, 1);
		msleep(1);
		gpio_set_value(pwr_gpio, 0);
		msleep(100);
		gpio_set_value(pwr_gpio, 1);
	}*/
	ret = gpio_request(irq_gpio, "ma_irq");
	if(ret){
		MALOGE("irq gpio_request failed\n");
		return -1;
	}
	pinctrl_gpio_direction_input(irq_gpio);
	irq = gpio_to_irq(irq_gpio);
	if(irq<0){
		MALOGE("gpio_to_irq failed\n");
		return irq;
	}
	enable_irq_wake(irq);
	//do pwr by the way 
	return irq;
}

/*
 * this function used for check the interrupt gpio state
 * @index 0 gpio level 1 gpio mode, often use 0
 * @return 0 gpio low 1 gpio high if index = 1,the return is the gpio mode
 *  		under 0  the of_property_read_u32_index return errno,check the dts as below:
 * last but not least use this function must checkt the label on dts file, after is an example:
 * ma_finger: ma_finger{
 *		compatible = "mediatek,afs120x";
 *		finger_int_pin = <100 0>;
 * }
 */
int mas_get_interrupt_gpio(unsigned int index){
/*	static unsigned int finger_int_pin;
	int val;
	val = of_property_read_u32_index(node, MA_INT_PIN_LABEL, index, &finger_int_pin);
	//the MA_INT_PIN_LABEL as same as the dts description, such as "finger_int_pin",modify in the mtk-settings.h file
	if(val < 0){
		return val;
	}
	finger_int_pin |=  0x80000000;
	val = mt_get_gpio_in(finger_int_pin);
	//printk if need
	return val;*/
	int ret;
	gpio_request(irq_gpio,"ma_irq");
	ret =  gpio_get_value(irq_gpio);
	printk(KERN_EMERG"MAFP guq  int %d\n", ret);
	return ret;
}

