#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#else
/* Needed only for host testing on x86 VirtualBox */
#include <linux/gpio/machine.h>
#endif

/*
 * =========================================================================
 * 1. VirtualBox / Host Testing Harness (x86 without Device Tree)
 * =========================================================================
 * Active only when CONFIG_OF is disabled. Simulates Device Tree hardware
 * nodes using kernel software platform devices and GPIO lookup tables.
 */
#ifndef CONFIG_OF
static struct platform_device *my_pdev1, *my_pdev2;

static struct gpiod_lookup_table pdev1_gpio_table = {
    .dev_id = "my_dev_driver.0",
    .table = {
        GPIO_LOOKUP("gpio-sim.0:node0", 0, "led", GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP("gpio-sim.0:node0", 1, "button", GPIO_ACTIVE_HIGH),
        { }
    },
};

static struct gpiod_lookup_table pdev2_gpio_table = {
    .dev_id = "my_dev_driver.1",
    .table = {
        GPIO_LOOKUP("gpio-sim.0:node0", 2, "led", GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP("gpio-sim.0:node0", 3, "button", GPIO_ACTIVE_HIGH),
        { }
    },
};
#endif

/*
 * =========================================================================
 * 2. Core Linux Platform Driver (Production Code for all platforms)
 * =========================================================================
 */
struct my_dev_data {
    struct gpio_desc *led;
    struct gpio_desc *button;
};

/* Device Tree match table for embedded targets (ARM) */
#ifdef CONFIG_OF
static const struct of_device_id my_dev_ids[] = {
    { .compatible = "brightlight,mydev" },
    { }
};
MODULE_DEVICE_TABLE(of, my_dev_ids);
#endif

static int my_dev_probe(struct platform_device *pdev) {
    struct my_dev_data *priv;
    int status;
    struct device *dev = &pdev->dev;

    priv = devm_kzalloc(dev, sizeof(struct my_dev_data), GFP_KERNEL);
    if (!priv) {
        dev_err(dev, "Out of memory\n");
        return -ENOMEM;
    }

    /* 1. Request LED pin as output initialized to LOW */
    priv->led = gpiod_get(dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(priv->led)) {
        status = PTR_ERR(priv->led);
        dev_err(dev, "Failed to get 'led' GPIO: %d\n", status);
        return status;
    }

    /* 2. Request Button pin as input */
    priv->button = gpiod_get(dev, "button", GPIOD_IN);
    if (IS_ERR(priv->button)) {
        status = PTR_ERR(priv->button);
        dev_err(dev, "Failed to get 'button' GPIO: %d\n", status);
        goto free_led;
    }

    /* 3. Turn LED ON (_cansleep avoids warnings with gpio-sim mutexes) */
    gpiod_set_value_cansleep(priv->led, 1);
    dev_info(dev, "LED turned ON\n");

    /* 4. Read Button state */
    status = gpiod_get_value_cansleep(priv->button);
    dev_info(dev, "Button is %spressed (raw value: %d)\n", status ? "" : "not ", status);

    platform_set_drvdata(pdev, priv);
    return 0;

free_led:
    gpiod_put(priv->led);
    return status;
}

static void my_dev_remove(struct platform_device *pdev) {
    struct my_dev_data *priv = platform_get_drvdata(pdev);
    struct device *dev = &pdev->dev;

    /* Turn LED OFF and release both GPIOs */
    if (priv->led) {
        gpiod_set_value_cansleep(priv->led, 0);
        gpiod_put(priv->led);
        dev_info(dev, "LED turned OFF and released\n");
    }
    if (priv->button) {
        gpiod_put(priv->button);
        dev_info(dev, "Button GPIO released\n");
    }
}

static struct platform_driver my_dev_driver = {
    .probe = my_dev_probe,
    .remove = my_dev_remove,
    .driver = {
        .name = "my_dev_driver",
#ifdef CONFIG_OF
        .of_match_table = my_dev_ids,
#endif
    }
};

/*
 * =========================================================================
 * 3. Driver Registration & Lifecycle
 * =========================================================================
 */
#ifdef CONFIG_OF
/*
 * Embedded Target (Raspberry Pi / ARM):
 * Hardware is described by Device Tree; the kernel handles device binding.
 * Single macro replaces all init/exit boilerplate.
 */
module_platform_driver(my_dev_driver);

#else
/*
 * VirtualBox / Host Testing (x86 without Device Tree):
 * Manually instantiate software devices and lookup tables.
 */
static int __init hello_init(void) {
    int ret;

    ret = platform_driver_register(&my_dev_driver);
    if (ret) {
        pr_err("my_dev_driver: Failed to register driver: %d\n", ret);
        return ret;
    }

    pr_info("my_dev_driver: Registering lookup tables and devices for host testing\n");
    gpiod_add_lookup_table(&pdev1_gpio_table);
    gpiod_add_lookup_table(&pdev2_gpio_table);

    my_pdev1 = platform_device_register_simple("my_dev_driver", 0, NULL, 0);
    if (IS_ERR(my_pdev1)) {
        gpiod_remove_lookup_table(&pdev1_gpio_table);
        gpiod_remove_lookup_table(&pdev2_gpio_table);
        platform_driver_unregister(&my_dev_driver);
        return PTR_ERR(my_pdev1);
    }

    my_pdev2 = platform_device_register_simple("my_dev_driver", 1, NULL, 0);
    if (IS_ERR(my_pdev2)) {
        platform_device_unregister(my_pdev1);
        gpiod_remove_lookup_table(&pdev2_gpio_table);
        gpiod_remove_lookup_table(&pdev1_gpio_table);
        platform_driver_unregister(&my_dev_driver);
        return PTR_ERR(my_pdev2);
    }

    return 0;
}

static void __exit hello_exit(void) {
    if (my_pdev2)
        platform_device_unregister(my_pdev2);
    if (my_pdev1)
        platform_device_unregister(my_pdev1);

    gpiod_remove_lookup_table(&pdev2_gpio_table);
    gpiod_remove_lookup_table(&pdev1_gpio_table);

    platform_driver_unregister(&my_dev_driver);
    pr_info("my_dev_driver: Driver unregistered\n");
}

module_init(hello_init);
module_exit(hello_exit);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("Platform driver managing per-device private data across multiple instances");
