#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>

#define CHARDEVBASE_MAJOR 200			//主设备号
#define CHARDEVNAME "chrdevbase"		//设备名

static int chrdev_open(struct inode *inode, struct file *file)
{
	printk("chrdevbase_open\r\n");
	return 0;
}

static int chrdev_close(struct inode *inode, struct file *file)
{
	printk("chrdevbase_close\r\n");
	return 0;
}

static ssize_t chrdev_read(struct file *file,	/* file descriptor */
				char __user *user_buf,	/* user buffer */
				size_t len,		/* length of buffer */
				loff_t *offset)		/* offset in the file */
{
	printk("chrdevbase_read\r\n");
	return 0;
}

static ssize_t chrdev_write(struct file *file, const char __user *user_buf,
			       size_t length, loff_t *offset)
{
	printk("chrdevbase_write\r\n");
	return 0;
}

static struct file_operations chardev_base = {
	.owner = THIS_MODULE,
	.open = chrdev_open,
	.release = chrdev_close,
	.read = chrdev_read,
	.write = chrdev_write,
};

static int __init chardevbase_init(void)
{
	int ret = 0;
	
	printk("chardevbase_init\r\n");

	//注册字符设备
	// static inline int register_chrdev(unsigned int major, const char *name, 
	// 				const struct file_operations *fops) 
	//第一个参数是主设备号,第二个参数是设备名，第三课参数是驱动要实现的结构体

	ret = register_chrdev(CHARDEVBASE_MAJOR, CHARDEVNAME, &chardev_base);
	if(ret < 0)
	{
		printk("chrdevbase_failed");
	}
	return 0;
}

static void __exit chardevbase_exit(void)
{
	printk("chardevbase_exit\r\n");

	//注销字符设备
	// static inline void unregister_chrdev(unsigned int major, const char *name)
	unregister_chrdev(CHARDEVBASE_MAJOR, CHARDEVNAME);
}


/**
 * 驱动模块的入口与出口
*/
module_init(chardevbase_init);  //入口，加载模块时执行
module_exit(chardevbase_exit);  //出口，卸载模块时执行

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYH");
