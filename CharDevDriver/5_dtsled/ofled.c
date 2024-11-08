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

#define DEVNAME "dtsled"
#define DEVCNT 1

/*地址映射后的虚拟地址指针*/
static void __iomem *IMX6U_CCm_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

struct dtsled_dev{
    dev_t devid;
    
    struct cdev cdev;
    struct class *led_class;
    struct device *led_device;
    struct device_node *dev_nd;
    int major;
    int minor;

};
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

struct dtsled_dev dtsled;

int dtsledopen (struct inode *inode, struct file *file)
{
    file->private_data = &dtsled;
    return 0;
}

int dtsledrelease (struct inode *inode, struct file *file)
{
    struct dtsled_dev *dev = (struct dtsled_dev *)file->private_data;
    return 0;
}

ssize_t dtsledwrite (struct file *file, const char __user *user_buf,
			       size_t length, loff_t *offset)
{
    struct dtsled_dev *dev = (struct dtsled_dev *)file->private_data;
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

static const struct file_operations fop = {
    .owner = THIS_MODULE,
    .open = dtsledopen,
    .release = dtsledrelease,
    .write = dtsledwrite,
};



/*注册函数的出入口*/
static int __init dtsled_init(void)
{
    int ret = 0;
    const char *str = NULL;
    u32 regval[10];
    u32 val = 0;
    int i;
    
    /*1.申请设备号*/
    dtsled.major = 0;
    if(dtsled.major)
    {   
        //表示手动指定了主设备号
        dtsled.devid = MKDEV(dtsled.major, 0);
        ret = register_chrdev_region(dtsled.devid, DEVCNT, DEVNAME);
    }
    else
    {
        ret = alloc_chrdev_region(&dtsled.devid, 0, DEVCNT, DEVNAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
        printk("MAJOR=%d MINOR=%d\r\n", dtsled.major, dtsled.minor);
    }

    /*2.添加字符设备*/
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &fop);
    cdev_add(&dtsled.cdev, dtsled.devid, DEVCNT);

    //4自动创建设备节点
    /*4.1 创建类*/
    dtsled.led_class = class_create(THIS_MODULE, DEVNAME);

    /*4.2创建设备*/
    dtsled.led_device = device_create(dtsled.led_class, NULL, dtsled.devid, NULL, DEVNAME);
    
    /*获取设备数属性内容*/
    dtsled.dev_nd  = of_find_node_by_path("/alphaled");
    of_property_read_string(dtsled.dev_nd, "compatible", &str);
    printk("compatible=%s\r\n", str);
    of_property_read_string(dtsled.dev_nd, "status", &str);
    printk("status=%s\r\n", str);
#if 0  
    of_property_read_u32_array(dtsled.dev_nd, "reg", regval, 10);
    
    for(i = 0; i < 10; i++)
    {
        printk("reg[%d]=%d\r\n", i, regval[i]);
    }


    //LED灯初始化
    IMX6U_CCm_CCGR1 = ioremap(regval[0], regval[1]);
	SW_MUX_GPIO1_IO03 = ioremap(regval[2], regval[3]);
	SW_PAD_GPIO1_IO03 = ioremap(regval[4], regval[5]);
	GPIO1_DR = ioremap(regval[6], regval[7]);
	GPIO1_GDIR = ioremap(regval[8], regval[9]);
#endif

    IMX6U_CCm_CCGR1 = of_iomap(dtsled.dev_nd, 0);
	SW_MUX_GPIO1_IO03 = of_iomap(dtsled.dev_nd, 1);
	SW_PAD_GPIO1_IO03 = of_iomap(dtsled.dev_nd, 2);
	GPIO1_DR = of_iomap(dtsled.dev_nd, 3);
	GPIO1_GDIR = of_iomap(dtsled.dev_nd, 4);

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
    
    return ret;
}

// alphaled{
// 		#address-cells = <1>;
// 		#size-cells = <1>;
// 		compatible = "jyhmade";
// 		status = "okay";
// 		reg = < 0x020C406C 0x04 /*CCM_CCGR1_BASE*/
// 			0x020E0068 0x04 /*SW_MUX_GPIO1_IO03_BASE*/
// 			0x020E02F4 0x04 /*SW_PAD_GPIO1_IO03_BASE*/
// 			0x0209C000 0x04 /*GPIO1_DR_BASE*/
// 			0x0209C004 0x04>; /*GPIO1_GDIR_BASE*/
		
// 	};

static void __exit dtsled_exit(void)
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


    //删除设备
    device_destroy(dtsled.led_class, dtsled.devid);
    //删除类
    class_destroy(dtsled.led_class);
    //删除设备
    cdev_del(&dtsled.cdev);
    //释放设备号
    unregister_chrdev_region(dtsled.devid, DEVCNT);
}

/*函数的出入口*/
module_init(dtsled_init);
module_exit(dtsled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
