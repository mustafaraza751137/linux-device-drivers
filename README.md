# Linux Device Drivers

A hands-on collection of Linux kernel loadable modules (LKMs) and character device drivers developed step-by-step on Ubuntu / x86_64 Linux.

---

## Repository Overview

| Directory | Topic | Key Concepts |
|---|---|---|
| [`01_hello`](01_hello/) | Hello World Module | `module_init`, `module_exit`, `pr_info`, Kernel Makefiles |
| [`03_gpio`](03_gpio/) | Simulated GPIO Driver | `gpio-sim`, `gpiod_lookup_table`, `gpiod_set_value_cansleep` |
| [`05_hello_cdev`](05_hello_cdev/) | Basic Character Device | `register_chrdev`, `mknod`, Major numbers, basic `.read` |
| [`07_open_cdev`](07_open_cdev/) | Device Open & Close | `.open`, `.release`, `struct inode`, `struct file`, `f_flags`, `f_mode` |
| [`08_read_write_cdev`](08_read_write_cdev/) | Reading & Writing | `.read`, `.write`, `copy_to_user`, `copy_from_user`, `loff_t *off` |
| [`09_manual_cdev`](09_manual_cdev/) | Modern `cdev` Registration | `alloc_chrdev_region`, `cdev_init`, `cdev_add`, `dev_t` |
| [`10_auto_devfile`](10_auto_devfile/) | Automatic `/dev` Creation | `class_create`, `device_create`, sysfs, uevent, udev daemon |
| [`11_kmalloc`](11_kmalloc/) | Dynamic Memory Allocation | `kmalloc`, `kfree`, `filep->private_data`, `lseek` (`default_llseek`) |

---

## Prerequisites & Building

### Kernel Headers & Build Tools
Ensure build tools and the kernel headers matching your running kernel are installed:
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

### Building a Module
Navigate to any driver folder and run `make`:
```bash
cd 10_auto_devfile
make
sudo insmod auto_devfile.ko
sudo dmesg | tail -n 10
sudo rmmod auto_devfile
make clean
```

