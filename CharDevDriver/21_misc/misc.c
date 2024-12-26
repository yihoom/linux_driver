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
#include <linux/miscdevice.h>

#define MISCBEEP_NAME "miscbeep"
#define MISCBEEP_MINOR 144

struct miscbeep_drv{
    struct device_node *nd;
    int beep_gpio;

};

struct miscbeep_drv miscbeep;

int beep_open (struct inode *inode, struct file *file)
{
    file->private_data = &miscbeep;
    return 0;
}

int beep_release (struct inode *inode, struct file *file)
{
    struct miscbeep_drv *dev = (struct miscbeep_drv *)file->private_data;
    return 0;
}

ssize_t beep_write (struct file *file, const char __user *user_buf,
			       size_t length, loff_t *offset)
{
    struct miscbeep_drv *dev = (struct miscbeep_drv *)file->private_data;

    u8 databuf;
    int ret;
    ret = copy_from_user(&databuf, user_buf, length);
    gpio_set_value(dev->beep_gpio, !databuf);
    return 0;
}

struct file_operations beepfop = {
    .open = beep_open,
    .release = beep_release,
    .write = beep_write,
};


/*MISC设备驱动*/
struct miscdevice miscdeepdev = {
    .name = MISCBEEP_NAME,
    .minor = MISCBEEP_MINOR,
    .fops = &beepfop,
};

static int miscbeep_probe(struct platform_device *dev)
{
    /*1.初始化蜂鸣器IO*/
    miscbeep.nd = dev->dev.of_node;
    /*获取及初始化io*/
    miscbeep.beep_gpio = of_get_named_gpio(miscbeep.nd, "beep-gpios", 0);
    gpio_request(miscbeep.beep_gpio, "beepgpio");
    /*输出，默认高电平*/
    gpio_direction_output(miscbeep.beep_gpio, 1); 


    /*MISC驱动的注册*/
    misc_register(&miscdeepdev);

    return 0;
}

int miscbeep_remove(struct platform_device *dev)
{
    /*拉高关闭*/
    gpio_set_value(miscbeep.beep_gpio, 1);
    gpio_free(miscbeep.beep_gpio);

    /*MISC驱动注销*/
    misc_deregister(&miscdeepdev);
    return 0;
}

struct of_device_id beep_of_match_table[] = {
    {.compatible = "alientek,beep"},
    {/* sentinel */},
};

static struct platform_driver miscbeep_driver = {
    .driver ={
        .name = "miscbeep",
        .of_match_table = beep_of_match_table,
    },
    .probe = miscbeep_probe,
    .remove = miscbeep_remove,
};


/*驱动入口函数*/
static int __init miscbeep_init(void)
{
    return platform_driver_register(&miscbeep_driver);
}


/*驱动出口函数*/
static void __exit miscbeep_exit(void)
{
    platform_driver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");

