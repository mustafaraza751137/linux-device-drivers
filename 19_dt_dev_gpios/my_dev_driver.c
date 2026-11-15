#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/machine.h>

static struct platform_device *my_pdev;
static struct gpio_desc *led, *button;

/* GPIO Lookup Table for host testing (VirtualBox + gpio-sim) */
static struct gpiod_lookup_table my_gpio_table = {
    .dev_id = "my_dev_driver",
    .table = {
        GPIO_LOOKUP("gpio-sim.0:node0", 0, "led", GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP("gpio-sim.0:node0", 1, "button", GPIO_ACTIVE_HIGH),
        { }
    },
};

/* Device Tree match table for embedded targets (ARM) */
static const struct of_device_id my_dev_ids[] = {
    { .compatible = "brightlight,mydev" },
    { }
};
MODULE_DEVICE_TABLE(of, my_dev_ids);

static int my_dev_probe(struct platform_device *pdev) {
    int status;
    struct device *dev = &pdev->dev;

    /* 1. Request LED pin as output initialized to LOW */
    led = gpiod_get(dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led)) {
        status = PTR_ERR(led);
        dev_err(dev, "Failed to get 'led' GPIO: %d\n", status);
        return status;
    }

    /* 2. Request Button pin as input */
    button = gpiod_get(dev, "button", GPIOD_IN);
    if (IS_ERR(button)) {
        status = PTR_ERR(button);
        dev_err(dev, "Failed to get 'button' GPIO: %d\n", status);
        goto free_led;
    }

    /* 3. Turn LED ON (_cansleep avoids warnings with gpio-sim mutexes) */
    gpiod_set_value_cansleep(led, 1);
    dev_info(dev, "LED turned ON\n");

    /* 4. Read Button state */
    status = gpiod_get_value_cansleep(button);
    dev_info(dev, "Button is %spressed (raw value: %d)\n", status ? "" : "not ", status);

    return 0;

free_led:
    gpiod_put(led);
    return status;
}

static void my_dev_remove(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;

    /* Turn LED OFF and release both GPIOs */
    if (led) {
        gpiod_set_value_cansleep(led, 0);
        gpiod_put(led);
        dev_info(dev, "LED turned OFF and released\n");
    }
    if (button) {
        gpiod_put(button);
        dev_info(dev, "Button GPIO released\n");
    }
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

    /* On non-DT hosts (x86 VM), register lookup table and software device */
    if (!of_have_populated_dt()) {
        pr_info("my_dev_driver: Registering lookup table and device for host testing\n");
        gpiod_add_lookup_table(&my_gpio_table);

        my_pdev = platform_device_register_simple("my_dev_driver", -1, NULL, 0);
        if (IS_ERR(my_pdev)) {
            gpiod_remove_lookup_table(&my_gpio_table);
            platform_driver_unregister(&my_dev_driver);
            return PTR_ERR(my_pdev);
        }
    }

    return 0;
}

static void __exit hello_exit(void) {
    if (my_pdev) {
        platform_device_unregister(my_pdev);
        gpiod_remove_lookup_table(&my_gpio_table);
    }

    platform_driver_unregister(&my_dev_driver);
    pr_info("my_dev_driver: Driver unregistered\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("Platform driver controlling GPIOs via Device Tree and lookup tables");