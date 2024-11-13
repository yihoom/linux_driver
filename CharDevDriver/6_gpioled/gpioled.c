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

#define DEV_CNT 1
#define DEV_NAME "gpioled"

struct gpioled_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    
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
    }
    else
    {
        ret = alloc_chrdev_region(&gpioled.devid, 0, DEV_CNT, DEV_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
        printk("devid=%d\r\n", gpioled.devid);
        printk("major=%d, minor=%d\r\n", gpioled.major, gpioled.minor);
    }

    /*初始化并cdev*/
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&(gpioled.cdev), &fop);
    cdev_add(&(gpioled.cdev), gpioled.devid, DEV_CNT);
    

    /*创建类*/
    gpioled.class = class_create(THIS_MODULE, DEV_NAME);


    /*创建设备*/
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, DEV_NAME);
    return ret;

}

static void __exit gpioled_exit(void)
{
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
