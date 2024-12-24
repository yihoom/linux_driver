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
#include <linux/of_gpio.h>
#include <linux/interrupt.h>

#include <linux/ide.h>
#include <linux/platform_device.h>

#define DEV_CNT 1
#define DEV_NAME "dtspltled"

struct gpioled_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *dev_nd;
    int led_gpio;
    
};

struct gpioled_dev gpioled;

int gpioled_open (struct inode *inode, struct file *file)
{
    file->private_data = &gpioled;
    return 0;
}

int gpioled_release (struct inode *inode, struct file *file)
{
    struct gpioled_dev *dev = (struct gpioled_dev *)file->private_data;
    return 0;
}

ssize_t gpioled_write (struct file *file, const char __user *user_buf,
			       size_t length, loff_t *offset)
{
    struct gpioled_dev *dev = (struct gpioled_dev *)file->private_data;
    u8 databuf;
    int ret;
    ret = copy_from_user(&databuf, user_buf, length);
    gpio_set_value(dev->led_gpio, !databuf);
    return 0;
}


static const struct file_operations fop =
{
    .owner = THIS_MODULE,
    .open = gpioled_open,
    .release = gpioled_release,
    .write = gpioled_write,
};


int led_probe(struct platform_device *dev)
{
    int ret = 0;
    gpioled.major = 0;
    /*注册设备号*/
    if(gpioled.major)
    {
        gpioled.devid = MKDEV(gpioled.major , 0);
        ret = register_chrdev_region(gpioled.devid, DEV_CNT, DEV_NAME);
        if(ret < 0)
        {
            ret = -EINVAL;
            goto fail_devid;
        }

    }
    else
    {
        ret = alloc_chrdev_region(&gpioled.devid, 0, DEV_CNT, DEV_NAME);
        if(ret < 0)
        {
            ret = -EINVAL;
            goto fail_devid;
        }
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
        printk("devid=%d\r\n", gpioled.devid);
        printk("major=%d, minor=%d\r\n", gpioled.major, gpioled.minor);
    }

    /*初始化并cdev*/
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&(gpioled.cdev), &fop);
    ret = cdev_add(&(gpioled.cdev), gpioled.devid, DEV_CNT);
    if( ret < 0)
    {
        goto fail_cdev;
    }

    

    /*创建类*/
    gpioled.class = class_create(THIS_MODULE, DEV_NAME);
    if(IS_ERR(gpioled.class))
    {
        ret = PTR_ERR(gpioled.class);
        goto fail_class;
    }


    /*创建设备*/
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, DEV_NAME);
    if(IS_ERR(gpioled.device))
    {
        ret = PTR_ERR(gpioled.device);
        goto fail_device;
    }
    
    #if 0
    //查找节点
    gpioled.dev_nd = of_find_node_by_path("/gpioled");
    if(gpioled.dev_nd == NULL)
    {
        ret = -EINVAL;
        goto fail_findnode;
    }
    #endif

    /*驱动和设备匹配成功以后，设备信息就会从设备树转为结构体*/
    gpioled.dev_nd = dev->dev.of_node;

    /*获取led所对应的GPIO*/
    gpioled.led_gpio = of_get_named_gpio(gpioled.dev_nd, "led-gpios", 0);
    if(gpioled.led_gpio < 0)
    {
        printk("can't find led_gpio\r\n");
        ret = -EINVAL;
        goto fail_findnode;
    }
    
    printk("led gpio num = %d\r\n", gpioled.led_gpio);

    /*申请io*/
    ret = gpio_request(gpioled.led_gpio, "led-gpios");
    if(ret){
        printk("failed to request the led gpio\r\n");
        ret = -EINVAL;
        goto fail_findnode;
    }

    /*使用io,设置为输出，此处是高电平还是低电平，与设备树中的active_low有关,1为有效，0为无效*/
    ret = gpio_direction_output(gpioled.led_gpio, 1);   
    if(ret){
        ret = -EINVAL;
        goto fail_setoutput;
    }

    /*设置输出低电平*/
    gpio_set_value(gpioled.led_gpio, 0);

    return 0;

fail_setoutput:
    gpio_free(gpioled.led_gpio);

fail_findnode:
    device_destroy(gpioled.class, gpioled.devid);

fail_device:
    class_destroy(gpioled.class);

fail_class:
    cdev_del(&(gpioled.cdev));

fail_cdev:
    unregister_chrdev_region(gpioled.devid, DEV_CNT);

fail_devid:
    return ret;
    return 0;
}
int led_remove(struct platform_device *dev)
{
    /*关灯*/
    gpio_set_value(gpioled.led_gpio, 1);

    /*释放IO*/
    gpio_free(gpioled.led_gpio);

    /*删除设备*/
    device_destroy(gpioled.class, gpioled.devid);
    /*删除类*/
    class_destroy(gpioled.class);
    /*删除cdev*/
    cdev_del(&(gpioled.cdev));
    /*注销设备号*/
    unregister_chrdev_region(gpioled.devid, DEV_CNT);
    return 0;
}

/*可以有多条*/
struct of_device_id led_of_match_table[] = {
    {.compatible = "alientek,gpioled"},
    {/* sentinel */},
};

struct platform_driver led_driver = {
    .driver = {
        .name = "jyh_led",      //无设备数的时候，驱动名字用来与设备匹配
        .of_match_table = led_of_match_table,   //设备数匹配表
    },
    .probe = led_probe,
    .remove = led_remove,
    
};

/**
 * 驱动加载
*/

static int __init leddriver_init(void)
{
    /*注册platform驱动*/
    return platform_driver_register(&led_driver);
}

/**
 * 卸载驱动
*/
static void __exit leddriver_exit(void)
{
   platform_driver_unregister(&led_driver);
}


module_init(leddriver_init);
module_exit(leddriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
