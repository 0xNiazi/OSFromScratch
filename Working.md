# Bare-Metal OS Development from Scratch: Architectural Notes & System Flow

---

## 1. Core Philosophy: Bare-Metal vs Hosted Systems

In standard software engineering, code runs in a **Hosted Environment**:
* The OS kernel sets up virtual memory, task scheduling, and loads dynamic libraries (`glibc`).
* Programs begin execution at `main(argc, argv)` with standard libraries (`<stdio.h>`, `<stdlib.h>`).

In **Bare-Metal Operating System Development**, there is no host:
* We write **Freestanding C** (`-ffreestanding -nostdlib`) and raw x86 assembly.
* The CPU executes raw machine opcodes directly on physical silicon.
* There is no runtime memory management, no standard library, and no standard I/O until we implement them ourselves.

```mermaid
flowchart LR
    subgraph Hosted Environment (Standard App)
        APP["Application (main)"] --> LIBC["glibc / stdlib"]
        LIBC --> KERNEL["Linux / Windows Kernel"]
        KERNEL --> HW["Physical Hardware"]
    end

    subgraph Bare-Metal Environment (Our OS)
        OUR_C["Our C Kernel (kernel_main)"] --> TRAMP["Assembly Entry (_start)"]
        TRAMP --> BOOT["Custom Bootloader (0x7C00)"]
        BOOT --> HW2["Physical Hardware"]
    end
```

---

## 2. Phase 1: BIOS Boot Protocol & 16-bit Real Mode

### 2.1 The Power-On Sequence
1. **Power-On Reset (POR):** The CPU powers on with registers initialized to hardware defaults.
2. **Reset Vector:** The Program Counter points to physical address `0xFFFFFFF0` (top of 4GB address space mapped to motherboard ROM).
3. **POST (Power-On Self-Test):** Motherboard firmware checks RAM, keyboard, bus topology, and storage drives.
4. **Boot Sector Hunt:** The BIOS searches configured boot devices for **Sector 0** (Master Boot Record).

```mermaid
sequenceDiagram
    participant Hardware as CPU / Motherboard
    participant BIOS as BIOS ROM
    participant Storage as Boot Disk (MBR)
    participant RAM as Physical RAM

    Hardware->>BIOS: Power-On Reset (Jump to 0xFFFFFFF0)
    BIOS->>BIOS: Run POST & Device Discovery
    BIOS->>Storage: Read Sector 0 (512 Bytes)
    Storage-->>RAM: Load bytes into address 0x7C00
    BIOS->>RAM: Check Magic Signature (0xAA55 at offset 510)
    Note over BIOS,RAM: Valid MBR Detected!
    BIOS->>RAM: Jump to 0x0000:0x7C00 (Hands control to our code)
```

### 2.2 The Geometry of an MBR Boot Sector
A boot sector is strictly **512 bytes**:
* **Bytes 0–445:** Boot code (executable instructions).
* **Bytes 446–509:** Partition table entries (4 entries of 16 bytes each).
* **Bytes 510–511:** The Magic Signature `0x55` and `0xAA` (as a 16-bit little-endian word: `0xAA55`).

### 2.3 Why Memory Address `0x7C00`?
In 1981, the designers of the IBM 5150 chose `0x7C00`:
* Minimum baseline RAM was 32 KB (`0x0000` to `0x7FFF`).
* They wanted the bootloader placed near the very top of conventional RAM to leave as much contiguous low memory free as possible for the loaded operating system.
* Subtracting 1 KB (512 bytes for bootloader code + 512 bytes for stack/scratchpad) from `0x8000` (32 KB) yields `0x7C00`.

### 2.4 The Real Mode Memory Model
* **Bit Width:** 16-bit registers (`AX`, `BX`, `CX`, `DX`, `SI`, `DI`, `SP`, `BP`, `IP`).
* **Address Space:** 1 Megabyte (`0x00000` to `0xFFFFF`).
* **Segmented Addressing Scheme:**
  $$\text{Physical Address} = (\text{Segment} \times 16) + \text{Offset}$$
