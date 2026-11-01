# 10_auto_devfile — Automatic Device File Creation (`class_create` + `device_create` + `udev`)

A character device driver demonstrating how to **automatically create device nodes in `/dev`** using the Linux device model (`sysfs` and `udev`), completely eliminating the need for manual `mknod` commands.

---

## What This Driver Does

In chapter `09_manual_cdev`, after loading the driver with `insmod`, you had to manually find the major number and run:
```bash
sudo mknod /dev/hello0 c 239 0   # Manual creation
```

This driver introduces **automatic device node creation**:
1. **`class_create("my_class")`**: Creates a device class directory in sysfs at `/sys/class/my_class/`.
2. **`device_create(my_class, NULL, dev_nr, NULL, "hello%d", 0)`**: Populates device metadata under `/sys/class/my_class/hello0/` including the `dev` file containing `239:0` (Major:Minor).
3. **Kernel uevent & udev**: The kernel broadcasts a `uevent` announcing the new device. The user-space daemon **`udevd`** receives this event, reads the major/minor number from sysfs, and **automatically creates `/dev/hello0`**.
4. **Automatic Cleanup on `rmmod`**: Calling `device_destroy()` and `class_destroy()` causes `udevd` to automatically remove `/dev/hello0` from the filesystem without needing `rm`.

---

## Files

| File | Purpose |
|---|---|
| `auto_devfile.c` | Driver source code implementing `class_create` and `device_create` |
| `Makefile` | Builds `auto_devfile.ko` |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Build the Driver
```bash
make
```
Produces `auto_devfile.ko`.

### 2. (Optional) Watch uevents in a Separate Terminal
To see the kernel and udev events in real time as they happen:
```bash
udevadm monitor
```

### 3. Load the Kernel Module
```bash
sudo insmod auto_devfile.ko
```

If you ran `udevadm monitor`, you will observe the automatic creation events:
```text
KERNEL[1886.913341] add      /class/my_class (class)
KERNEL[1886.913902] add      /devices/virtual/my_class/hello0 (my_class)
KERNEL[1886.913990] add      /module/auto_devfile (module)
UDEV  [1887.097605] add      /class/my_class (class)
UDEV  [1887.097649] add      /module/auto_devfile (module)
UDEV  [1887.097691] add      /devices/virtual/my_class/hello0 (my_class)
```

In `dmesg`:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_init++
#   my_init: Register a character device for Major 239 staring with Minor 0
#   my_init: Created devices under /sys/class/my_class/hello0
```

### 4. Verify in sysfs
Inspect the newly created class and device entries:
```bash
# Check the class:
ls /sys/class/my_class
# Output: hello0

# Check the device attributes:
ls /sys/class/my_class/hello0
# Output: dev  power  subsystem  uevent

# Verify the Major:Minor mapping:
cat /sys/class/my_class/hello0/dev
# Output: 239:0
```

### 5. Verify the Automatic Device Node in `/dev`
Notice that `/dev/hello0` already exists without running `mknod`:
```bash
ls -l /dev/hello0
# Output:
#   crw------- 1 root root 239, 0 Nov  1 16:40 /dev/hello0
```

### 6. Test the Driver
Read from the device:
```bash
sudo cat /dev/hello0
```
> [!NOTE]
> Running plain `cat /dev/hello0` returns `Permission denied` because default udev permissions are `0600` (`crw-------`, owned by `root`). Using `sudo` allows access.

Check `dmesg` to confirm `my_read` was called:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_read: Read is called.
```

### 7. Unload and Verify Automatic Deletion
```bash
sudo rmmod auto_devfile
```

In `udevadm monitor`, you will see udev tear down the device nodes automatically:
```text
KERNEL[2285.704482] remove   /devices/virtual/my_class/hello0 (my_class)
KERNEL[2285.706208] remove   /class/my_class (class)
KERNEL[2285.706256] remove   /module/auto_devfile (module)
UDEV  [2285.714802] remove   /module/auto_devfile (module)
UDEV  [2285.714850] remove   /devices/virtual/my_class/hello0 (my_class)
UDEV  [2285.716781] remove   /class/my_class (class)
```

In `dmesg`:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_exit--
```

Verify that `/dev/hello0` and the sysfs entry are gone:
```bash
ls -l /dev/hello0
# Output: ls: cannot access '/dev/hello0': No such file or directory

ls /sys/class/my_class 2>/dev/null || echo "Class removed"
# Output: Class removed
```

Clean build artifacts:
```bash
make clean
```

---

## Key Technical Details

### 1. `class_create()`
```c
my_class = class_create("my_class");
```
* Registers a device class in sysfs at `/sys/class/my_class`.
* Groups related devices together under a common subsystem category.

### 2. `device_create()`
```c
device_create(my_class, NULL, dev_nr, NULL, "hello%d", 0);
```
* **`my_class`**: Pointer to the parent `struct class`.
* **`parent` (`NULL`)**: Optional parent device pointer.
* **`dev_t dev_nr`**: The major/minor pair (`239:0`).
* **`drvdata` (`NULL`)**: Private driver data pointer.
* **`fmt, ...` (`"hello%d", 0`)**: Formatted string used to name the sysfs directory and the resulting `/dev` node (`/dev/hello0`).

### 3. Cleanup Order in `my_exit`
Kernel resources must be destroyed in exact reverse order of creation:
1. `device_destroy(my_class, dev_nr)`: Destroys device node, informs udev to delete `/dev/hello0`.
2. `class_unregister(my_class)` & `class_destroy(my_class)`: Removes `/sys/class/my_class`.
3. `cdev_del(&my_cdev)`: Unregisters `cdev` from VFS.
4. `unregister_chrdev_region(dev_nr, count)`: Frees the reserved major/minor numbers.
