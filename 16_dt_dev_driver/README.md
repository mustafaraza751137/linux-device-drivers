# 16_dt_dev_driver — Device Tree & Platform Drivers

A Linux Platform Driver implementing `probe()` and `remove()` callbacks, demonstrating how the Linux device model matches drivers to devices using both **Device Tree (`of_match_table`)** and **Bus Name matching (`platform_device_register_simple`)**.

---

## What This Chapter Covers

1. **Linux Platform Bus (`platform_bus_type`)**:
   - In Linux, peripherals that cannot be dynamically detected across a physical bus (unlike USB or PCI) are called **Platform Devices** (e.g. SoC GPIO controllers, I2C controllers, memory-mapped devices).
   - The kernel separates **Devices** (hardware description) from **Drivers** (software logic).

2. **The Lifecycle of `probe()` and `remove()`**:
   - Drivers **never call `probe()` or `remove()` directly**.
   - When a platform device is registered, the kernel searches registered platform drivers for a match.
   - Upon finding a match, the kernel automatically calls `probe(struct platform_device *pdev)`.
   - When the device or driver is unregistered, the kernel automatically calls `remove(struct platform_device *pdev)`.

3. **Device Matching Mechanisms**:
   - **Device Tree**: Matching via `of_match_table` (e.g., `compatible = "brightlight,mydev"`).
   - **Name Match**: Fallback matching via `driver.name == device.name` (e.g., `"my_dev_driver"`).

---

## Files

| File | Purpose |
|---|---|
| `my_dev_driver.c` | Platform driver source with `probe`, `remove`, and self-registering device (Option B) |
| `my_overlay.dts` | Device tree overlay defining node with `compatible = "brightlight,mydev"` |
| `Makefile` | Builds both the kernel module (`.ko`) and device tree blob (`.dtbo`) |
| `README.md` | Complete explanation, troubleshooting, and architectural notes |

---

## Deep Dive: Why Did We Use Option B?

### 1. The Context (Johannes 4GNU_Linux Tutorial on Raspberry Pi)
In the tutorial, Johannes develops on a **Raspberry Pi** (an ARM board). On ARM:
1. The hardware is described by a base **Device Tree** (`/proc/device-tree/`).
2. When Johannes runs `sudo insmod my_dev_driver.ko`, the driver is registered with the kernel, but `probe()` is **not** called yet because no matching hardware exists.
3. Johannes then runs:
   ```bash
   sudo dtoverlay my_overlay.dtbo
   ```
4. This dynamically merges `my_overlay.dtbo` into the live Device Tree.
5. The kernel sees a new node with `compatible = "brightlight,mydev"`, matches it to `my_dev_ids[]` in `my_dev_driver`, and **invokes `my_dev_probe()`**.
6. When he removes the overlay (`sudo dtoverlay -r my_overlay`), the kernel **invokes `my_dev_remove()`**.

---

### 2. The Problem on x86 VirtualBox
On your development machine (Ubuntu running inside VirtualBox on x86_64):
* **x86 PCs use ACPI, not Device Tree**: Intel/AMD PCs enumerate hardware using ACPI tables and PCI buses; they do not have `/proc/device-tree/`.
* **`dtoverlay` does not exist**: `dtoverlay` is a proprietary utility provided by Raspberry Pi OS firmware (`libraspberrypi-bin`). It cannot run on standard Ubuntu x86.
* **The Result**: If you only called `platform_driver_register(&my_dev_driver)`, `probe()` would **never** be executed because no matching device could ever be injected via Device Tree!

---

