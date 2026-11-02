# 12_simple_ioctl — Basic Device Control with `ioctl` (`.unlocked_ioctl`)

A character device driver and user-space test application demonstrating how to handle **device control commands** using the `ioctl()` system call and the driver's `.unlocked_ioctl` callback.

---

## What This Driver Does

While `read()` and `write()` are designed to transfer streams of data, **`ioctl` (Input/Output Control)** is used to send control commands and configuration parameters to a device (e.g. changing baud rates, ejecting media, configuring hardware registers, or querying device status).

In this driver:
1. **`.unlocked_ioctl` in `fops`**: Registers the `my_ioctl` callback to intercept `ioctl()` calls from user space.
2. **Command & Argument Delivery**: Demonstrates how user-space passes an integer command (`cmd`) and an arbitrary argument or pointer (`args`) directly to the kernel.
3. **Automatic Device Creation**: Reuses the sysfs/udev mechanism from earlier chapters to automatically create `/dev/hello0`.

---

## Files

| File | Purpose |
|---|---|
| `simple_ioctl.c` | Kernel module implementing the `.unlocked_ioctl` callback |
| `test.c` | User-space test application invoking `ioctl()` with various commands |
| `Makefile` | Builds `simple_ioctl.ko` and compiles `test.c` into `cdev_test` |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Build Driver and User Test Program
```bash
make
```
*(Builds `simple_ioctl.ko` and compiles `test.c` into `cdev_test`).*

### 2. Load the Kernel Module
```bash
sudo insmod simple_ioctl.ko
```

Verify that `/dev/hello0` was automatically created by udev:
```bash
ls /dev/hello0
# Output: /dev/hello0
```

Check `dmesg` for module initialization:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_init++
#   my_init: Register a character device for Major 239 staring with Minor 0
#   my_init: Created devices under /sys/class/my_class/hello0
```

### 3. Run the Test Application
```bash
sudo ./cdev_test
```
> [!NOTE]
> Running without `sudo` (`./cdev_test`) returns `Open: Permission denied` because `/dev/hello0` is created with default `0600` permissions (`root:root`).

**User-space Output:**
```text
ioctl returned: 0
ioctl returned: 0
ioctl returned: 0
```

### 4. Inspect Kernel Logs (`dmesg`)
```bash
sudo dmesg | tail -n 10
```

**Expected Kernel Output:**
```text
my_ioctl: ioctl called with cmd: 1 and args: 0000000000000000
my_ioctl: ioctl called with cmd: 1234 and args: 00000000eea6fee5
my_ioctl: ioctl called with cmd: 987654321 and args: 00000000310fcc9f
```

### 5. Unload and Cleanup
```bash
# Unload kernel module:
sudo rmmod simple_ioctl

# Confirm removal in dmesg:
sudo dmesg | tail -n 5
# Output:
#   my_exit--

# Clean build artifacts:
make clean
```

---

## Key Technical Details

### 1. The `ioctl` System Call & Callback Signature
* **User Space (`test.c`)**:
  ```c
  int ioctl(int fd, unsigned long request, ...);
  ```
* **Kernel Space (`simple_ioctl.c`)**:
  ```c
  static long int my_ioctl(struct file *f, unsigned int cmd, unsigned long args);
  ```
  - `struct file *f`: The open file instance.
  - `unsigned int cmd`: The command number sent by user space (e.g. `1`, `1234`, `987654321`).
  - `unsigned long args`: The argument passed from user space (can be a plain value or a pointer to a user-space variable).

### 2. Why `.unlocked_ioctl`?
In ancient Linux kernels (pre-2.6.36), drivers used `.ioctl`, which held the Big Kernel Lock (BKL). Modern Linux character drivers use **`.unlocked_ioctl`**, which runs concurrently without holding the BKL, providing much higher performance and scalability.

### 3. Why Addresses Look Like `00000000eea6fee5` (`%p` Hashing)
In `my_ioctl`:
```c
pr_info("... args: %p\n", (void *)args);
```
Notice that `args` prints a hashed value rather than raw pointer values. For security reasons, modern Linux kernels automatically hash pointer values printed with `%p` to prevent leaking real kernel or user memory addresses through `dmesg`.