* **BIOS Interrupts:** Handled via the **Interrupt Vector Table (IVT)** at physical addresses `0x0000..0x03FF`:
  * `INT 0x10`: Video services (Teletype mode `AH=0x0E`).
  * `INT 0x13`: Low-level disk services (Sector read `AH=0x02`).

---

## 3. Phase 2: Transitioning to 32-bit Protected Mode

### 3.1 Why Leave Real Mode?
1. **Memory Addressing:** Real Mode is locked to 1 MB. Protected Mode enables flat 32-bit addressing up to **4 Gigabytes**.
2. **Memory Safety & Privileges:** Real Mode has no memory protection; any instruction can wipe interrupt tables. Protected Mode introduces **Privilege Rings** (Ring 0 Kernel to Ring 3 User).
3. **Compiler Compatibility:** Modern C compilers (GCC) output 32-bit/64-bit flat memory instructions; they cannot target 16-bit segmented Real Mode.

### 3.2 The Global Descriptor Table (GDT)
In Protected Mode, segment registers (`CS`, `DS`, `SS`, `ES`) no longer hold base addresses. Instead, they hold **Segment Selectors** (byte offsets into the GDT).

```text
 63       56 55  52 51  48 47       40 39        16 15         0
+-----------+------+------+-----------+------------+------------+
| Base 31:24|Flags |Limit |Access Byte| Base 23:0  | Limit 15:0 |
|  (1 byte) |(4bit)| 19:16|(1 byte)   | (3 bytes)  | (2 bytes)  |
+-----------+------+------+-----------+------------+------------+
```

Our GDT creates a **Flat Memory Model** where Code and Data segments span the entire 4 GB address space:
* **Entry 0 (Offset `0x00`): Null Descriptor** (8 zero bytes required by CPU architecture).
* **Entry 1 (Offset `0x08`): Code Segment Descriptor** (Base = `0x0`, Limit = `0xFFFFF`, Granularity = 4KB, 32-bit, Executable, Readable, Ring 0).
* **Entry 2 (Offset `0x10`): Data Segment Descriptor** (Base = `0x0`, Limit = `0xFFFFF`, Granularity = 4KB, 32-bit, Writable, Ring 0).

### 3.3 The 4-Step Protected Mode Switch Sequence

```mermaid
flowchart TD
    STEP1["1. Disable Interrupts (cli)"] --> STEP2["2. Load GDT Register (lgdt [gdt_descriptor])"]
    STEP2 --> STEP3["3. Set Protection Enable Bit in CR0 (PE = 1)"]
    STEP3 --> STEP4["4. Far Jump (jmp 0x08:init_pm)"]
    STEP4 --> STEP5["5. Reload Segment Registers (DS, SS, ES, FS, GS = 0x10)"]
    STEP5 --> STEP6["6. Setup 32-bit Stack Pointer (ESP = 0x90000)"]
```

#### Why Each Action is Necessary:
1. **`cli`:** 16-bit BIOS interrupt handlers will crash if triggered in 32-bit mode.
2. **`lgdt`:** Tells the CPU where the descriptor table resides in RAM and its limit.
3. **`CR0` Bit 0:** The hardware switch that toggles the CPU into 32-bit execution mode.
4. **The Far Jump:** The CPU instruction prefetch queue contains decoded 16-bit instructions. A far jump flushes the pipeline and forces `CS` to reload with selector `0x08`.
5. **Reloading Segments:** General segment registers retain invalid 16-bit limits until explicitly refreshed with the data selector (`0x10`).

---

## 4. Phase 3: Bare-Metal Display Mechanics (VGA Text Buffer)

Once in Protected Mode, BIOS `INT 0x10` is gone. Video output is achieved by writing directly to **Memory-Mapped I/O (MMIO)** at physical address **`0xB8000`**.

