#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>

/* Two device instances for host testing (x86) */
static struct platform_device *my_pdev1;
static struct platform_device *my_pdev2;

/* Name all the compatible devices for Device Tree targets (ARM) */
static const struct of_device_id my_dev_ids[] = {
    { .compatible = "brightlight,mydev" },
    { }
};
MODULE_DEVICE_TABLE(of, my_dev_ids);

/* Probe function called for each matching device instance */
static int my_dev_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;

    dev_info(dev, "Probe function called\n");
    dev_warn(dev, "This is a warning\n");
    dev_err(dev, "This is an error: %d\n", 123);
    return 0;
}

static void my_dev_remove(struct platform_device *pdev) {
    dev_info(&pdev->dev, "Remove function called\n");
}

static struct platform_driver my_dev_driver = {
    .probe = my_dev_probe,
    .remove = my_dev_remove,
    .driver = {
        .name = "my_dev_driver",
        .of_match_table = my_dev_ids,
    }
};

static int __init hello_init(void) {
    int ret;

    ret = platform_driver_register(&my_dev_driver);
    if (ret) {
        pr_err("my_dev_driver: Failed to register driver: %d\n", ret);
        return ret;
    }

    /* On non-DT hosts (x86 VM), register 2 software devices to test multi-instance logging */
    if (!of_have_populated_dt()) {
        pr_info("my_dev_driver: Registering 2 software devices for host testing\n");

        /* Device 0 -> "my_dev_driver.0" */
        my_pdev1 = platform_device_register_simple("my_dev_driver", 0, NULL, 0);
        if (IS_ERR(my_pdev1)) {
            platform_driver_unregister(&my_dev_driver);
            return PTR_ERR(my_pdev1);
        }

        /* Device 1 -> "my_dev_driver.1" */
        my_pdev2 = platform_device_register_simple("my_dev_driver", 1, NULL, 0);
        if (IS_ERR(my_pdev2)) {
            platform_device_unregister(my_pdev1);
            platform_driver_unregister(&my_dev_driver);
            return PTR_ERR(my_pdev2);
        }
    }

    return 0;
}

static void __exit hello_exit(void) {
    if (my_pdev2)
        platform_device_unregister(my_pdev2);
    if (my_pdev1)
        platform_device_unregister(my_pdev1);

    platform_driver_unregister(&my_dev_driver);
    pr_info("my_dev_driver: Driver unregistered\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("Platform driver demonstrating device logging across multiple instances");