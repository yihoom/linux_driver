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


#define DEV_CNT 1
#define DEV_NAME "gpiobeep"

struct beep_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *dev_nd;
    int beep_gpio;
    
};

struct beep_dev gpioled;

int gpioled_open (struct inode *inode, struct file *file)
{
    file->private_data = &gpioled;
    return 0;
}

int gpioled_release (struct inode *inode, struct file *file)
{
    struct beep_dev *dev = (struct beep_dev *)file->private_data;
    return 0;
}

ssize_t gpioled_write (struct file *file, const char __user *user_buf,
			       size_t length, loff_t *offset)
{
    struct beep_dev *dev = (struct beep_dev *)file->private_data;

    u8 databuf;
    int ret;
    ret = copy_from_user(&databuf, user_buf, length);
    gpio_set_value(gpioled.beep_gpio, !databuf);
    return 0;
}


static const struct file_operations fop =
{
    .owner = THIS_MODULE,
    .open = gpioled_open,
    .release = gpioled_release,
    .write = gpioled_write,
};


static int __init gpioled_init(void)
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
    
    /*初始化设备节点*/
    gpioled.dev_nd = of_find_node_by_path("/beepled");
    if(gpioled.dev_nd == NULL)
    {
        ret = -EINVAL;
        goto fail_node;
    }

    /*找到gpio*/
    gpioled.beep_gpio = of_get_named_gpio(gpioled.dev_nd, "beep-gpios", 0);
    if(gpioled.beep_gpio < 0)
    {
        ret = gpioled.beep_gpio;
        goto fail_gpio;
    }
    printk("gpio num = %d\r\n", gpioled.beep_gpio);
    
    /*申请GPIO*/
    ret = gpio_request(gpioled.beep_gpio, "beep_gpio3");
    if(ret < 0)
    {
        printk("cannot request gpio\r\n");
        goto fail_gpio_request;
    }

    ret = gpio_direction_output(gpioled.beep_gpio, 0);
    if(ret < 0)
    {
        goto fail_gpio_dir;
    }
    return 0;

    gpio_set_value(gpioled.beep_gpio, 0);

fail_gpio_dir:
    gpio_free(gpioled.beep_gpio);
fail_gpio_request:
fail_gpio:
fail_node:
    device_destroy(gpioled.class, gpioled.devid);

fail_device:
    class_destroy(gpioled.class);

fail_class:
    cdev_del(&(gpioled.cdev));

fail_cdev:
    unregister_chrdev_region(gpioled.devid, DEV_CNT);

fail_devid:
    return ret;


}

static void __exit gpioled_exit(void)
{
    gpio_set_value(gpioled.beep_gpio, 1);
    /*释放GPIO*/
    gpio_free(gpioled.beep_gpio);
    /*删除设备*/
    device_destroy(gpioled.class, gpioled.devid);
    /*删除类*/
    class_destroy(gpioled.class);
    /*删除cdev*/
    cdev_del(&(gpioled.cdev));
    /*注销设备号*/
    unregister_chrdev_region(gpioled.devid, DEV_CNT);
}


module_init(gpioled_init);
module_exit(gpioled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
