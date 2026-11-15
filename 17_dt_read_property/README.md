# 17_dt_read_property — Reading Device Properties from Platform Drivers

A platform driver that reads device properties (`u32` and `string`) using the **Unified Device Property API** (`device_property_read_*`), with automatic environment detection to work on both Device Tree targets and x86 host machines.

---

## What This Chapter Covers

1. **Unified Device Property API** (`<linux/property.h>`):
   - `device_property_read_u32()` — reads a 32-bit unsigned integer property.
   - `device_property_read_string()` — reads a null-terminated string property.
   - These functions are **firmware-agnostic**: they work identically whether the properties come from a Device Tree node, ACPI, or a software node.

2. **Software Properties** (`struct property_entry`):
   - On non-DT hosts (x86), properties can be attached to a platform device via `PROPERTY_ENTRY_U32()` and `PROPERTY_ENTRY_STRING()` macros.
   - `platform_device_register_full()` accepts a `struct platform_device_info` containing these software properties.

3. **Automatic Environment Detection**:
   - `of_have_populated_dt()` checks at runtime whether a live Device Tree has been populated by the bootloader.
   - On DT targets (ARM): the driver waits for the overlay / DT node to trigger `probe()`.
   - On non-DT hosts (x86): the driver self-instantiates a software platform device with attached properties.

---

## Files

| File | Purpose |
|---|---|
| `my_dev_driver.c` | Platform driver reading `int_val` and `string_val` properties via `device_property_*` |
| `my_overlay.dts` | Device Tree overlay defining property values for embedded targets |
| `Makefile` | Builds both the kernel module (`.ko`) and device tree blob (`.dtbo`) |
| `README.md` | Documentation and usage guide |

---

## How It Works

### Property Flow on Each Platform

```
┌─────────────────────────────────────────────────────────────┐
│                    my_dev_probe(pdev)                        │
│         device_property_read_u32(dev, "int_val", ...)       │
│         device_property_read_string(dev, "string_val", ...) │
└──────────────────────┬──────────────────────────────────────┘
                       │
            ┌──────────┴──────────┐
            │                     │
   ┌────────▼────────┐  ┌────────▼────────┐
   │  ARM / DT Host  │  │  x86 / PC Host  │
   │                 │  │                 │
   │  Properties     │  │  Properties     │
   │  from of_node   │  │  from software  │
   │  (my_overlay)   │  │  property_entry │
   └─────────────────┘  └─────────────────┘
```

The `device_property_*` API abstracts the property source. The `probe()` function is **identical on both platforms** — no `#ifdef` required.

### Software Properties Definition (for x86 testing)

```c
static const struct property_entry my_dev_props[] = {
    PROPERTY_ENTRY_U32("int_val", 123),
    PROPERTY_ENTRY_STRING("string_val", "Hello, DT!"),
    { }
};
```

These mirror the values in `my_overlay.dts`:

```dts
int_val = <123>;
string_val = "Hello, DT!";
```

---

## `device_property_*` vs `of_property_*`

| API | Header | Works On |
|---|---|---|
| **`device_property_*`** (used here) | `<linux/property.h>` | Device Tree, ACPI, and Software Nodes |
| `of_property_*` (legacy) | `<linux/of.h>` | Device Tree only |

The `device_property_*` API is the **modern recommended approach** for new drivers.

---

## Build and Testing

### 1. Compile
```bash
make
```

### 2. Load and Verify
```bash
sudo dmesg -C
sudo insmod my_dev_driver.ko
sudo dmesg
```

**Expected kernel output:**
```text
my_dev_driver: Registering software device for host testing
my_dev_driver my_dev_driver: Probe called successfully
my_dev_driver my_dev_driver: int_val = 123
my_dev_driver my_dev_driver: string_val = Hello, DT!
```

### 3. Unload
```bash
sudo rmmod my_dev_driver
sudo dmesg
```

**Expected kernel output:**
```text
my_dev_driver my_dev_driver: Remove called
my_dev_driver: Driver unregistered
```

### 4. Cleanup
```bash
make clean
```

---

## Key Takeaways

- `device_property_read_*` already checks if the property exists — calling `device_property_present()` beforehand is redundant.
- Always use `dev_info()` / `dev_err()` instead of `pr_info()` inside `probe()` and `remove()` — they automatically prefix logs with the driver and device name.
- Return proper kernel error codes (e.g., the value from `device_property_read_*`) instead of raw `-1`.
