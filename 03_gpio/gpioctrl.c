#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h> /* GPIO descriptor consumer API */
#include <linux/gpio/machine.h>  /* GPIO lookup table API */

static struct gpio_desc *led;

/* Maps hardware chip and pin to connection name "led" without Device Tree */
static struct gpiod_lookup_table led_gpio_table = {
    .dev_id = NULL, /* Standalone module; not bound to a device */
    .table = {
        /* Chip label, pin number, connection name, active high */
        GPIO_LOOKUP("gpio-sim.0:node0", 4, "led", GPIO_ACTIVE_HIGH),
        { }, /* End of table marker */
    },
};

static int __init gpio_init(void)
{
    pr_info("%s++\n", __func__);

    /* Register lookup table with GPIO subsystem */
    gpiod_add_lookup_table(&led_gpio_table);

    /* Request GPIO "led" as output initialized to LOW */
    led = gpiod_get(NULL, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led)) {
        pr_err("%s: failed to request GPIO: %ld\n", __func__, PTR_ERR(led));
        gpiod_remove_lookup_table(&led_gpio_table);
        return PTR_ERR(led); /* Return error code (e.g. -ENOENT = -2) */
    }

    /* Turn LED ON; _cansleep is used because gpio-sim can sleep (uses mutexes) */
    gpiod_set_value_cansleep(led, 1);
    pr_info("%s: LED on\n", __func__);
    return 0;
}

static void __exit gpio_exit(void)
{
    if (led) {
        gpiod_set_value_cansleep(led, 0); /* Turn LED OFF */
        pr_info("%s: LED off\n", __func__);
        gpiod_put(led);                   /* Free the GPIO */
    }
    gpiod_remove_lookup_table(&led_gpio_table); /* Remove lookup table */
    pr_info("%s--\n", __func__);
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Basic gpio driver");
MODULE_AUTHOR("Mustafa Raza");