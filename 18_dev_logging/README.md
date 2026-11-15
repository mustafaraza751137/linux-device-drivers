# 18_dev_logging — Structured Kernel Device Logging

A Linux platform driver demonstrating structured, contextual logging using **`dev_info()`**, **`dev_warn()`**, and **`dev_err()`**, showing how the kernel distinguishes messages across **multiple device instances**.

---

## What This Chapter Covers

1. **Contextual Device Logging (`dev_*` macros)**:
   - In kernel drivers, using generic `pr_info()` or `printk()` is discouraged once a `struct device` is available.
   - `dev_info(dev, ...)`, `dev_warn(dev, ...)`, and `dev_err(dev, ...)` automatically prefix log messages with the **driver name** and **device instance name**:
     ```
     <driver_name> <device_name>: <message>
     ```

2. **The Multi-Instance Problem**:
   - In production systems, multiple physical chips or peripherals often bind to the exact same driver (e.g., two I2C sensors, multiple UARTs, multiple GPIO expanders).
   - If an error occurs and you only log `pr_err("Read failed\n")`, you cannot tell which physical device failed.
   - `dev_err(dev, "Read failed\n")` disambiguates the source immediately (e.g., `my_dev_driver my_dev_driver.0` vs. `my_dev_driver my_dev_driver.1`).

3. **Dual-Platform Multi-Device Architecture**:
   - **Embedded ARM (Device Tree)**: `my_overlay.dts` defines two nodes (`my_device` and `my_device2@12345678`). When the overlay is applied, the kernel creates both devices and probes each one.
   - **Host VM (x86 / VirtualBox)**: Using runtime detection (`of_have_populated_dt()`), the driver registers two software platform devices (`id = 0` and `id = 1`) to simulate multiple hardware instances without requiring physical hardware.

---

## Files

| File | Purpose |
|---|---|
| `my_dev_driver.c` | Platform driver implementing `dev_info`, `dev_warn`, `dev_err` across 2 device instances |
| `my_overlay.dts` | Device Tree overlay defining two device nodes (`my_device` and `my_device2@12345678`) |
| `Makefile` | Builds both the kernel module (`.ko`) and device tree blob (`.dtbo`) |
| `README.md` | Documentation and verification guide |

---

## `pr_*()` vs. `dev_*()` Comparison

| Feature | `pr_info()`, `pr_err()`, etc. | `dev_info()`, `dev_err()`, etc. |
|---|---|---|
| **Header** | `<linux/printk.h>` | `<linux/dev_printk.h>` (via `<linux/device.h>`) |
| **First Argument** | Format string | `struct device *dev = &pdev->dev;` |
| **Output Format** | `[ timestamp ] Message` | `[ timestamp ] <driver> <device>: Message` |
| **Context** | Global / Module-level | Specific hardware / device instance |
| **Upstream Standard** | Used in `init`/`exit` before devices exist | Mandatory inside `probe()`, `remove()`, and driver operations |

### Common `dev_*` Log Levels

| Function | Kernel Log Level | Standard Usage |
|---|---|---|
| `dev_err(dev, ...)` | `KERN_ERR` | Critical hardware failures, missing required resources |
| `dev_warn(dev, ...)` | `KERN_WARNING` | Non-fatal anomalies, fallback modes, degraded states |
| `dev_info(dev, ...)` | `KERN_INFO` | Successful probe, device initialization milestones |
| `dev_dbg(dev, ...)` | `KERN_DEBUG` | Detailed debug logs (enabled via dynamic debug or `DEBUG`) |

---

## How It Works on Both Platforms

```
                          ┌──────────────────────────┐
                          │   my_dev_driver.ko       │
                          │   .probe = my_dev_probe  │
                          └────────────┬─────────────┘
                                       │
                  ┌────────────────────┴────────────────────┐
                  │                                         │
       [ Embedded Target / ARM ]                 [ Host VM / x86_64 ]
         of_have_populated_dt() = true             of_have_populated_dt() = false
                  │                                         │
       Overlay: my_overlay.dtbo                  Software Registration:
       ├── Node 1: my_device                     ├── Instance 0: id = 0
       └── Node 2: my_device2@12345678           └── Instance 1: id = 1
                  │                                         │
       Kernel Probes:                            Kernel Probes:
       ├── my_dev_driver my_device               ├── my_dev_driver my_dev_driver.0
       └── my_dev_driver 12345678.my_device2     └── my_dev_driver my_dev_driver.1
```

---

## Build and Testing

### 1. Compile
```bash
make
```

### 2. Load Driver
```bash
# Clear previous logs
sudo dmesg -C

# Insert module (registers driver and both device instances)
sudo insmod my_dev_driver.ko

# Inspect logs
sudo dmesg
```

**Expected output:**
```text
my_dev_driver: Registering 2 software devices for host testing
my_dev_driver my_dev_driver.0: Probe function called
my_dev_driver my_dev_driver.0: This is a warning
my_dev_driver my_dev_driver.0: This is an error: 123
my_dev_driver my_dev_driver.1: Probe function called
my_dev_driver my_dev_driver.1: This is a warning
my_dev_driver my_dev_driver.1: This is an error: 123
```

Notice how the device instance (`.0` vs. `.1`) is automatically prefixed to every log line, making debugging multi-instance hardware completely unambiguous.

### 3. Unload Driver
```bash
sudo rmmod my_dev_driver
sudo dmesg
```

**Expected output:**
```text
my_dev_driver my_dev_driver.1: Remove function called
my_dev_driver my_dev_driver.0: Remove function called
my_dev_driver: Driver unregistered
```

### 4. Cleanup
```bash
make clean
```
