#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include "ioctl.h"

static dev_t dev_nr;
static struct cdev my_cdev;
static struct class *my_class;

static int ans = 42;

static long int my_ioctl(struct file *f, unsigned int cmd, unsigned long args){
    int status;
    struct mystruct s;
    pr_info("%s: ioctl called with cmd: 0x%x and args: %p\n", __func__, cmd, (void *)args);

    switch(cmd){
        case WR_VAL:
            status = copy_from_user(&ans, (int *) args, sizeof(int));
            if(status){
                pr_err("%s: Error on WR_VAL\n", __func__);
            }
            break;
        case RD_VAL:
            status = copy_to_user((int *) args, &ans, sizeof(int));
            if(status){
                pr_err("%s: Error on RD_VAL\n", __func__);
            }
            break;
        case GREET:
            status = copy_from_user(&s, (struct mystruct *) args, sizeof(struct mystruct));
            if(status){
                pr_err("%s: Error on GREET\n", __func__);
            }
            for(int i=0; i<s.repeat; i++)
                pr_info("%s: Hello %s\n", __func__, s.name);
            break;
        default:
            return -EOPNOTSUPP;
    }
    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = my_ioctl,
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