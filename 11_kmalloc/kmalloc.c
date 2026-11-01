#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>

static dev_t dev_nr;
static struct cdev my_cdev;
static struct class *my_class;

#define MEMSIZE 64

static int my_open (struct inode *node, struct file *filep){
    filep->private_data = kmalloc(MEMSIZE, GFP_KERNEL);
    if(!filep->private_data){
        pr_err("%s: Out of memory\n", __func__);
        return -ENOMEM;
    }
    return 0;
}

static int my_release (struct inode *node, struct file *filep){
    kfree(filep->private_data);
    return 0;
}

static ssize_t my_read (struct file *filep, char __user *user_buf, size_t len, loff_t *off){
    char *text = filep->private_data;
    int not_copied, delta, to_copy = (len + *off) < MEMSIZE ? len : (MEMSIZE - *off);

    pr_info("%s: Read is called, we want to read %ld bytes, but actually only copying %d bytes. The Offset is %lld\n", __func__, len, to_copy, *off);

    if(*off >= MEMSIZE)
        return 0;

    not_copied = copy_to_user(user_buf, &text[*off], to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
        pr_warn("%s: Could only copy %d bytes\n", __func__, delta);

    *off += delta;

    return delta;
}

static ssize_t my_write(struct file *filep, const char __user *user_buf, size_t len, loff_t *off){
    char *text = filep->private_data;
    int not_copied, delta, to_copy = (len + *off) < MEMSIZE ? len : (MEMSIZE - *off);

    pr_info("%s: Write is called, we want to write %ld bytes, but actually only copying %d bytes. The Offset is %lld\n", __func__, len, to_copy, *off);

    if(*off >= MEMSIZE)
        return 0;

    not_copied = copy_from_user(&text[*off], user_buf, to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
        pr_warn("%s: Could only copy %d bytes\n", __func__, delta);

    *off += delta;

    return delta;
} 

static struct file_operations fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .llseek = default_llseek,
};

static int __init my_init(void) {
    int status;
    pr_info("%s++\n", __func__);

#ifdef STATIC_DEVNR
    dev_nr = STATIC_DEVNR;
    status = register_chrdev_region(dev_nr, MINORMASK + 1, "manual_cdev");
#else
    status = alloc_chrdev_region(&dev_nr, 0, MINORMASK + 1, "manual_cdev");
#endif
    if(status){
        pr_err("%s: Error reserving the region of device numbers\n", __func__);
        return status;
    }

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    status = cdev_add(&my_cdev, dev_nr, MINORMASK + 1);
    if(status){
        pr_err("%s: Error adding cdev\n", __func__);
        goto free_devnr;
    }

    pr_info("%s: Register a character device for Major %d staring with Minor %d\n", __func__, MAJOR(dev_nr), MINOR(dev_nr));
    
    my_class = class_create("my_class");
    if(!my_class){
        pr_err("%s: Could not create class my_class\n", __func__);
        status = ENOMEM;
        goto delete_cdev;
    }

    if(!device_create(my_class, NULL, dev_nr, NULL, "hello%d", 0)){
        pr_err("%s: Could not create device hello0\n", __func__);
        status = ENOMEM;
        goto delete_class;
    }

    pr_info("%s: Created devices under /sys/class/my_class/hello0\n", __func__);
    return 0;

delete_class:
    class_unregister(my_class);
    class_destroy(my_class);

delete_cdev:
    cdev_del(&my_cdev);

free_devnr:
    unregister_chrdev_region(dev_nr, MINORMASK + 1);
    return status;
}

static void __exit my_exit(void) {
    device_destroy(my_class, dev_nr);
    class_unregister(my_class);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_nr, MINORMASK + 1);
    pr_info("%s--\n", __func__);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("A sample driver for manually registering a character device");