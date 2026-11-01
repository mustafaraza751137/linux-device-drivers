# 05_hello_cdev — Basic Character Device Driver (register_chrdev + mknod)

A simple Linux character device driver that registers a major number using the classic `register_chrdev()` API, implements a basic `read` file operation, and is accessed through manually created `/dev` nodes using `mknod`.

---

### 1. Find the Assigned Major Number
The driver requests a dynamic major number from the kernel (`register_chrdev(0, ...)`). Check which number was allocated:

* **Via `dmesg`:**
  ```bash
  sudo dmesg | tail -n 10
  # Output:
  #   hello_init++
  #   hello_init++: Major device number: 239
  ```

* **Via `/proc/devices`:**
  ```bash
  cat /proc/devices | grep hello_cdev
  # Output:
  #   239 hello_cdev
  ```
*(In this example, the allocated major number is **`239`**).*

### 4. Create Device Node(s) Using `mknod`
Create device files in `/dev` matching the major number (`239`):

```bash
# Create device with minor number 0:
sudo mknod /dev/hello0 c 239 0

# Create device with minor number 128:
sudo mknod /dev/hello128 c 239 128
```

Verify the created device nodes:
```bash
ls -l /dev/hello*
# Output:
#   crw-r--r-- 1 root root 239,   0 Oct 20 13:37 /dev/hello0
#   crw-r--r-- 1 root root 239, 128 Oct 20 13:40 /dev/hello128
```
*(Notice **`239, 0`** and **`239, 128`**: `239` is the Major number routing to your driver, and `0`/`128` are Minor numbers).*

### 5. Test the Driver
Read from the device using `cat`:
```bash
sudo cat /dev/hello0
sudo cat /dev/hello128
```

Verify in kernel logs that `my_read` was invoked:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_read: Read is called.
#   my_read: Read is called.
```

### 6. Unload and Cleanup
When finished, remove the kernel module and delete the manually created `/dev` nodes:

```bash
# Unload kernel module:
sudo rmmod hello_cdev

# Remove device files from /dev:
sudo rm -f /dev/hello0 /dev/hello128

# Confirm clean removal:
sudo dmesg | tail -n 5
# Output:
#   hello_exit--
```

---

## Key Technical Details

1. **`register_chrdev(0, "hello_cdev", &fops)`**:
   - Passing `0` tells the kernel to dynamically allocate an unused major number.
   - Automatically allocates a range of **256 minor numbers** (0 to 255) for this major number.

2. **`mknod <name> c <major> <minor>`**:
   - Creates a special character device (`c`) inode in the filesystem.
   - The filesystem uses the `major` number to find the registered `file_operations` table (`fops`), and passes the `minor` number to the driver to distinguish between different sub-devices.

3. **`my_read()` returning `0`**:
   - In Linux read system calls, returning `0` signifies **EOF (End of File)**.
   - This allows commands like `cat` to trigger the read operation and exit immediately without hanging or looping infinitely.