### 3. The Solution: Option B (Software Platform Device)
To test and verify `probe()` and `remove()` without requiring physical ARM hardware, we implemented **Option B** in [my_dev_driver.c](file:///home/mustafa/Drivers/16_dt_dev_driver/my_dev_driver.c):

```c
static struct platform_device *my_pdev;

static int __init hello_init(void) {
    int ret;
    pr_info("%s: Init function is called\n", __func__);

    /* 1. Register the platform driver */
    ret = platform_driver_register(&my_dev_driver);
    if (ret)
        return ret;

    /* 2. Programmatically instantiate a platform device matching driver.name */
    my_pdev = platform_device_register_simple("my_dev_driver", -1, NULL, 0);
    if (IS_ERR(my_pdev)) {
        platform_driver_unregister(&my_dev_driver);
        return PTR_ERR(my_pdev);
    }

    return 0;
}

static void __exit hello_exit(void) {
    pr_info("%s: Exit function is called\n", __func__);
    /* Unregistering the device triggers my_dev_remove() */
    platform_device_unregister(my_pdev);
    /* Unregistering the driver */
    platform_driver_unregister(&my_dev_driver);
}
```

### How the Linux Platform Bus Resolves This:

```
[ hello_init() ]
       |
       v
1. platform_driver_register(&my_dev_driver)
       --> Driver registered with driver.name = "my_dev_driver"
       |
       v
2. platform_device_register_simple("my_dev_driver", -1, NULL, 0)
       --> Kernel creates platform_device with dev.name = "my_dev_driver"
       |
       v
3. Platform Bus matches: dev.name == driver.name ("my_dev_driver")
       |
       v
[ KERNEL CALLS: my_dev_probe(pdev) ]
```

When unloading (`sudo rmmod my_dev_driver`):
```
[ hello_exit() ]
       |
       v
1. platform_device_unregister(my_pdev)
       --> Kernel detects device removal from driver
       |
       v
[ KERNEL CALLS: my_dev_remove(pdev) ]
       |
       v
2. platform_driver_unregister(&my_dev_driver)
```

This guarantees 100% test coverage of your driver lifecycle directly on your x86 machine!

---

## Comparison: Raspberry Pi vs. x86 VirtualBox

| Feature | Embedded ARM (Raspberry Pi) | x86_64 VirtualBox (Our Environment) |
|---|---|---|
| **Hardware Description** | Device Tree (`.dts` / `.dtb`) | ACPI Tables / PCI Bus |
| **Overlay Loader** | `dtoverlay my_overlay.dtbo` | Software `platform_device_register_simple` |
| **Match Mechanism** | `of_match_table` (`compatible = "brightlight,mydev"`) | Bus Name (`"my_dev_driver"`) |
| **Probe Trigger** | Applied overlay in Device Tree | `platform_device_register_simple()` call |
| **Remove Trigger** | `dtoverlay -r my_overlay` | `platform_device_unregister()` call |

---

## Build and Testing

### 1. Compile the Module and Overlay
```bash
make
```

### 2. Test Driver Insertion and Probe Execution
```bash
# Clear kernel ring buffer
sudo dmesg -C

# Insert the module
sudo insmod my_dev_driver.ko

# Inspect kernel logs
sudo dmesg
```

**Expected output:**
```text
hello_init: Init function is called
my_dev_probe: Probe function called
```

### 3. Test Driver Removal and Remove Execution
```bash
# Remove the module
sudo rmmod my_dev_driver

# Inspect kernel logs
sudo dmesg
```

**Expected output:**
```text
hello_exit: Exit function is called
my_dev_remove: Remove function called
```

---

## Notes on the `compatible` String

- The format in `my_dev_ids[]` is `"<vendor>,<model>"` (e.g. `brightlight,mydev`).
- `brightlight` is a fictional vendor prefix used in the tutorial.
- On a Device Tree target, `my_dev_driver.c` and `my_overlay.dts` **must match identically**:
  ```dts
  compatible = "brightlight,mydev";
  ```
  matched with:
  ```c
  static const struct of_device_id my_dev_ids[] = {
      { .compatible = "brightlight,mydev" },
      {},
  };
  ```
- Because Option B operates at the bus-level matching name, the `.of_match_table` is preserved completely untouched, ready to be deployed to any real ARM/Device Tree board without code changes.
