/* Core header for loading and unloading Loadable Kernel Modules (LKMs) */
#include <linux/module.h>
/* Contains kernel types, functions, and print utilities (e.g. pr_info, pr_err) */
#include <linux/kernel.h>
/* Provides __init and __exit macros for module initialization and cleanup */
#include <linux/init.h>

static int __init hello_init(void) {
    pr_info("%s: Hello, World! Driver loaded successfully.\n", __func__);
    return 0;
}


static void __exit hello_exit(void) {
    pr_info("%s: Goodbye, World! Driver unloaded successfully.\n", __func__);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mustafa Raza");
MODULE_DESCRIPTION("A Simple Hello World Linux Device Driver");