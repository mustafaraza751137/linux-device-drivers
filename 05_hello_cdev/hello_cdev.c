#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

static int major;

static ssize_t my_read (struct file *f, char __user *u, size_t l, loff_t *o){
    pr_info("%s: Read is called.\n", __func__);
    return 0;
}

static struct file_operations fops = {
    .read = my_read,
};

static int __init hello_init(void) {
    pr_info("%s++\n", __func__);
    major = register_chrdev(0, "hello_cdev", &fops);
    if(major < 0){
        pr_err("%s: Error while registering char device\n", __func__);
        return major;
    }
    pr_info("%s: Major device number: %d\n", __func__, major);
    return 0;
}


static void __exit hello_exit(void) {
    unregister_chrdev(major, "hello_cdev");
    pr_info("%s--\n", __func__);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("A Simple Hello World Linux Device Driver");