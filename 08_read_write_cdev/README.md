# 08_read_write_cdev — Reading and Writing with Character Drivers

A Linux character device driver and user-space test program demonstrating how to implement **read** and **write** operations using a fixed-size kernel buffer (`char text[64]`), `copy_to_user()`, `copy_from_user()`, and offset (`loff_t *off`) tracking.

---

## Files

| File | Purpose |
|---|---|
| `read_write_cdev.c` | Kernel module implementing `.read` and `.write` callbacks |
| `test.c` | User-space test application reading from the device byte-by-byte |
| `Makefile` | Builds `read_write_cdev.ko` and `cdev_test`, and cleans both |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Build Driver and User Test Program
```bash
make
```
*(Builds both `read_write_cdev.ko` and the `cdev_test` binary).*

### 2. Load the Kernel Module
```bash
sudo insmod read_write_cdev.ko
```

Check the assigned major number in `/proc/devices`:
```bash
grep read_write /proc/devices
# Output:
#   239 read_write_cdev
```
*(In this run, the allocated major number is **`239`**).*

### 3. Create the Device Node
```bash
sudo mknod /dev/hello0 c 239 0
ls /dev | grep hello
# Output:
#   hello0
```

### 4. Write Data to the Device
Write a message to `/dev/hello0` using `tee`:
```bash
echo "Hello world!" | sudo tee /dev/hello0
# Output:
#   Hello world!
```

In `dmesg`, the driver confirms the 13-byte write (including `\n`):
```text
my_write: Write is called, we want to write 13 bytes, but actually only copying 13 bytes. The Offset is 0
```

### 5. Read Data from the Device
Run the user-space test program to read the stored message byte-by-byte:
```bash
sudo ./cdev_test /dev/hello0
# Output:
#   Hello world!
```

Or read using standard `cat`:
```bash
sudo cat /dev/hello0
# Output:
#   Hello world!
```

### 6. Inspect Kernel Logs (`dmesg`)
```bash
sudo dmesg | tail -n 20
```
When `cdev_test` reads byte-by-byte (`read(fd, &c, 1)`), each call copies 1 byte and increments `*off`:
```text
my_read: Read is called, we want to read 1 bytes, but actually only copying 1 bytes. The Offset is 0
my_read: Read is called, we want to read 1 bytes, but actually only copying 1 bytes. The Offset is 1
...
my_read: Read is called, we want to read 1 bytes, but actually only copying 1 bytes. The Offset is 63
my_read: Read is called, we want to read 1 bytes, but actually only copying 0 bytes. The Offset is 64
```
At offset `64`, the driver returns `0` (EOF), causing the read loop to terminate cleanly.

### 7. Unload and Cleanup
```bash
# Unload driver:
sudo rmmod read_write_cdev

# Remove device node:
sudo rm -f /dev/hello*

# Clean all build artifacts and binaries:
make clean
```

---

## Key Technical Details

### 1. User/Kernel Space Data Transfer
Kernel memory and user-space memory are strictly isolated. Data cannot be accessed via direct pointers:
* **`copy_from_user(kernel_dst, user_src, len)`**: Copies data from user-space buffer into kernel memory during `write`. Returns number of uncopied bytes (`0` on full success).
* **`copy_to_user(user_dst, kernel_src, len)`**: Copies data from kernel buffer into user-space memory during `read`.

### 2. Offset Tracking (`loff_t *off`) and EOF
* The kernel passes a pointer `*off` representing the current file position for that open file descriptor.
* After copying `delta` bytes, the driver advances the offset:
  ```c
  *off += delta;
  ```
* When `*off >= sizeof(text)`, the driver returns **`0`**:
  - In `read()`, returning `0` signals **EOF (End of File)**, telling `cat` or `cdev_test` that no more data remains.
  - In `write()`, returning `0` signals that the buffer is full and no more bytes can be written (e.g. writing >64 bytes causes `tee: failed to write whole buffer`).

### 3. Byte-by-Byte vs Block Reading
* **`cdev_test`**: Calls `read(fd, &c, 1)` in a loop, triggering 64 separate kernel `my_read` invocations, incrementing offset by 1 byte each time.
* **`cat`**: Requests 65,536 bytes at once (`len = 65536`), copying all available 64 bytes in a single call, followed by a second call returning `0` (EOF).
