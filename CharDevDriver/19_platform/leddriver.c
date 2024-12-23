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


// struct platform_driver {
// 	int (*probe)(struct platform_device *);
// 	int (*remove)(struct platform_device *);
// 	void (*shutdown)(struct platform_device *);
// 	int (*suspend)(struct platform_device *, pm_message_t state);
// 	int (*resume)(struct platform_device *);
// 	struct device_driver driver;
// 	const struct platform_device_id *id_table;
// 	bool prevent_deferred_probe;
// };

struct newchrled_dev{
    dev_t devid;    /*设备号*/
    int major;      /*主设备号*/
    int minor;      /*此设备号*/
    struct class *led_class;    /*类*/
    struct device *led_dev;    /*设备*/
    struct cdev cdev;   
};

struct newchrled_dev newchrled;

#define PLATFORM "platformled"
#define PLATFORM_CNT 1

#define LEDOFF 	0  	/*关闭*/
#define LEDON 	1	/*打开*/

/*地址映射后的虚拟地址指针*/
static void __iomem *IMX6U_CCm_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static void LED_Switch(u8 sta)
{
	u32 val;
	if(sta == LEDOFF)
	{
		val = readl(GPIO1_DR);
		val |= (1 << 3);			/*关闭LED灯*/
		writel(val, GPIO1_DR);
	}
	else if (sta == LEDON)
	{
		val = readl(GPIO1_DR);
		val &= ~(1 << 3);			/*bit3清零，打开LED灯*/
		writel(val, GPIO1_DR);
	}
	
} 

static int newled_open(struct inode *inode, struct file *file)
{
	// printk("chrdevbase_open\r\n");
    file->private_data = &newchrled;
	return 0;
}

static int newled_release(struct inode *inode, struct file *file)
{
    struct newchrled_dev *dev = (struct newchrled_dev *)file->private_data;
	return 0;
}

static ssize_t newled_write(struct file *file, const char __user *user_buf,
			       size_t length, loff_t *offset)
{
    u8 databuf;
    int ret;
    ret = copy_from_user(&databuf, user_buf, length);
    if(ret < 0)
    {
        printk("write failed\r\n");
    }

    LED_Switch(databuf);

	return 0;
}

static const struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
	.open = newled_open,
	.write = newled_write,
	.release = newled_release,

};


static int led_probe(struct platform_device *dev)
{
    struct resource *ledsource[5];
    int i;
    int ret;
    u32 val = 0;
    // printk("led driver probe\r\n");
    /*初始化led，字符设备驱动*/
    /*1.从设备中获取资源*/
    for(i = 0; i<5; i++)
    {
        ledsource[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
        if(ledsource[i] == NULL)
        {
            return -EINVAL;
        }
    }
    
    /* 1.初始化LED灯,地址映射 */
	IMX6U_CCm_CCGR1 = ioremap(ledsource[0]->start, resource_size(ledsource[0]));
	SW_MUX_GPIO1_IO03 = ioremap(ledsource[1]->start, resource_size(ledsource[1]));
	SW_PAD_GPIO1_IO03 = ioremap(ledsource[2]->start, resource_size(ledsource[2]));
	GPIO1_DR = ioremap(ledsource[3]->start, resource_size(ledsource[3]));
	GPIO1_GDIR = ioremap(ledsource[4]->start, resource_size(ledsource[4]));

	/*2.GPIO初始化*/
	val = readl(IMX6U_CCm_CCGR1); 
	val &= ~(3 << 26);		/*先清除以前的配置*/
	val |= 3 << 26;
	writel(val, IMX6U_CCm_CCGR1);

	writel(0x5, SW_MUX_GPIO1_IO03);		/*设置复用*/
	writel(0x10B0, SW_PAD_GPIO1_IO03);	/*设置电气属性*/

	val = readl(GPIO1_GDIR); 
	val |= 1 << 3;						/*bit3置1，设置为输出*/
	writel(val, GPIO1_GDIR);

	val = readl(GPIO1_DR);
	val &= ~(1 << 3);			/*bit3清零，打开LED灯*/
	writel(val, GPIO1_DR);

    /*2.注册字符设备号*/
    if(newchrled.major) /*给定了主设备号*/
    {
        newchrled.devid = MKDEV(newchrled.major, 0);
        ret = register_chrdev_region(newchrled.devid, PLATFORM_CNT, PLATFORM);
    }
    else    /*没给定主设备号*/
    {
        ret = alloc_chrdev_region(&(newchrled.devid), 0, PLATFORM_CNT, PLATFORM);
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
    }

    if(ret < 0)
    {
        printk("newchrled chrdev_region err!\r\n");
        return -1;
    }
    printk("newchrled major = %d, minor = %d\r\n", newchrled.major, newchrled.minor);

    /*3.添加字符设备*/
    // void cdev_init(struct cdev *cdev, const struct file_operations *fops)
    // int cdev_add(struct cdev *p, dev_t dev, unsigned count)
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&(newchrled.cdev), &newchrled_fops);
    ret = cdev_add(&(newchrled.cdev), newchrled.devid, PLATFORM_CNT);

    /*4.自动创建设备节点*/
    /*4.1 先创建类*/
    newchrled.led_class = class_create(THIS_MODULE, PLATFORM);
    if(IS_ERR(newchrled.led_class))
    {
        return PTR_ERR(newchrled.led_class);
    }

    // struct device *device_create(struct class *class, 
    //     struct device *parent, 
    //     dev_t devt, 
    //     void *drvdata, 
    //     const char *fmt, ...) 

    /*4.2 创建设备*/
    newchrled.led_dev = device_create(newchrled.led_class, NULL, newchrled.devid, NULL, PLATFORM);
    if(IS_ERR(newchrled.led_dev))
    {
        return PTR_ERR(newchrled.led_dev);
    }

    return 0;
}


static int led_remove(struct platform_device *dev)
{
    u32 val;

	val = readl(GPIO1_DR);
	val |= (1 << 3);			/*关闭LED灯*/
	writel(val, GPIO1_DR);

	/*1.取消地址映射*/
	iounmap(IMX6U_CCm_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);


    /*删除字符设备*/
    // void cdev_del(struct cdev *p)
    cdev_del(&(newchrled.cdev));
    /*删除注册的设备号*/
    unregister_chrdev_region(newchrled.devid, PLATFORM_CNT);
    printk("newchrled exit\r\n");
    /*删除设备*/
    device_destroy(newchrled.led_class, newchrled.devid);
    /*删除类*/
    class_destroy(newchrled.led_class);
    return 0;
}

struct platform_driver leddriver = {
    .driver = {
        .name = "plat-led",     /*驱动名字，用于和设备匹配*/
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
    return platform_driver_register(&leddriver);
}

/**
 * 卸载驱动
*/
static void __exit leddriver_exit(void)
{
    platform_driver_unregister(&leddriver);
}


module_init(leddriver_init);
module_exit(leddriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