### 4.1 Screen Geometry
* **Dimensions:** 80 columns $\times$ 25 rows = 2,000 character cells.
* **Cell Size:** Exactly 2 bytes per cell (Total buffer size = 4,000 bytes).

```text
Cell Memory Layout:
Byte 0: ASCII Character Code (e.g., 'A' = 0x41)
Byte 1: Color Attribute Byte
        Bit 7:    Blink / Bright background
        Bit 6..4: Background Color (0-7)
        Bit 3..0: Foreground Color (0-15)
```

### 4.2 Linear Address Calculation
To write a character at grid coordinate $(x, y)$:
$$\text{Memory Offset} = 2 \times (y \times 80 + x)$$

---

## 5. Phase 4: Disk Loading & Multi-Stage Bootloader Architecture

Because an MBR is limited to 512 bytes, operating systems use a **multi-stage architecture**:

```mermaid
flowchart TD
    subgraph Disk Geometry
        SEC1["Sector 1 (LBA 0): boot.bin (512 Bytes)"]
        SEC2["Sectors 2..15: kernel.bin (C Kernel + Entry)"]
    end

    subgraph Memory Layout in RAM
        M1["0x0000:0x7C00 - Bootloader loaded by BIOS"]
        M2["0x0000:0x1000 - Kernel loaded by Bootloader via INT 0x13"]
        M3["0x0009:0x0000 - 32-bit Stack (ESP grows downward)"]
        M4["0x000B:0x8000 - VGA Text Mode Buffer"]
    end

    SEC1 -.->|BIOS boots| M1
    M1 -->|INT 0x13 AH=0x02 reads SEC2| M2
    M1 -->|Switch to 32-bit PM| M3
    M1 -->|call 0x1000| M2
    M2 -->|Direct MMIO Writes| M4
```

### BIOS Disk Reading Parameters (`INT 0x13, AH=0x02`):
* `AH = 0x02` (Read Sectors)
* `AL = 0x0E` (Number of sectors to read: 14 sectors)
* `CH = 0` (Cylinder 0)
* `CL = 2` (Sector 2: BIOS sectors are 1-indexed)
* `DH = 0` (Head 0)
* `DL = Drive Number` (Passed by BIOS in `DL` upon power-on)
* `ES:BX = 0x0000:0x1000` (Destination RAM pointer)

---

## 6. Phase 5: Freestanding C & Linker Scripts

### 6.1 The Compilation Pipeline
```mermaid
flowchart LR
    ASM["kernel_entry.asm"] -->|nasm -f elf32| OBJ1["kernel_entry.o"]
    CSRC["kernel.c"] -->|gcc -m32 -ffreestanding| OBJ2["kernel.o"]
    LDSCRIPT["linker.ld"] --> LD["GNU Linker (ld)"]
    OBJ1 --> LD
    OBJ2 --> LD
    LD -->|--oformat binary| KBIN["kernel.bin (Raw flat binary)"]
```

### 6.2 The Purpose of `kernel_entry.asm`
C compilers do not guarantee that the entry function (`kernel_main`) is placed at byte 0 of the generated code section. 
The assembly trampoline guarantees execution enters `_start` at byte 0 and issues a standardized `call kernel_main`.

### 6.3 Linker Script Architecture (`linker.ld`)
* **`ENTRY(_start)`:** Sets the execution entry symbol.
* **`. = 0x1000;`:** Location counter. Enforces that all internal pointers, jump targets, and memory references are resolved relative to `0x1000`.
* **Section Sequencing:**
  * `.text`: Executable code (with `kernel_entry.o` placed first).
  * `.rodata`: Constant strings and read-only tables.
  * `.data`: Initialized global variables.
  * `.bss`: Zero-initialized / uninitialized variables.
* **`ALIGN(4)`:** Enforces 32-bit word alignment on memory boundaries for optimal CPU bus access.

---

## 7. Real Hardware vs Virtual Machine (Troubleshooting & Quirks)

Developing for real physical silicon revealed several critical architectural lessons:

