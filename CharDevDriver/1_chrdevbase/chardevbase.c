#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init chardevbase_init(void)
{
	printk("chardevbase_init\r\n");
	return 0;
}

static void __exit chardevbase_exit(void)
{
	printk("chardevbase_exit\r\n");
}


/**
 * 驱动模块的入口与出口
*/
module_init(chardevbase_init);  //入口，加载模块时执行
module_exit(chardevbase_exit);  //出口，卸载模块时执行

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
