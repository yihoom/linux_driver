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
#define DEV_NAME "key"

#define KEYVAL    0xF0
#define INVALKEY  0x0

struct key_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *dev_nd;
    int key_gpio;
    atomic_t keyvalue;
};

struct key_dev key;

static int key_open (struct inode *inode, struct file *file)
{
    file->private_data = &key;
    return 0;
}

static int key_release (struct inode *inode, struct file *file)
{
    struct key_dev *dev = (struct key_dev *)file->private_data;
    return 0;
}

static ssize_t key_write (struct file *file, const char __user *user_buf,
			       size_t length, loff_t *offset)
{
    struct key_dev *dev = (struct key_dev *)file->private_data;

    int ret = 0;
    return ret;
}

static ssize_t key_read(struct file *file,
			char __user *user_buf, size_t length, loff_t *pos)
{
    int ret = 0;
    struct key_dev *dev = (struct key_dev *)file->private_data;
    int value;

    //按下
    if(gpio_get_value(dev->key_gpio) == 0)
    {
        //释放
        while(!(gpio_get_value(dev->key_gpio)));
        atomic_set(&key.keyvalue, KEYVAL);
    }
    else
    {
        atomic_set(&key.keyvalue, INVALKEY);
    }

    value = atomic_read(&key.keyvalue);

    ret = copy_to_user(user_buf, &value, length);

    return ret;
}


static const struct file_operations fop =
{
    .owner = THIS_MODULE,
    .open = key_open,
    .release = key_release,
    .write = key_write,
    .read = key_read,
};

static int keyio_init(struct key_dev * dev)
{
    int ret = 0;

    dev->dev_nd = of_find_node_by_path("/key");
    if(dev->dev_nd == NULL)
    {
        ret = -EINVAL;
        goto fail_nd;
    }

    dev->key_gpio = of_get_named_gpio(dev->dev_nd, "key-gpios", 0);
    if(dev->key_gpio < 0)
    {
        ret = dev->key_gpio;
        goto fail_get_gpio;
    }

    ret = gpio_request(dev->key_gpio, "keygpio");
    if(ret < 0)
    {
        printk("cannot request gpio\r\n");
        goto fail_request_gpio;
    }

    ret = gpio_direction_input(dev->key_gpio);
    if(ret < 0)
    {
        goto fail_inpuset;
    }

    return 0;

fail_inpuset:
    gpio_free(dev->key_gpio);

fail_request_gpio:
fail_get_gpio:
fail_nd:
    return ret;
}

static int keyio_deinit(struct key_dev * dev)
{
    gpio_free(dev->key_gpio);

    return 0;
}


static int __init key_init(void)
{
    int ret = 0;
    key.major = 0;

    /*初始化atomic*/
    atomic_set(&key.keyvalue, INVALKEY);

    /*注册设备号*/
    if(key.major)
    {
        key.devid = MKDEV(key.major , 0);
        ret = register_chrdev_region(key.devid, DEV_CNT, DEV_NAME);
        if(ret < 0)
        {
            ret = -EINVAL;
            goto fail_devid;
        }

    }
    else
    {
        ret = alloc_chrdev_region(&key.devid, 0, DEV_CNT, DEV_NAME);
        if(ret < 0)
        {
            ret = -EINVAL;
            goto fail_devid;
        }
        key.major = MAJOR(key.devid);
        key.minor = MINOR(key.devid);
        printk("devid=%d\r\n", key.devid);
        printk("major=%d, minor=%d\r\n", key.major, key.minor);
    }

    /*初始化并cdev*/
    key.cdev.owner = THIS_MODULE;
    cdev_init(&(key.cdev), &fop);
    ret = cdev_add(&(key.cdev), key.devid, DEV_CNT);
    if( ret < 0)
    {
        goto fail_cdev;
    }

    

    /*创建类*/
    key.class = class_create(THIS_MODULE, DEV_NAME);
    if(IS_ERR(key.class))
    {
        ret = PTR_ERR(key.class);
        goto fail_class;
    }


    /*创建设备*/
    key.device = device_create(key.class, NULL, key.devid, NULL, DEV_NAME);
    if(IS_ERR(key.device))
    {
        ret = PTR_ERR(key.device);
        goto fail_device;
    }
    
    ret = keyio_init(&key);
    if(ret < 0)
    {
        goto fail_keyio;
    }

    return 0;

fail_keyio:
fail_device:
    class_destroy(key.class);

fail_class:
    cdev_del(&(key.cdev));

fail_cdev:
    unregister_chrdev_region(key.devid, DEV_CNT);

fail_devid:
    return ret;


}

static void __exit key_exit(void)
{
    keyio_deinit(&key);
    /*删除设备*/
    device_destroy(key.class, key.devid);
    /*删除类*/
    class_destroy(key.class);
    /*删除cdev*/
    cdev_del(&(key.cdev));
    /*注销设备号*/
    unregister_chrdev_region(key.devid, DEV_CNT);
}


module_init(key_init);
module_exit(key_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
