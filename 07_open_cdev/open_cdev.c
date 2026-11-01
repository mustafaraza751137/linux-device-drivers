#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

static int major;

static int my_open (struct inode *node, struct file *filep){
    pr_info("%s: Major: %d, Minor: %d\n", __func__, imajor(node), iminor(node));

    pr_info("%s: filep->f_pos: %lld", __func__, filep->f_pos);
    pr_info("%s: filep->f_mode: 0x%x", __func__, filep->f_mode);
    pr_info("%s: filep->f_flags: 0x%x", __func__, filep->f_flags);

    return 0;
}

static int my_release (struct inode *node, struct file *filep){
    pr_info("%s: File is closed\n", __func__);
    return 0;
}

static struct file_operations fops = {
    .open = my_open,
    .release = my_release,
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