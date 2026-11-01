# 03_gpio — Basic GPIO Output Driver (gpiod + gpio-sim)

A simple Linux kernel module that controls an LED pin (line 4) on a simulated GPIO chip using the modern descriptor API (`gpiod`).

Because this runs inside a VirtualBox VM with no physical GPIOs, the controller is simulated using the kernel's `gpio-sim` module. The driver locates the pin using a **GPIO Lookup Table** (`struct gpiod_lookup_table`), eliminating the need for Device Tree or ACPI.

---

## Files

| File | Purpose |
|---|---|
| `gpioctrl.c` | Driver source code |
| `Makefile` | Builds `gpioctrl.ko` |
| `../gpio-sim-setup.sh` | Sets up and destroys the simulated GPIO controller |

---

## Quick Start (Step-by-Step)

### 1. Create the Simulated Chip (Once per boot)
```bash
sudo ./gpio-sim-setup.sh
```
Verify the chip is active (8 lines on `gpio-sim.0:node0`):
```bash
sudo gpiodetect
# expected: gpiochip0 [gpio-sim.0:node0] (8 lines)

sudo gpioinfo
# expected: gpiochip0 - 8 lines: ... line 4 shows up once the driver claims it
```

### 2. Build the Driver
```bash
make
```

### 3. Load the Module
```bash
sudo insmod gpioctrl.ko
```

### 4. Verify

* **Kernel Logs (`dmesg`):**
  ```bash
  sudo dmesg | tail -n 10
  # Expected:
  #   gpio_init++
  #   gpio_init: LED on
  ```

* **GPIO Line State (`gpioinfo`):**
  ```bash
  sudo gpioinfo gpiochip0
  # Expected:
  #   line   4: unnamed  "led"  output  active-high [used]
  ```

* **Simulated Hardware State (sysfs):**
  ```bash
  cat /sys/devices/platform/gpio-sim.0/gpiochip0/sim_gpio4/value
  # Expected: 1
  ```

### 5. Unload the Module
```bash
sudo rmmod gpioctrl
sudo dmesg | tail -n 10
# Expected:
#   gpio_exit: LED off
#   gpio_exit--
```

### 6. Cleanup Chip (When finished)
```bash
sudo ../gpio-sim-setup.sh --cleanup
```

---

## Key Technical Details

1. **GPIO Lookup Table (`led_gpio_table`)**:
   Maps the hardware chip label (`gpio-sim.0:node0`) and line offset (`4`) to the logical name `"led"`. This allows `gpiod_get(NULL, "led", ...)` to locate the pin without Device Tree.

2. **Why `gpiod_set_value_cansleep()`**:
   `gpio-sim` uses mutexes internally, meaning operations may sleep (`chip->can_sleep = true`). Calling standard `gpiod_set_value()` causes a kernel `WARN_ON` warning.