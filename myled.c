#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/io.h>

MODULE_AUTHOR("kato-masahiro");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

/*
At first,this program was written by Ryuichi Ueda.
And,cloned from "https://github.com/ryuichiueeda/robosys_device_drivers".
Then,it was revised by kato-masahiro.
*/

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;

static volatile u32 *gpio_base = NULL;

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos)
{
    int cnt = 0;
    int i,ii,iii;
    int max = 10000;
	char c;
	if(copy_from_user(&c,buf,sizeof(char)))
		return -EFAULT;

    if (c == '1')
    {
        for(i = 0 ; i < max ; ++i)
        {
            for (ii = 0;ii < i ; ++ii)
            {
                gpio_base[7] = 1 << 25; 
            }

            for(iii = 0; iii < (max-i) ; ++iii)
            {
                gpio_base[10] = 1 << 25;
            }
        }

        for(i = 0 ; i < max ; ++i)
        {
            for (ii = 0;ii < i ; ++ii)
            {
                gpio_base[10] = 1 << 25; 
            }

            for(iii = 0; iii < (max-i) ; ++iii)
            {
                gpio_base[7] = 1 << 25;
            }
        }
        gpio_base[10] = 1 << 25;
    }
    else if(c == '0')
    {
        gpio_base[10] = 1 << 25;
    }
        return 1;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write
};

static int __init init_mod(void)
{
	int retval;

	gpio_base = ioremap_nocache(0x3f200000, 0xA0); //0x3f..:base address, 0xA0: region to map

	const u32 led = 25;
	const u32 index = led/10;//GPFSEL2
	const u32 shift = (led%10)*3;//15bit
	const u32 mask = ~(0x7 << shift);//11111111111111000111111111111111
	gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift);//001: output flag
	//11111111111111001111111111111111
	
	retval =  alloc_chrdev_region(&dev, 0, 1, "myled");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));

	cdev_init(&cdv, &led_fops);
	cdv.owner = THIS_MODULE;
	retval = cdev_add(&cdv, dev, 1);
	if(retval < 0){
		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d",MAJOR(dev),MINOR(dev));
		return retval;
	}

	cls = class_create(THIS_MODULE,"myled");
	if(IS_ERR(cls)){
		printk(KERN_ERR "class_create failed.");
		return PTR_ERR(cls);
	}
	device_create(cls, NULL, dev, NULL, "myled%d",MINOR(dev));

	return 0;
}

static void __exit cleanup_mod(void)
{
	cdev_del(&cdv);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));
	iounmap(gpio_base);
}

module_init(init_mod);
module_exit(cleanup_mod);
