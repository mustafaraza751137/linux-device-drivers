#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>

static struct platform_device *my_pdev;

/* Name all the compatible devices */
static const struct of_device_id my_dev_ids[] = {
    { .compatible = "brightlight,mydev" },
    {}, /* Empty Element -> end of the list */
};
MODULE_DEVICE_TABLE(of, my_dev_ids);

/* Implement a probe and a remove function */
static int my_dev_probe(struct platform_device *pdev){
    pr_info("%s: Probe function called\n", __func__);
    return 0;
}

static void my_dev_remove(struct platform_device *pdev){
    pr_info("%s: Remove function called\n", __func__);
}

/* Bundle compatible devices, probe and remove in driver's struct */
static struct platform_driver my_dev_driver = {
    .probe = my_dev_probe,
    .remove = my_dev_remove,
    .driver = {
        .name = "my_dev_driver",
        .of_match_table = my_dev_ids,
    }
};

/* Register the driver at the OS */
#if 1
static int __init hello_init(void) {
    int ret;
    pr_info("%s: Init function is called\n", __func__);
    ret = platform_driver_register(&my_dev_driver);
    if(ret)
        return ret;

    my_pdev = platform_device_register_simple("my_dev_driver", -1, NULL, 0);
    if(IS_ERR(my_pdev)) {
        platform_driver_unregister(&my_dev_driver);
        return PTR_ERR(my_pdev);
    }

    return 0;
}

static void __exit hello_exit(void) {
    pr_info("%s: Exit function is called\n", __func__);
    platform_device_unregister(my_pdev);     /* Triggers my_dev_remove() */
    platform_driver_unregister(&my_dev_driver);
}

module_init(hello_init);
module_exit(hello_exit);
#else
module_platform_driver(my_dev_driver);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("DT Driver for my device");