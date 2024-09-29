#include <linux/module.h>

static int __init chardevbase_init(void)
{

	return 0;
}

static void __exit chardevbase_exit(void)
{

}


/**
 * 驱动模块的入口与出口
*/
module_init(chardevbase_init);  //入口，加载模块时执行
module_exit(chardevbase_exit);  //出口，卸载模块时执行

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
