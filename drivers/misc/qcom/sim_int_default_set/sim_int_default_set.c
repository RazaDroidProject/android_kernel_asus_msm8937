//addde by  zhaozhensen@wind-mobi 20180110  for sim_card_eint_default begin
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

int sim_card_eint_default_irq;

struct pinctrl *sim_int_pinctrl;
struct pinctrl *sim2_int_pinctrl;
struct pinctrl_state *sim_int_state;
struct pinctrl_state *sim2_int_state;
struct device_node *sim_card_eint_default_irq_node;


static int sim_card_eint_default_probe(struct platform_device *dev)
{

   	sim_card_eint_default_irq_node = of_find_compatible_node(NULL, NULL, "qcom, sim-card-eint");
	 
	//sim_card_eint_default_setup_eint();
	
	sim_int_pinctrl = devm_pinctrl_get(&dev->dev);
	sim2_int_pinctrl = devm_pinctrl_get(&dev->dev);

	if (IS_ERR(sim_int_pinctrl))
		printk("Cannot find  sim_int_pinctrl!");	
	else
		sim_int_state = pinctrl_lookup_state(sim_int_pinctrl, "sim1_default");

	if(IS_ERR_OR_NULL(sim_int_state))
		printk("wind-log:sim_int_state NULL \n");
	else
		pinctrl_select_state(sim_int_pinctrl, sim_int_state);


	if (IS_ERR(sim2_int_pinctrl))
		printk("Cannot find  sim2_int_pinctrl!");	
	else
		sim2_int_state = pinctrl_lookup_state(sim2_int_pinctrl, "sim2_default");

	if(IS_ERR_OR_NULL(sim2_int_state))
		printk("wind-log:sim2_int_state NULL \n");
	else
		pinctrl_select_state(sim2_int_pinctrl, sim2_int_state);


	printk("wind-log:enter sim_card_eint_default_probe \n");
	return 0;
}

static int sim_card_eint_default_remove(struct platform_device *dev)
{
	printk("wind-log:remove sim_card_eint_default\n");
	return 0;
}


struct of_device_id sim_card_eint_of_match[] = {
	{ .compatible = "qcom, sim-card-eint" },	
	{},
};

static struct platform_driver sim_card_eint_default_driver = {
	.probe = sim_card_eint_default_probe,
	.remove =sim_card_eint_default_remove,
	.shutdown = NULL,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = "sim_card_eint_default",
		   	.of_match_table = sim_card_eint_of_match,
		   },
};


static int  sim_card_eint_default_init(void)
{
    int ret;
	
	ret = platform_driver_register(&sim_card_eint_default_driver);
	if(ret){
		printk("Unable to sim_card_eint_default_driver register(%d)\n",ret);
		return ret;
	}

	return 0;
}

static void __exit sim_card_eint_default_exit(void)
{
	platform_driver_unregister(&sim_card_eint_default_driver);
}


late_initcall(sim_card_eint_default_init);
module_exit(sim_card_eint_default_exit);
MODULE_AUTHOR("ZZS");
MODULE_DESCRIPTION("Sim_card_eint_default Driver");
MODULE_LICENSE("GPL");

//addde by  zhaozhensen@wind-mobi 20180110  for sim_card_eint_default end