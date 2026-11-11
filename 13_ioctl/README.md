# 13_ioctl — Advanced `ioctl` with Custom Commands and Struct Transfer

A character device driver and user-space test application demonstrating how to define standardized `ioctl` commands using `_IOW` and `_IOR`, transfer scalar values and complex structs across the user/kernel boundary, and handle unsupported commands.

---

## What This Driver Does

In chapter `12_simple_ioctl`, we only inspected command numbers and pointer addresses. This driver demonstrates **practical, bidirectional communication via `ioctl`**:
1. **Standard `ioctl` Command Definitions**: Uses the kernel macros `_IOW` (write to kernel) and `_IOR` (read from kernel) with unique sequence numbers in [`ioctl.h`](ioctl.h).
2. **Reading Kernel State (`RD_VAL`)**: Uses `copy_to_user()` to return the internal integer variable `ans` (default `42`) to user-space.
3. **Writing Kernel State (`WR_VAL`)**: Uses `copy_from_user()` to overwrite the driver's internal variable with a value passed from user-space.
4. **Passing Complex Structs (`GREET`)**: Copies a multi-field `struct mystruct` (`repeat` count and `name` string) into the kernel and loops printing greetings to `dmesg`.
5. **Error Handling (`-EOPNOTSUPP`)**: Returns standard Linux error `-EOPNOTSUPP` (Operation not supported) whenever an unknown command is received.

---

## Files

| File | Purpose |
|---|---|
| `ioctl.c` | Kernel module implementing `my_ioctl` using `copy_to_user` and `copy_from_user` |
| `ioctl.h` | Shared header defining `struct mystruct` and `ioctl` command macros (`WR_VAL`, `RD_VAL`, `GREET`) |
| `test.c` | User-space test application demonstrating read, write, struct passing, and error handling |
| `Makefile` | Builds `ioctl.ko` and compiles `test.c` into `cdev_test` |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Build Driver and User Test Program
```bash
make
```
*(Builds `ioctl.ko` and compiles `test.c` into `cdev_test`).*

### 2. Load the Kernel Module
```bash
sudo insmod ioctl.ko
```

Verify that `/dev/hello0` was automatically created by udev:
```bash
ls /dev/hello0
# Output: /dev/hello0
```

---

### 3. Run Test Commands

#### A. Read Current Value (`RD_VAL`)
Running with no extra arguments executes `ioctl(fd, RD_VAL, &ans)`:
```bash
sudo ./cdev_test
# Output:
#   the answer is 42
```

#### B. Write a New Value (`WR_VAL`)
Passing 1 argument writes the integer into the kernel:
```bash
sudo ./cdev_test -12
```

Verify the new value was stored by reading it back:
```bash
sudo ./cdev_test
# Output:
#   the answer is -12
```

#### C. Send a Struct (`GREET`)
Passing 2 arguments populates `struct mystruct { repeat, name }` and sends it via `ioctl(fd, GREET, &s)`:
```bash
sudo ./cdev_test 4 Raza
```

Check `dmesg` to verify the driver received the struct and executed the loop:
```bash
sudo dmesg | tail -n 5
# Output:
#   my_ioctl: ioctl called with cmd: 0x40486163 and args: 0x7ffd...
#   my_ioctl: Hello Raza
#   my_ioctl: Hello Raza
#   my_ioctl: Hello Raza
#   my_ioctl: Hello Raza
```

#### D. Test Unsupported Commands
Passing more than 2 arguments sends an unrecognized command number (`123`):
```bash
sudo ./cdev_test 4 Raza 568 gg
# Output:
#   ioctl: Operation not supported
```
*(The driver hits `default: return -EOPNOTSUPP;`, which user-space catches as `errno = EOPNOTSUPP`).*

---

### 4. Unload and Cleanup
```bash
# Unload kernel module:
sudo rmmod ioctl

# Clean build artifacts and test binary:
make clean
```

---

## Key Technical Details

### 1. Anatomy of `_IOW` and `_IOR` Macros
Defined in [`ioctl.h`](ioctl.h):
```c
#define WR_VAL _IOW('a', 'a', int *)
#define RD_VAL _IOR('a', 'b', int *)
#define GREET  _IOW('a', 'c', struct mystruct *)
```
* **Direction**:
  - `_IOR`: Read from kernel $\rightarrow$ user space.
  - `_IOW`: Write from user space $\rightarrow$ kernel.
* **Type (Magic Number)**: `'a'` identifies the subsystem/driver.
* **Sequence Number (`nr`)**: Must be **unique** for each command (`'a'`, `'b'`, `'c'`). If two commands share the same sequence number, direction, and size, they produce duplicate values and trigger a compiler error.
* **Data Type**: Determines the byte size encoded into the 32-bit ioctl command number.

### 2. Passing Structures Across User/Kernel Boundary
Pointers passed via `ioctl` point to user-space virtual memory and cannot be dereferenced directly in kernel space:
```c
case GREET:
    status = copy_from_user(&s, (struct mystruct *) args, sizeof(struct mystruct));
    if (status) {
        pr_err("%s: Error on GREET\n", __func__);
    }
    for (int i = 0; i < s.repeat; i++)
        pr_info("%s: Hello %s\n", __func__, s.name);
    break;
```
`copy_from_user()` safely transfers the entire struct into a local kernel stack variable `s`.
