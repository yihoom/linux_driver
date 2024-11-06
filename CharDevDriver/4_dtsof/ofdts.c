#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>

/***
 * 
 * 
 * backlight {
		compatible = "pwm-backlight";
		pwms = <&pwm1 0 5000000>;
		brightness-levels = <0 4 8 16 32 64 128 255>;
		default-brightness-level = <6>;
		status = "okay";
	};
 * 
 * 
*/

/*
*模块入口
*/
static int __init dtsof_init(void)
{
    int ret = 0;
    struct device_node *bl_nd = NULL;
    struct property *bl_proper = NULL;
    const char *s = NULL;
    int num;
    int i;
    int elemsize;
    u32 *brival = NULL;
    
    /*1.找到backlight节点*/
    bl_nd = of_find_node_by_path("/backlight");
    if(bl_nd == NULL)
    {
        printk("Cannot find node\r\n");
        ret = -EINVAL;
        goto failed_found;
    }

    /*2.获取属性*/
    bl_proper = of_find_property(bl_nd, "compatible", NULL);
    if(bl_proper == NULL)
    {
        printk("Cannot find node\r\n");
        ret = -EINVAL;
        goto failed_proper;
    }
    else{
        printk("compatible=%s\r\n", (char*)bl_proper->value);
    }
    
    ret = of_property_read_string(bl_nd, "status", &s);
    if( ret < 0)
    {
        goto failed_proper;
    }
    else
    {
        printk("status=%s\r\n", s);
    }

    /**3.获取数字*/
    ret = of_property_read_u32(bl_nd, "default-brightness-level", &num);
    if( ret < 0)
    {
        goto failed_proper;
    }
    else
    {
        printk("default-brightness-level=%d\r\n", num);
    }

    /*4.获取数组类型的属性*/
    elemsize = of_property_count_elems_of_size(bl_nd, "brightness-levels", sizeof(u32));
    if( elemsize < 0)
    {
        ret= -EINVAL;
        goto failed_proper;
    }
    else
    {
        printk("brightness-levels elem size=%d\r\n", elemsize);
    }
    //申请内存
    brival = kmalloc(elemsize * sizeof(u32), GFP_KERNEL);
    if(brival == NULL)
    {
        ret= -EINVAL;
        goto failed_proper;
    }

    //获取数组
    ret = of_property_read_u32_array(bl_nd, "brightness-levels",brival, elemsize);
    if( ret < 0)
    {
        goto failed_rdarry;
    }
    else
    {   
        for(i = 0; i < elemsize; i++)
        {
            printk("brightness-levels=[%d]=%d\r\n", i, brival[i]);
        }
        
    }
    kfree(brival);
    return 0;
failed_rdarry:
    kfree(brival);
failed_proper:
failed_found:
    return ret;
}


/**
 * 模块出口
 * 
*/
static void __exit dtsof_exit(void)
{

}


/* 模块入口和出口 */
module_init(dtsof_init);
module_exit(dtsof_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
