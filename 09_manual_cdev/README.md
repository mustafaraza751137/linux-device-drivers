# 09_manual_cdev — Modern Character Device Registration (`cdev` + `alloc_chrdev_region`)

A character device driver demonstrating the modern Linux kernel interface for registering character devices using `alloc_chrdev_region()`, `cdev_init()`, and `cdev_add()`, instead of the legacy `register_chrdev()` API.

---

## What This Driver Does

In earlier chapters (`05_hello_cdev`, `07_open_cdev`, `08_read_write_cdev`), we used the legacy function:
```c
register_chrdev(0, "name", &fops);
```
While simple, `register_chrdev()` is considered obsolete because it bundles two distinct jobs together and wastes resources: it automatically reserves all 256 minor numbers (0–255) for that major number.

This driver demonstrates the **modern, professional Linux approach**, which separates the process into two distinct, flexible steps:
1. **Device Number Allocation**: Reserving a range of major and minor numbers (`dev_t`) via `alloc_chrdev_region()`.
2. **Device Object Registration**: Creating and linking the character device object (`struct cdev`) to its `file_operations` table via `cdev_init()` and `cdev_add()`.

---

## Files

| File | Purpose |
|---|---|
| `manual_cdev.c` | Driver source code |
| `Makefile` | Builds `manual_cdev.ko` |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Build the Driver
```bash
make
```
Produces `manual_cdev.ko`.

### 2. Load the Kernel Module
```bash
sudo insmod manual_cdev.ko
```

### 3. Find the Assigned Major Number
Check the allocated major number in `/proc/devices`:
```bash
grep manual_cdev /proc/devices
# Output:
#   239 manual_cdev
```
*(In this run, the dynamically allocated major number is **`239`**).*

You can also check `dmesg`:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_init++
#   my_init: Register a character device for Major 239 staring with Minor 0
```

### 4. Create Device Node(s) with `mknod`
Create device files in `/dev` matching the major number (`239`):
```bash
sudo mknod /dev/hello0 c 239 0
sudo mknod /dev/hello100 c 239 100
```

Verify the device files:
```bash
ls -l /dev/hello*
# Output:
#   crw-r--r-- 1 root root 239,   0 Oct 31 23:20 /dev/hello0
#   crw-r--r-- 1 root root 239, 100 Oct 31 23:20 /dev/hello100
```

### 5. Test the Driver
Read from the device using `cat`:
```bash
cat /dev/hello0
cat /dev/hello100
```

Verify in kernel logs that `my_read` was invoked:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_read: Read is called.
#   my_read: Read is called.
```

### 6. Unload and Cleanup
```bash
# Unload module:
sudo rmmod manual_cdev

# Verify removal from /proc/devices:
grep manual_cdev /proc/devices
# (Outputs nothing, confirming clean unregistration)

# Remove device files:
sudo rm -f /dev/hello*

# Clean build artifacts:
make clean
```

---

## Key Technical Details

### 1. The 4 Arguments of `alloc_chrdev_region()`
```c
int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name);
```
* **`dev_t *dev`**: Output parameter where the kernel writes the allocated major/minor number pair.
* **`unsigned baseminor`**: The starting minor number requested (usually `0`).
* **`unsigned count`**: Number of contiguous minor numbers to reserve (`MINORMASK + 1`).
* **`const char *name`**: Name that appears in `/proc/devices` (`"manual_cdev"`).

> [!NOTE]
> Passing fewer than 4 arguments (e.g. omitting `baseminor`) causes the compiler error:
> `too few arguments to function 'alloc_chrdev_region'; expected 4, have 3`.

### 2. `cdev_init()` and `cdev_add()`
Once the device numbers (`dev_t`) are reserved, the driver prepares the actual device:
* **`cdev_init(&my_cdev, &fops)`**: Initializes the `struct cdev` and connects it with the `file_operations` callback structure (`fops`).
* **`my_cdev.owner = THIS_MODULE`**: Sets module ownership to prevent unloading while in use.
* **`cdev_add(&my_cdev, dev_nr, count)`**: Tells the VFS (Virtual File System) that this `cdev` is live and ready to handle operations for the assigned device numbers.

### 3. Cleanup Sequence (`cdev_del` + `unregister_chrdev_region`)
Cleanup must be performed in exact reverse order:
1. `cdev_del(&my_cdev)`: Unregisters the device from VFS first (prevents new operations).
2. `unregister_chrdev_region(dev_nr, count)`: Releases the allocated device numbers back to the kernel.
