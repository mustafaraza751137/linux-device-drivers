# 07_open_cdev — Inspecting `open()` and `release()` in Character Drivers

A character device driver and user-space test program demonstrating how the Linux kernel handles `open()` and `close()` system calls, specifically inspecting `struct inode` (major/minor numbers) and `struct file` (flags, modes, and position).

---

## Files

| File | Purpose |
|---|---|
| `open_cdev.c` | Kernel module implementing `.open` and `.release` callbacks |
| `test.c` | User-space test application opening the device with different flags |
| `Makefile` | Builds `open_cdev.ko` and `cdev_test` |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Build Driver and User Test Program
```bash
make
```
*(Builds both the kernel module `open_cdev.ko` and the test application `cdev_test`).*

### 2. Load the Kernel Module
```bash
sudo insmod open_cdev.ko
```

Check the dynamically assigned major number:
```bash
sudo dmesg | tail -n 5
# Output:
#   hello_init++
#   hello_init: Major device number: 239
```
*(In this run, the allocated major number is **`239`**).*

### 3. Create Device Nodes with Different Minors
```bash
sudo mknod /dev/hello0 c 239 0
sudo mknod /dev/hello10 c 239 10
ls -l /dev/hello*
# Output:
#   crw-r--r-- 1 root root 239,  0 Oct 20 23:19 /dev/hello0
#   crw-r--r-- 1 root root 239, 10 Oct 20 23:19 /dev/hello10
```

### 4. Run the Test Application
The test program opens and closes the device 3 times with different flags:
1. `O_RDONLY`
2. `O_RDWR | O_SYNC`
3. `O_WRONLY | O_NONBLOCK`

Run against `/dev/hello0`:
```bash
sudo ./cdev_test /dev/hello0
```

Run against `/dev/hello10`:
```bash
sudo ./cdev_test /dev/hello10
```

### 5. Inspect Kernel Logs (`dmesg`)
```bash
sudo dmesg | tail -n 25
```

**Expected Output for `/dev/hello0`:**
```text
my_open: Major: 239, Minor: 0
my_open: filep->f_pos: 0
my_open: filep->f_mode: 0x400001d
my_open: filep->f_flags: 0x8000
my_release: File is closed
my_open: Major: 239, Minor: 0
my_open: filep->f_pos: 0
my_open: filep->f_mode: 0x400001f
my_open: filep->f_flags: 0x109002
my_release: File is closed
my_open: Major: 239, Minor: 0
my_open: filep->f_pos: 0
my_open: filep->f_mode: 0x400001e
my_open: filep->f_flags: 0x8801
my_release: File is closed
```

**Expected Output for `/dev/hello10`:**
```text
my_open: Major: 239, Minor: 10
...
my_release: File is closed
```
*(Notice **`Minor: 10`** is accurately detected when accessing `/dev/hello10`).*

### 6. Unload and Cleanup
```bash
# Unload driver:
sudo rmmod open_cdev

# Remove device files:
sudo rm -f /dev/hello*

# Clean build artifacts and cdev_test:
make clean

# Confirm clean removal:
sudo dmesg | tail -n 5
# Output:
#   hello_exit--
```

---

## Key Technical Details

### 1. `struct inode` vs `struct file`
* **`struct inode` (The File on Disk)**:
  Represents the file node in the filesystem. Contains fixed hardware identity:
  - `imajor(node)`: Driver major number (`239`).
  - `iminor(node)`: Device index (`0` vs `10`), allowing one driver to manage multiple devices.
* **`struct file` (The Open Instance)**:
  Created in kernel memory every time a process calls `open()`. Destroyed when `close()` is called (`my_release`). Multiple processes opening the same device get their own separate `struct file`.

### 2. What `f_flags` and `f_mode` Show
* **`filep->f_flags`**: Reflects the user-space open flags passed to `open(pathname, flags)`:
  - `O_RDONLY` -> `0x8000` (read flag with large-file support)
  - `O_RDWR | O_SYNC` -> `0x109002` (`0x2` for RDWR + sync flags)
  - `O_WRONLY | O_NONBLOCK` -> `0x8801` (`0x1` for WRONLY + non-blocking flag)
* **`filep->f_mode`**: Kernel's internal permission mask:
  - Contains `FMODE_READ` (`0x1`), `FMODE_WRITE` (`0x2`), and `FMODE_LSEEK`.
