#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

static int major;
static char text[64];

static ssize_t my_read (struct file *filep, char __user *user_buf, size_t len, loff_t *off){
    int not_copied, delta, to_copy = (len + *off) < sizeof(text) ? len : (sizeof(text) - *off);

    pr_info("%s: Read is called, we want to read %ld bytes, but actually only copying %d bytes. The Offset is %lld\n", __func__, len, to_copy, *off);

    if(*off >= sizeof(text))
        return 0;

    not_copied = copy_to_user(user_buf, &text[*off], to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
        pr_warn("%s: Could only copy %d bytes\n", __func__, delta);

    *off += delta;

    return delta;
}

static ssize_t my_write(struct file *filep, const char __user *user_buf, size_t len, loff_t *off){
    int not_copied, delta, to_copy = (len + *off) < sizeof(text) ? len : (sizeof(text) - *off);

    pr_info("%s: Write is called, we want to write %ld bytes, but actually only copying %d bytes. The Offset is %lld\n", __func__, len, to_copy, *off);

    if(*off >= sizeof(text))
        return 0;

    not_copied = copy_from_user(&text[*off], user_buf, to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
        pr_warn("%s: Could only copy %d bytes\n", __func__, delta);

    *off += delta;

    return delta;
} 

static struct file_operations fops = {
    .read = my_read,
    .write = my_write,
};

static int __init hello_init(void) {
    pr_info("%s++\n", __func__);
    major = register_chrdev(0, "read_write_cdev", &fops);
    if(major < 0){
        pr_err("%s: Error while registering char device\n", __func__);
        return major;
    }
    pr_info("%s: Major device number: %d\n", __func__, major);
    return 0;
}


static void __exit hello_exit(void) {
    unregister_chrdev(major, "read_write_cdev");
    pr_info("%s--\n", __func__);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("A Simple Hello World Linux Device Driver");