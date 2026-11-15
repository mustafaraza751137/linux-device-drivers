#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/property.h>

static struct platform_device *my_pdev;

/* 
 * Software properties used for fallback testing on non-DT hosts (e.g. x86).
 * On embedded targets with Device Tree (ARM/Raspberry Pi), the kernel
 * reads these directly from the live Device Tree node / overlay instead.
 */
static const struct property_entry my_dev_props[] = {
    PROPERTY_ENTRY_U32("int_val", 123),
    PROPERTY_ENTRY_STRING("string_val", "Hello, DT!"),
    { }
};

static const struct platform_device_info my_pdev_info = {
    .name = "my_dev_driver",
    .id = -1,
    .properties = my_dev_props,
};

/* Device Tree match table for embedded targets */
static const struct of_device_id my_dev_ids[] = {
    { .compatible = "brightlight,mydev" },
    { }
};
MODULE_DEVICE_TABLE(of, my_dev_ids);

/*
 * Probe function:
 * Uses the Unified Device Property API (device_property_*).
 * Works identically on BOTH architectures:
 * - On ARM/Device Tree: reads properties from DT node (of_node)
 * - On x86/Software Node: reads properties from software property_entry
 */
static int my_dev_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    const char *string_val;
    u32 int_val;
    int ret;

    dev_info(dev, "Probe called successfully\n");

    /* Read integer property */
    ret = device_property_read_u32(dev, "int_val", &int_val);
    if (ret) {
        dev_err(dev, "Failed to read 'int_val' property: %d\n", ret);
        return ret;
    }
    dev_info(dev, "int_val = %u\n", int_val);

    /* Read string property */
    ret = device_property_read_string(dev, "string_val", &string_val);
    if (ret) {
        dev_err(dev, "Failed to read 'string_val' property: %d\n", ret);
        return ret;
    }
    dev_info(dev, "string_val = %s\n", string_val);

    return 0;
}

static void my_dev_remove(struct platform_device *pdev) {
    dev_info(&pdev->dev, "Remove called\n");
}

/* Platform driver structure */
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

    /* Instantiate software device with properties on non-DT hosts (e.g. x86) */
    if (!of_have_populated_dt()) {
        pr_info("my_dev_driver: Registering software device for host testing\n");
        my_pdev = platform_device_register_full(&my_pdev_info);
        if (IS_ERR(my_pdev)) {
            pr_err("my_dev_driver: Failed to create test device: %ld\n", PTR_ERR(my_pdev));
            platform_driver_unregister(&my_dev_driver);
            return PTR_ERR(my_pdev);
        }
    }

    return 0;
}

static void __exit hello_exit(void) {
    if (my_pdev)
        platform_device_unregister(my_pdev);

    platform_driver_unregister(&my_dev_driver);
    pr_info("my_dev_driver: Driver unregistered\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("Platform Driver reading device properties");
