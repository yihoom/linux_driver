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
#include <linux/wait.h>
#include <linux/ide.h>
#include <linux/poll.h>


#define DEV_CNT 1
#define DEV_NAME "imx6uirq"

#define KEYVAL    0xF0
#define INVALKEY  0x0
#define KEY_NUM 1

struct irq_keyboj
{
    int key_gpio;           /*io编号*/
    int irqnum;             /*中断号*/
    uint8_t value;          /*键指*/
    char name[10];          /*名字*/
    irqreturn_t (*handler)(int, void *);/*中断处理函数*/
};


struct key_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *dev_nd;
    struct irq_keyboj imx6uirq[KEY_NUM];
    struct timer_list timer;
    atomic_t keyvalue;
    atomic_t relesval;

    wait_queue_head_t r_wait; /* 读等待队列头 */
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
    unsigned char keyvalue = 0;
    unsigned char releaseval = 0;
    struct key_dev *dev = file->private_data;

    // /*等待事件*/
    // wait_event_interruptible(dev->r_wait, atomic_read(&dev->relesval));
    if(file->f_flags & O_NONBLOCK)  //非阻塞式访问
    {
        if(atomic_read(&dev->relesval) == 0)
        {
            return -EAGAIN;
        }
    }
    else    //阻塞式访问
    {
        wait_event_interruptible(dev->r_wait, atomic_read(&dev->relesval));
    }
    


    keyvalue = atomic_read(&dev->keyvalue);
    releaseval = atomic_read(&dev->relesval);

    if(releaseval)
    {
        if(keyvalue & 0x80)
        {
            keyvalue &= ~0x80;
            ret = copy_to_user(user_buf, &keyvalue, sizeof(keyvalue));
            atomic_set(&dev->relesval, 0);
            atomic_set(&dev->keyvalue, 0);
        }
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;

// data_error:
//     // __set_current_state(TASK_RUNNING);
//     // remove_wait_queue(&dev->r_wait, &wait);
//     return ret;

}

static unsigned int key_pool(struct file *filp, struct poll_table_struct * wait)
{
    unsigned int mask = 0;
    struct key_dev *dev = (struct key_dev *)filp->private_data;

    poll_wait(filp, &dev->r_wait, wait);

    if(atomic_read(&dev->relesval))
    {
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

static const struct file_operations fop =
{
    .owner = THIS_MODULE,
    .open = key_open,
    .release = key_release,
    .write = key_write,
    .read = key_read,
    .poll = key_pool,
};

static irqreturn_t key_irqhandler(int irq, void *dev_key)
{
    
    struct key_dev *dev = (struct key_dev *)dev_key;


    // if(val == 0)
    // {
    //     printk("key0 pushed\r\n");
    // }
    // else if(val == 1)
    // {
    //     printk("key0 relieased\r\n");
    // }
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));

    return IRQ_HANDLED;
}

void timer_callbackfn(unsigned long arg)
{
    int val = 0;
    struct key_dev *dev = (struct key_dev *)arg;
    val = gpio_get_value(dev->imx6uirq[0].key_gpio);

    if(val == 0)
    {
        atomic_set(&key.keyvalue, 0X01);
        // printk("key0 pushed\r\n");
    }
    else if(val == 1)
    {
        // atomic_set(&key.keyvalue, 1);
        atomic_set(&key.keyvalue, 0x80 | 0X01);
        atomic_set(&key.relesval, 1);
        
        // printk("key0 relieased\r\n");
    }

    /*唤醒进程*/
    if(atomic_read(&dev->relesval))
    {
        wake_up(&key.r_wait);
    }

    // mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerprd));
}


static int keyio_init(struct key_dev * dev)
{
    int ret = 0;
    int i;

    dev->dev_nd = of_find_node_by_path("/key");
    if(dev->dev_nd == NULL)
    {
        ret = -EINVAL;
        goto fail_nd;
    }

    for(i = 0; i < KEY_NUM; i++)
    {
        dev->imx6uirq[i].key_gpio = of_get_named_gpio(dev->dev_nd, "key-gpios", i);
    }

    
    for(i = 0; i < KEY_NUM; i++)
    {
        memset(dev->imx6uirq[i].name, 0, sizeof(dev->imx6uirq[i].name));
        sprintf(dev->imx6uirq[i].name, "KEY%d", i);
        ret = gpio_request(dev->imx6uirq[i].key_gpio, dev->imx6uirq[i].name);
        gpio_direction_input(dev->imx6uirq[i].key_gpio);

        /*获取中断号*/
        dev->imx6uirq[i].irqnum = gpio_to_irq(dev->imx6uirq[i].key_gpio);
        #if 0
        //通用的方式
        dev->imx6uirq[i].irqnum = irq_of_parse_and_map(dev->dev_nd, i);
        #endif
    }

    dev->imx6uirq[0].handler = key_irqhandler;
    /*初始化中断*/
    for(i = 0; i < KEY_NUM; i++)
    {
        ret = request_irq(dev->imx6uirq[i].irqnum, dev->imx6uirq[i].handler, 
                    IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
                    dev->imx6uirq[i].name, dev);
    }

    //初始化时钟
    init_timer(&dev->timer);
    dev->timer.function = timer_callbackfn;
    dev->timer.data = (unsigned long)dev;

    return 0;

fail_nd:
    return ret;
}

static int keyio_deinit(struct key_dev * dev)
{
    int i;
    for(i = 0; i < KEY_NUM; i++)
    {
        free_irq(dev->imx6uirq[i].irqnum, dev);
    }

    for(i = 0; i < KEY_NUM; i++)
    {
        gpio_free(dev->imx6uirq[i].key_gpio);
    }

    del_timer_sync(&dev->timer);

    return 0;
}


static int __init keyirq_init(void)
{
    int ret = 0;
    key.major = 0;

    /*初始化atomic*/
    atomic_set(&key.keyvalue, INVALKEY);
    atomic_set(&key.relesval ,INVALKEY);
    

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

    init_waitqueue_head(&key.r_wait);


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


module_init(keyirq_init);
module_exit(key_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
