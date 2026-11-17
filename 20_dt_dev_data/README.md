# 20_dt_dev_data — Platform Driver Private Data & Multi-Instance State

A Linux platform driver demonstrating **per-device private data allocation (`struct my_dev_data`)** using `devm_kzalloc()`, storing and retrieving instance state with `platform_set_drvdata()` / `platform_get_drvdata()`, and independently managing **multiple device instances**.

---

## Files

| File | Purpose |
|---|---|
| `my_dev_driver.c` | Driver managing private instance state (`devm_kzalloc`, `drvdata`) across 2 devices |
| `my_overlay.dts` | Device Tree overlay defining two instances (`my_device` and `my_device2`) with separate pins |
| `gpio-sim-setup.sh` | Shell script to instantiate/destroy the `gpio-sim` virtual chip on x86 |
| `Makefile` | Builds both the kernel module (`.ko`) and device tree blob (`.dtbo`) |
| `README.md` | Usage guide and verification instructions |

---

## Quick Start (Step-by-Step on VirtualBox)

### 1. Setup Virtual GPIO Chip
```bash
sudo ./gpio-sim-setup.sh
```
Verify the chip is active (8 lines on `gpio-sim.0:node0`):
```bash
sudo gpiodetect
# Expected: gpiochip0 [gpio-sim.0:node0] (8 lines)
```

### 2. Build the Driver & Overlay
```bash
make
```

### 3. Load the Driver
```bash
sudo dmesg -C
sudo insmod my_dev_driver.ko
sudo dmesg
```

**Expected output:**
```text
my_dev_driver: Registering lookup table and device for host testing
my_dev_driver my_dev_driver.0: LED turned ON
my_dev_driver my_dev_driver.0: Button is not pressed (raw value: 0)
my_dev_driver my_dev_driver.1: LED turned ON
my_dev_driver my_dev_driver.1: Button is not pressed (raw value: 0)
```
Both devices (`.0` and `.1`) probe independently with their own private descriptors.

### 4. Verify Per-Device State Isolation
Simulate a button press on **Device 0 only** (pin 1):
```bash
echo pull-up | sudo tee /sys/devices/platform/gpio-sim.0/gpiochip0/sim_gpio1/pull
cat /sys/devices/platform/gpio-sim.0/gpiochip0/sim_gpio1/value
# Output: 1
```

Reload the driver and verify that only Device 0 sees the button pressed while Device 1 remains unpressed:
```bash
sudo rmmod my_dev_driver
sudo dmesg -C
sudo insmod my_dev_driver.ko
sudo dmesg
```

**Expected output:**
```text
my_dev_driver my_dev_driver.0: LED turned ON
my_dev_driver my_dev_driver.0: Button is pressed (raw value: 1)
my_dev_driver my_dev_driver.1: LED turned ON
my_dev_driver my_dev_driver.1: Button is not pressed (raw value: 0)
```

### 5. Unload & Cleanup
```bash
sudo rmmod my_dev_driver
sudo dmesg
# Expected:
#   my_dev_driver my_dev_driver.1: LED turned OFF and released
#   my_dev_driver my_dev_driver.1: Button GPIO released
#   my_dev_driver my_dev_driver.0: LED turned OFF and released
#   my_dev_driver my_dev_driver.0: Button GPIO released
#   my_dev_driver: Driver unregistered

sudo ./gpio-sim-setup.sh --cleanup
make clean
```

---

## Key Technical Details

1. **Why Global Variables Fail**:
   In single-instance drivers, static globals (e.g. `static struct gpio_desc *led`) work. But when a driver binds to multiple devices, each probe overwrites the global pointer, corrupting earlier instances.

2. **Per-Device Private Data (`struct my_dev_data`)**:
   Device state is encapsulated in a dedicated struct and allocated per-device using `devm_kzalloc(dev, sizeof(...), GFP_KERNEL)`. Memory is automatically reclaimed if probe fails or when the device is unbound.

3. **Driver Data Attachment**:
   - `platform_set_drvdata(pdev, priv)` binds the private struct to the device.
   - `platform_get_drvdata(pdev)` retrieves it inside `my_dev_remove()` to release specific pins for that instance.

4. **Multi-Instance Host Mapping**:
   For host testing without Device Tree, each instance has its own `dev_id` (`my_dev_driver.0` mapped to pins 0/1; `my_dev_driver.1` mapped to pins 2/3), ensuring zero resource collision between devices.
