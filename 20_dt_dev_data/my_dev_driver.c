#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/machine.h>

static struct platform_device *my_pdev1, *my_pdev2;

/* GPIO Lookup Table for host testing (VirtualBox + gpio-sim) */
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

struct my_dev_data {
    struct gpio_desc *led;
    struct gpio_desc *button;
};

/* Device Tree match table for embedded targets (ARM) */
static const struct of_device_id my_dev_ids[] = {
    { .compatible = "brightlight,mydev" },
    { }
};
MODULE_DEVICE_TABLE(of, my_dev_ids);

static int my_dev_probe(struct platform_device *pdev) {
    struct my_dev_data *priv;
    int status;
    struct device *dev = &pdev->dev;

    priv = devm_kzalloc(dev, sizeof(struct my_dev_data), GFP_KERNEL);

    if(!priv) {
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
        gpiod_add_lookup_table(&pdev1_gpio_table);
        gpiod_add_lookup_table(&pdev2_gpio_table);

        my_pdev1 = platform_device_register_simple("my_dev_driver", 0, NULL, 0);
        if (IS_ERR(my_pdev1)) {
            gpiod_remove_lookup_table(&pdev1_gpio_table);
            platform_driver_unregister(&my_dev_driver);
            return PTR_ERR(my_pdev1);
        }

        my_pdev2 = platform_device_register_simple("my_dev_driver", 1, NULL, 0);
        if (IS_ERR(my_pdev2)) {
            platform_device_unregister(my_pdev1);
            gpiod_remove_lookup_table(&pdev1_gpio_table);
            gpiod_remove_lookup_table(&pdev2_gpio_table);
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
    if (!of_have_populated_dt()) {
        gpiod_remove_lookup_table(&pdev2_gpio_table);
        gpiod_remove_lookup_table(&pdev1_gpio_table);
    }

    platform_driver_unregister(&my_dev_driver);
    pr_info("my_dev_driver: Driver unregistered\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("Platform driver controlling GPIOs via Device Tree and lookup tables");