# 19_dt_dev_gpios — Device Tree GPIO Descriptors (`gpiod`)

A Linux platform driver demonstrating GPIO consumer control using the modern **GPIO Descriptor API (`gpiod`)**, binding pins from **Device Tree (`led-gpio`, `button-gpio`)** on embedded targets and falling back to a **`gpiod_lookup_table` with `gpio-sim`** on x86 virtual machines.

---

## Files

| File | Purpose |
|---|---|
| `my_dev_driver.c` | Platform driver requesting and controlling `led` and `button` GPIOs |
| `my_overlay.dts` | Device Tree overlay mapping `led-gpio` and `button-gpio` |
| `gpio-sim-setup.sh` | Shell script to instantiate/destroy the `gpio-sim` virtual chip on x86 |
| `Makefile` | Builds both the kernel module (`.ko`) and device tree blob (`.dtbo`) |
| `README.md` | Usage guide and test instructions |

---

## Quick Start (Step-by-Step on VirtualBox)

### 1. Setup Virtual GPIO Chip
```bash
sudo ./gpio-sim-setup.sh
```
Verify the chip is active:
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
my_dev_driver my_dev_driver: LED turned ON
my_dev_driver my_dev_driver: Button is not pressed (raw value: 0)
```

### 4. Verify GPIO Line Claims
```bash
sudo gpioinfo -c gpiochip0
```
Lines 0 and 1 show as claimed by the driver:
```text
line   0:      "led"   output  active-high [used]
line   1:   "button"    input  active-high [used]
```

### 5. Simulate Button Press via Pull
In `gpio-sim`, you simulate physical input by toggling the pin's electrical bias:

* **Press Button (Logic HIGH / 1)**:
  ```bash
  echo pull-up | sudo tee /sys/devices/platform/gpio-sim.0/gpiochip0/sim_gpio1/pull
  cat /sys/devices/platform/gpio-sim.0/gpiochip0/sim_gpio1/value
  # Output: 1
  ```

* **Release Button (Logic LOW / 0)**:
  ```bash
  echo pull-down | sudo tee /sys/devices/platform/gpio-sim.0/gpiochip0/sim_gpio1/pull
  cat /sys/devices/platform/gpio-sim.0/gpiochip0/sim_gpio1/value
  # Output: 0
  ```

### 6. Unload & Cleanup
```bash
sudo rmmod my_dev_driver
sudo dmesg
# Expected:
#   my_dev_driver my_dev_driver: LED turned OFF and released
#   my_dev_driver my_dev_driver: Button GPIO released
#   my_dev_driver: Driver unregistered

sudo ./gpio-sim-setup.sh --cleanup
make clean
```

---

## Key Technical Details

1. **Device Tree Property Resolution**:
   When calling `gpiod_get(dev, "led", ...)`, the kernel looks for `led-gpios` or `led-gpio` in the device node. On ARM, this resolves to `<&gpio 21 0>` directly from Device Tree.

2. **Host Fallback (`gpiod_lookup_table`)**:
   When `of_have_populated_dt()` is false (x86 VM), the driver dynamically registers a `gpiod_lookup_table` mapping `"my_dev_driver"` to virtual lines on `gpio-sim.0:node0`.

3. **Safe Sleeping Access**:
   `gpio-sim` uses mutexes internally, so the driver uses `gpiod_set_value_cansleep()` and `gpiod_get_value_cansleep()` to prevent kernel warnings.
