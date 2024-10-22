#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>

#define NEWCHRLEDNAME "newcheled"
#define NEWCHRLED_CNT 1

/*寄存器物理地址*/
#define CCM_CCGR1_BASE				(0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE		(0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE		(0x020E02F4)
#define GPIO1_DR_BASE				(0x0209C000)
#define GPIO1_GDIR_BASE				(0x0209C004)
 
/*地址映射后的虚拟地址指针*/
static void __iomem *IMX6U_CCm_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;
 
 
/*LED设备结构体*/
struct newchrled_dev{
    dev_t devid;    /*设备号*/
    int major;      /*主设备号*/
    int minor;      /*此设备号*/
    struct cdev cdev;   
};

struct newchrled_dev newchrled;

#define LEDOFF 	0  	/*关闭*/
#define LEDON 	1	/*打开*/


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
	return 0;
}

static int newled_release(struct inode *inode, struct file *file)
{
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


static  int __init newcharled_init(void)
{
    
    /*1. 初始化LED*/
    int ret;
    u32 val = 0;
    printk("newcharled_init\r\n");

    
	/* 1.初始化LED灯,地址映射 */
	IMX6U_CCm_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
	SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
	SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
	GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
	GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

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
        ret = register_chrdev_region(newchrled.devid, NEWCHRLED_CNT, NEWCHRLEDNAME);
    }
    else    /*没给定主设备号*/
    {
        ret = alloc_chrdev_region(&(newchrled.devid), 0, NEWCHRLED_CNT, NEWCHRLEDNAME);
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
    ret = cdev_add(&(newchrled.cdev), newchrled.devid, NEWCHRLED_CNT);

    return 0;
}
 
static  void __exit newcharled_exit(void)
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
    unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
    printk("newchrled exit\r\n");
}
 
/*注册驱动和卸载驱动*/
module_init(newcharled_init);
module_exit(newcharled_exit);
MODULE_LICENSE("GPPL");
MODULE_AUTHOR("JYH");