| Symptom / Trap | Root Cause | Engineering Solution |
| :--- | :--- | :--- |
| **Triple Fault on Real AMD Hardware** | Some physical BIOSes initialize with dirty segment registers (`CS = 0x07C0`, `DS != 0`). Addressing `[gdt_descriptor]` loaded garbage RAM. | Perform a far jump `jmp 0x0000:start` to normalize `CS`, followed by `xor ax, ax; mov ds, ax; mov es, ax; mov ss, ax`. |
| **CPU Runaway / Fan Overheating** | `jmp $` runs in an infinite 100% busy-loop on physical CPU cores. | Use `cli` followed by a halted loop: `.hang: hlt; jmp .hang` to place the CPU core in low-power idle. |
| **BIOS End-of-Disk Error on `INT 0x13`** | Reading 14 sectors from an image smaller than 7.5 KB causes the disk controller to return an out-of-bounds error. | Pad `os-image.bin` with `truncate -s 64K` to guarantee disk geometry depth. |
| **Modern NVMe / eDP Limitations** | Modern laptop UEFI firmware lacks legacy `INT 0x13` Option ROMs for PCIe NVMe drives, and eDP internal displays do not route physical `0xB8000` VGA MMIO. | Chainload via GRUB or use VESA Linear Framebuffers (LFB) for graphical display output. |

---

## 8. Phase 2: Hardware Port I/O & The VGA Display Driver

### 8.1 Port-Mapped I/O (PMIO) Architecture
While video memory (`0xB8000`) uses Memory-Mapped I/O (MMIO), motherboard controllers communicate via a dedicated 16-bit address space (Ports `0x0000` to `0xFFFF`):
* Standard memory pointers cannot touch this space.
* The CPU provides dedicated hardware opcodes: `IN` (read from port) and `OUT` (write to port).
* Using GCC's `-masm=intel` flag, we implemented pure Intel syntax inline assembly:
  * `in al, dx` (Destination `AL`, Source `DX`)
  * `out dx, al` (Destination `DX`, Source `AL`)

```mermaid
flowchart TD
    subgraph CPU
        REG_AL["AL (Data Byte)"]
        REG_DX["DX (Port Address)"]
    end

    subgraph I/O Bus
        IO_PORT["I/O Port (0x0000 - 0xFFFF)"]
    end

    subgraph Peripherals
        VGA_CTRL["VGA Controller (0x3D4 / 0x3D5)"]
        KEYBOARD["PS/2 Keyboard (0x60)"]
        PIC_CHIP["8259 PIC (0x20 / 0xA0)"]
    end

    REG_AL -->|"out dx, al"| IO_PORT
    IO_PORT -->|"in al, dx"| REG_AL
    IO_PORT --> VGA_CTRL & KEYBOARD & PIC_CHIP
```

### 8.2 CRT Controller & Hardware Cursor Control
The physical VGA blinking text cursor is driven by the internal CRT Controller registers:
* **Index / Address Register:** Port `0x3D4`
* **Data Register:** Port `0x3D5`
To position the cursor at linear offset $\text{pos} = (y \times 80) + x$:
1. Select Register `0x0E` via Port `0x3D4`, write high byte `(pos >> 8) & 0xFF` to Port `0x3D5`.
2. Select Register `0x0F` via Port `0x3D4`, write low byte `pos & 0xFF` to Port `0x3D5`.

### 8.3 VGA Color Encoding & Coordinate Mapping
* Color attribute byte packing:
  $$\text{Attribute Byte} = \text{Foreground} \mid (\text{Background} \ll 4)$$
* Memory offset for grid cell $(x, y)$:
  $$\text{Byte Offset} = 2 \times (y \times 80 + x)$$
  * Even byte: ASCII Character
  * Odd byte: Color Attribute

