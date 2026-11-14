# 15_dt_overlay — Device Tree Overlays (`.dts` to `.dtbo`)

A Device Tree Source Overlay (`my_overlay.dts`) demonstrating how to describe hardware peripherals and devices for the Linux kernel without recompiling the entire base Device Tree.

---

## What This Chapter Covers

On embedded Linux systems (such as ARM and RISC-V targets), the kernel learns about attached hardware through a **Device Tree**. 

A **Device Tree Overlay (`.dtbo`)** allows you to dynamically patch the running Device Tree:
1. **Dynamic Extension**: Adds new device nodes, pin muxing, or hardware configurations at runtime or during boot.
2. **Device Matching**: Defines a unique `compatible` string (`"brightlight,mydev"`) that triggers probe callbacks in matching platform drivers.
3. **Property Passing**: Supplies custom device parameters (such as `string_val = "Hello, DT!"` and `int_val = <123>`) directly to the kernel driver.

---

## Files

| File | Purpose |
|---|---|
| `my_overlay.dts` | Device Tree Source overlay defining `my_device` under `/` |
| `Makefile` | Automates compilation using `dtc` with symbols support |
| `README.md` | Documentation and usage guide |

---

## Quick Start (Step-by-Step)

### 1. Install the Device Tree Compiler (`dtc`)
```bash
sudo apt update
sudo apt install device-tree-compiler
```

### 2. Compile the Overlay
Run `make`:
```bash
make
```
*(Runs `dtc -@ -I dts -O dtb -o my_overlay.dtbo my_overlay.dts`)*.

Verify the compiled binary overlay blob:
```bash
ls -lh my_overlay.dtbo
```

### 3. Cleanup
```bash
make clean
```

---

## How Overlays Are Deployed

### On a Target Board (e.g. Raspberry Pi / ARM)
On an embedded ARM system running a base Device Tree, the compiled overlay is loaded dynamically:
```bash
sudo dtoverlay my_overlay.dtbo
```
Once loaded, the device node appears in the live tree:
```bash
ls /proc/device-tree/my_device
cat /proc/device-tree/my_device/compatible
# Output: brightlight,mydev
```

### On an x86 Virtual Machine (VirtualBox)
* Standard x86 PCs and VMs use **ACPI and PCI** rather than Device Tree.
* `dtoverlay` is a Raspberry Pi-specific tool, so it is not present on Ubuntu x86.
* However, writing `.dts` files and compiling them with `dtc` is identical across all development hosts. The `compatible` string defined here (`"brightlight,mydev"`) directly prepares you for the upcoming **Platform Driver** chapter!

---

## Key Technical Details

### 1. Overlay Syntax Breakdown
```dts
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target-path = "/";
        __overlay__ {
            my_dev_label: my_device {
                compatible = "brightlight,mydev";
                status = "okay";
                string_val = "Hello, DT!";
                int_val = <123>;
            };
        };
    };
};
```
* **`/plugin/;`**: Informs `dtc` that this is an overlay plugin meant to modify an existing tree rather than a complete standalone system tree.
* **`fragment@0`**: An individual patch block in the overlay.
* **`target-path = "/"`**: Specifies where in the base Device Tree this node will be inserted (here, under the root node `/`).
* **`__overlay__`**: Contains the actual node and properties to inject.
* **`compatible`**: Used by the Linux kernel to bind this device to a matching `struct platform_driver`.

### 2. Why the `-@` Flag Matters
When compiling overlays, you must pass `-@`:
```bash
dtc -@ -I dts -O dtb -o my_overlay.dtbo my_overlay.dts
```
The `-@` flag generates a `__symbols__` node inside the `.dtbo`, allowing the kernel runtime overlay resolver to track labels and reference phandles across different files.
