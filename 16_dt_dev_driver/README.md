# 16_dt_dev_driver — Device Tree & Platform Drivers

A Linux Platform Driver implementing `probe()` and `remove()` callbacks, demonstrating how the Linux device model binds drivers to devices using both **Device Tree (`of_match_table`)** and **Bus Name matching (`platform_device_register_simple`)**.

---

## What This Chapter Covers

1. **Linux Platform Bus (`platform_bus_type`)**:
   - In Linux, peripherals that cannot be dynamically enumerated across a physical self-describing bus (unlike USB or PCI) are registered as **Platform Devices** (e.g., SoC peripherals, memory-mapped devices, on-chip controllers).
   - The Linux device model cleanly separates **Device Description** (hardware resources and metadata) from **Driver Logic** (software execution).

2. **The Lifecycle of `probe()` and `remove()`**:
   - Device drivers **never call `probe()` or `remove()` directly**.
   - When a platform device is registered, the kernel searches registered platform drivers for a match.
   - Upon finding a match, the platform bus subsystem automatically invokes `probe(struct platform_device *pdev)`.
   - When the device or driver is unregistered, the platform bus subsystem automatically invokes `remove(struct platform_device *pdev)`.

3. **Device Matching Mechanisms**:
   - **Device Tree**: Matching via Open Firmware compatibility tables (`of_match_table`), e.g., `compatible = "brightlight,mydev"`.
   - **Driver Name Matching**: Matching via string equality between `driver.name` and `device.name` (e.g., `"my_dev_driver"`).

---

## Files

| File | Purpose |
|---|---|
| `my_dev_driver.c` | Platform driver source with `probe`, `remove`, and software device registration for host testing |
| `my_overlay.dts` | Device Tree overlay defining node with `compatible = "brightlight,mydev"` |
| `Makefile` | Builds both the kernel module (`.ko`) and device tree blob (`.dtbo`) |
| `README.md` | Architectural documentation and verification guide |

---

## Device Matching & Testing Strategy: Software Instantiation vs. Device Tree Overlay

### 1. Device Tree-Based Instantiation (Embedded Linux / ARM)
On embedded Linux systems (such as ARM or RISC-V targets):
1. Attached peripherals are described in a base **Device Tree** (`/proc/device-tree/`).
2. When the driver is inserted (`sudo insmod my_dev_driver.ko`), the driver is registered with the platform bus, but `probe()` is not called yet because no matching hardware node exists.
3. When a dynamic Device Tree Overlay is applied:
   ```bash
   sudo dtoverlay my_overlay.dtbo
   ```
4. The kernel creates a device node with `compatible = "brightlight,mydev"`, matches it to `my_dev_ids[]` in `my_dev_driver`, and **invokes `my_dev_probe()`**.
5. When the overlay is removed, the kernel **invokes `my_dev_remove()`**.

---

### 2. The Challenge on x86 / PC Platforms (ACPI vs. Device Tree)
On standard PC and virtual machine environments (such as Ubuntu on x86_64):
* **ACPI vs. Device Tree**: x86 systems detect hardware using ACPI tables and PCI enumeration rather than Open Firmware / Device Tree.
* **No Dynamic Overlay Loader**: Utilities like `dtoverlay` are embedded-platform-specific and do not exist on generic x86 Linux distributions.
* **The Result**: If a platform driver only relies on Device Tree matching, `probe()` will never execute on an x86 host without target hardware attached.

---

### 3. The Alternative Method: Software-Instantiated Platform Device
To thoroughly test the complete driver lifecycle (`probe` and `remove`) directly on a host development machine, we implement software platform device instantiation inside [my_dev_driver.c](file:///home/mustafa/Drivers/16_dt_dev_driver/my_dev_driver.c):

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
    /* Unregistering the platform driver */
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
       --> Kernel instantiates platform_device with dev.name = "my_dev_driver"
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
       --> Kernel detects device removal from active driver
       |
       v
[ KERNEL CALLS: my_dev_remove(pdev) ]
       |
       v
2. platform_driver_unregister(&my_dev_driver)
```

This approach allows complete verification of resource allocation, driver binding, and probe/remove teardown logic directly on the development workstation.

---

## Comparison: Embedded ARM Target vs. x86 Host Environment

| Feature | Embedded ARM Target | x86 Host / VM Environment |
|---|---|---|
| **Hardware Description** | Device Tree (`.dts` / `.dtb`) | ACPI Tables / PCI Bus |
| **Device Instantiation** | Overlay Loader (`dtoverlay my_overlay.dtbo`) | Software Registration (`platform_device_register_simple`) |
| **Matching Mechanism** | `of_match_table` (`compatible = "brightlight,mydev"`) | Bus Name (`"my_dev_driver"`) |
| **Probe Trigger** | Applied overlay node in Device Tree | `platform_device_register_simple()` call |
| **Remove Trigger** | Overlay unloading (`dtoverlay -r`) | `platform_device_unregister()` call |

---

## Build and Testing

### 1. Compile the Module and Overlay
```bash
make
```

### 2. Test Driver Insertion and Probe Execution
```bash
# Clear kernel log buffer
sudo dmesg -C

# Insert the module
sudo insmod my_dev_driver.ko

# Inspect kernel logs
sudo dmesg
```

**Expected kernel output:**
```text
hello_init: Init function is called
my_dev_probe: Probe function called
```

### 3. Test Driver Removal and Teardown
```bash
# Remove the module
sudo rmmod my_dev_driver

# Inspect kernel logs
sudo dmesg
```

**Expected kernel output:**
```text
hello_exit: Exit function is called
my_dev_remove: Remove function called
```

---

## Notes on the `compatible` String

- The format in `my_dev_ids[]` follows the official Device Tree convention:
  $$\text{"<vendor>,<model>"}$$
- `"brightlight,mydev"` uses `brightlight` as an example custom vendor prefix adhering to the standard naming schema defined in kernel Device Tree bindings.
- On a Device Tree target, `my_dev_driver.c` and `my_overlay.dts` must match identically:
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
- Because software platform device registration matches by driver name (`"my_dev_driver"`), the `.of_match_table` is preserved completely intact, allowing the exact same driver to be deployed to an embedded Device Tree target without code modifications.