### 8.4 The Driver Abstraction Layer
```mermaid
flowchart TD
    KM["kernel_main()"] --> VP["vga_print()"]
    KM --> VDB["vga_draw_block()"]
    KM --> VC["vga_clear()"]
    VP --> VPC["vga_put_char()"]
    VPC --> VSC["vga_set_cursor()"]
    VC --> VSC
    VSC --> OUTB["outb() [ports.c]"]
    VPC --> MMIO["0xB8000 (Video RAM)"]
    VDB --> MMIO
```

---

## 9. Phase 3: Hardware Interrupts, 8259 PIC, PS/2 Keyboard & 8254 PIT Timer

### 9.1 The Interrupt Architecture (IDT)
In 32-bit Protected Mode, interrupts and exceptions are vectored through the **Interrupt Descriptor Table (IDT)**:
* An array of **256 8-byte descriptors** loaded into the CPU via the `lidt [idt_ptr]` instruction.
* **Vector Layout:**
  * Vectors `0..31` (`0x00..0x1F`): **CPU Hardware Exceptions** (Faults, Traps, Aborts).
  * Vectors `32..47` (`0x20..0x2F`): **Hardware IRQs** (mapped from the 8259 PIC).
  * Vectors `48..255` (`0x30..0xFF`): **Software Interrupts / System Calls**.

```text
 63                  48 47          40 39       32 31               16 15                0
+----------------------+--------------+-----------+-------------------+--------------------+
|  Handler Offset High |  Type & Attr |   Zero    |  Segment Selector | Handler Offset Low |
|      (bits 16..31)   |   (1 byte)   |  (1 byte) |  (2 bytes: 0x08)  |    (bits 0..15)    |
+----------------------+--------------+-----------+-------------------+--------------------+
```
* **Flags byte `0x8E`:** Present = 1, Ring 0 = 00, 32-bit Interrupt Gate = 1110b.

### 9.2 The 8259 PIC Remap (The IBM vs. Intel Conflict)
The motherboard pairs two 8259 PIC chips:
* **Master PIC** (Ports `0x20` / `0x21`): IRQ 0..7
* **Slave PIC** (Ports `0xA0` / `0xA1`): IRQ 8..15 (Cascaded into Master IRQ 2)

#### The Conflict:
IBM's default BIOS mapped IRQ 0..7 to interrupt vectors `0x08..0x0F`. On 386+ processors, vector `0x08` is the **Double Fault Exception**. A timer tick (IRQ 0) without remapping immediately triggers an unhandled Double Fault $\rightarrow$ Triple Fault $\rightarrow$ Reboot.

#### The 4-Step Initialization Ritual (ICW1..ICW4):
1. **ICW1 (`0x11`):** Enter initialization mode with cascade support.
2. **ICW2 (`0x20`, `0x28`):** Vector offsets:
   * Master IRQs 0..7 $\rightarrow$ Vectors **`0x20..0x27`** (32..39)
   * Slave IRQs 8..15 $\rightarrow$ Vectors **`0x28..0x2F`** (40..47)
3. **ICW3 (`0x04`, `0x02`):** Establish cascade lines (Master Pin 2 $\leftrightarrow$ Slave Identity 2).
4. **ICW4 (`0x01`):** Select 8086/88 microprocessor environment mode.
5. **End of Interrupt (EOI):** Handlers must acknowledge completion by sending `outb(0x20, 0x20)` to the Master PIC (and `outb(0xA0, 0x20)` if Slave IRQ) before returning.

### 9.3 The PS/2 Keyboard Driver (IRQ 1 / Vector 0x21)
The keyboard controller (Intel 8042) pulses IRQ 1 on state transitions:
* **Make Code (Key Press):** Top bit is `0` (`scancode < 0x80`).
* **Break Code (Key Release):** Top bit is `1` (`scancode & 0x80` = `Make Code | 0x80`).
* **Typematic Repeat:** Holding a key triggers the 8042 controller's internal repeat timer, streaming repeated make codes.
* **Scancode Translation:** We translate PS/2 Set 1 scancodes into ASCII characters via a 128-byte lookup table.
* **Destructive Backspace:** When `\b` is received, the driver queries current cursor coordinates (`vga_get_x()`, `vga_get_y()`), retreats 1 cell, overwrites with an empty space `' '`, and resets the cursor.

