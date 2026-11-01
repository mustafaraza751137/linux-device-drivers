# 11_kmalloc — Dynamic Memory Allocation & Per-Open Private Data

A Linux character device driver and user-space test program demonstrating how to dynamically allocate kernel memory using `kmalloc()`, assign it to a process's file handle via `filep->private_data`, handle repositioning with `lseek()`, and safely free memory with `kfree()`.

---

## What This Driver Does

In earlier chapters (`08_read_write_cdev`), data was stored in a fixed global array (`static char text[64]`). That approach had major limitations:
* Memory was permanently tied up even when no one was using the device.
* Multiple processes opening the device simultaneously would overwrite each other's data (race conditions).

This driver implements a **standard production kernel pattern**:
1. **Per-Open Buffer Allocation**: When a process calls `open()`, `kmalloc(MEMSIZE, GFP_KERNEL)` allocates a private 64-byte buffer stored in `filep->private_data`.
2. **Private Session Isolation**: Each open file descriptor gets its own independent buffer.
3. **Seek Support (`.llseek = default_llseek`)**: Allows user-space to rewind or reposition the file offset pointer using `lseek()`.
4. **Automatic Memory Cleanup**: When the process calls `close()`, `my_release()` calls `kfree(filep->private_data)` to prevent kernel memory leaks.

---

## Files

| File | Purpose |
|---|---|
| `kmalloc.c` | Kernel module allocating per-open memory with `kmalloc` / `kfree` |
| `test.c` | User-space test app opening `/dev/hello0`, writing, seeking, and reading back |
| `Makefile` | Builds both `kmalloc.ko` and `cdev_test`, and cleans both |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Build Driver and Test Program
```bash
make
```
*(Builds `kmalloc.ko` and compiles `test.c` into `cdev_test`).*

### 2. Load the Kernel Module
```bash
sudo insmod kmalloc.ko
```

Check `dmesg` to verify device registration and automatic creation:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_init++
#   my_init: Register a character device for Major 239 staring with Minor 0
#   my_init: Created devices under /sys/class/my_class/hello0
```

Verify that `/dev/hello0` was automatically created by udev:
```bash
ls -l /dev/hello0
# Output:
#   crw------- 1 root root 239, 0 Nov  2 00:36 /dev/hello0
```

### 3. Run the User-Space Test Program
Run `cdev_test`:
```bash
sudo ./cdev_test
# Output:
#   Hello World
```

### 4. Inspect Kernel Logs (`dmesg`)
```bash
sudo dmesg | tail -n 5
# Output:
#   my_write: Write is called, we want to write 11 bytes, but actually only copying 11 bytes. The Offset is 0
#   my_read: Read is called, we want to read 64 bytes, but actually only copying 64 bytes. The Offset is 0
```

### 5. Understanding `cdev_test` vs `cat`

* **Why `sudo ./cdev_test` works:**
  `test.c` opens `/dev/hello0`, writes `"Hello World"`, calls `lseek(fd, 0, SEEK_SET)` to rewind the file position to 0, and reads back from the **same open file session** before closing.
* **Why `echo` followed by `cat` is empty:**
  When `echo` closes the device, `my_release()` immediately calls `kfree()`, freeing that buffer. When `cat` opens the device next, `my_open()` allocates a **brand-new, blank buffer**. Each independent open gets its own fresh memory.

### 6. Unload and Cleanup
```bash
# Unload module:
sudo rmmod kmalloc

# Confirm removal:
sudo dmesg | tail -n 5
# Output:
#   my_exit--

# Clean build artifacts and test binary:
make clean
```

---

## Key Technical Details

### 1. `kmalloc()` & `GFP_KERNEL`
```c
filep->private_data = kmalloc(MEMSIZE, GFP_KERNEL);
```
* **`kmalloc()`**: Allocates physically contiguous memory in kernel space (analogous to user-space `malloc()`).
* **`GFP_KERNEL`**: Standard allocation flag for normal kernel execution context (allows the allocator to sleep/wait if memory is temporarily low).

### 2. `filep->private_data`
* A generic `void *` pointer provided by the kernel inside `struct file`.
* Drivers use it to bind instance-specific data (buffers, device states) to that open file descriptor throughout its lifecycle.

### 3. `kfree()`
```c
kfree(filep->private_data);
```
* Frees the kernel memory previously allocated by `kmalloc()`. Must be called in `my_release()` to avoid kernel memory leaks.

### 4. `default_llseek`
```c
static struct file_operations fops = {
    ...
    .llseek = default_llseek,
};
```
* Built-in kernel helper handling `lseek()` requests (`SEEK_SET`, `SEEK_CUR`, `SEEK_END`), allowing user applications to rewind or seek within the device buffer.