### 9.4 The 8254 PIT Timer (IRQ 0 / Vector 0x20) & Thermal Management
The Programmable Interval Timer (PIT) provides real-time OS ticks:
* **Oscillator Base Clock:** $1,193,182\text{ Hz}$.
* **Divisor Formula:** $\text{Divisor} = \frac{1,193,182}{\text{Target Frequency (Hz)}}$.
* **Mode 3 (Square Wave):** Configured by writing command `0x36` to Port `0x43`, followed by low/high divisor bytes to Port `0x40`.
* **Low-Power Sleeping & The `volatile` Keyword:**
  ```c
  uint32_t target = timer_ticks + ticks;
  while (timer_ticks < target) {
      __asm__ __volatile__("hlt"); // Halts execution core until next IRQ tick
  }
  ```
  * `volatile` prevents the compiler from caching `timer_ticks` in a register.
  * `hlt` places the physical CPU core in a C1 low-power sleep state, preventing thermal runaway (solving the 100% CPU overheating trap on bare metal!).

### 9.5 The Interrupt Dispatch Pipeline
```mermaid
sequenceDiagram
    participant HW as Hardware (Keyboard / Timer)
    participant PIC as 8259 PIC
    participant CPU as x86 CPU
    participant ASM as interrupt.asm (pusha)
    participant C as C Handler (timer/keyboard_c)

    HW->>PIC: Pulse IRQ Line (IRQ 0 or 1)
    PIC->>CPU: Signal INTR pin with Vector (0x20 or 0x21)
    CPU->>CPU: Push EFLAGS, CS, EIP to stack
    CPU->>ASM: Jump to ISR Stub (timer/keyboard_handler_asm)
    ASM->>ASM: pusha (Save all 8 general registers)
    ASM->>C: call timer/keyboard_handler_c()
    C->>HW: Read Port 0x60 (if keyboard) / Increment ticks
    C->>PIC: outb(0x20, 0x20) [Send EOI]
    C-->>ASM: return
    ASM->>ASM: popa (Restore all 8 general registers)
    ASM-->>CPU: iret (Atomically restore EIP, CS, EFLAGS)
    CPU->>CPU: Resume interrupted code seamlessly!
```

---

## 10. Summary of Project Map

```text
OSfromScratch/
├── boot/
│   ├── boot.asm           # 16-bit MBR, disk reader, 32-bit switch, far jump
│   └── gdt.asm            # GDT null, code, data descriptors & GDTR structure
├── drivers/
│   ├── keyboard.c         # PS/2 keyboard driver (IRQ 1, scancodes, backspace)
│   ├── ports.c            # Hardware port I/O (inb, outb) in Intel asm
│   ├── timer.c            # 8254 PIT timer driver (IRQ 0, 100Hz, sleep via hlt)
│   └── vga.c              # VGA text driver (clear, print, cursor, block render)
├── kernel/
│   ├── idt.c              # 256-gate IDT setup & 8259 PIC remapping
│   ├── interrupt.asm      # Low-level assembly ISR stubs (pusha/popa/iret)
│   ├── kernel_entry.asm   # 32-bit trampoline calling kernel_main
│   └── kernel.c           # High-level C kernel entry point
├── include/
│   ├── idt.h              # IDT entry and pointer packed structures
│   ├── keyboard.h         # Keyboard driver API prototypes
│   ├── ports.h            # Hardware port function prototypes
│   ├── timer.h            # PIT timer API prototypes
│   ├── types.h            # Bare-metal fixed-width types (uint8_t, size_t, NULL)
│   └── vga.h              # VGA dimensions, 16-color palette & API prototypes
├── linker.ld              # Memory layout linking code at 0x1000
├── Makefile               # Automated compilation, disk assembly & QEMU launcher
├── run                    # Executable launcher script
└── Working.md             # Architectural documentation & study notes
```
