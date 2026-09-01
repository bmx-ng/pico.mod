# Pico modules for BlitzMax

`pico.mod` provides the BlitzMax module namespace for Raspberry Pi Pico
microcontrollers. It is used with the bcc2 `pico` target, the Pico-enabled bmk
build pipeline, and the Raspberry Pi Pico SDK.

## Supported boards

- Raspberry Pi Pico and other RP2040 boards selected with `-board pico`
- Raspberry Pi Pico 2 and other RP2350 boards selected with `-board pico2`

Both targets currently use the ARM toolchain (`-g arm`).

## Modules

The namespace includes:

- `Pico.Core` and `Pico.Runtime`
- `Pico.Board.Pico` and `Pico.Board.Pico2`
- `Pico.Hardware.GPIO`
- `Pico.Hardware.ADC`
- `Pico.Hardware.DMA`
- `Pico.Hardware.I2C`
- `Pico.Hardware.PIO`
- `Pico.Hardware.PWM`
- `Pico.Hardware.SPI`
- `Pico.Hardware.UART`
- `Pico.System.Time`
- `Pico.IO.StandardIO`

The target also reuses compatible standard modules such as `BRL.Blitz`,
`BRL.StandardIO`, `BRL.Stream`, and selected collection modules. The
[`examples`](examples) directory contains language, runtime, peripheral, PIO,
DMA, stream, and debugging examples.

## Requirements

- A Pico-enabled BlitzMax SDK containing the matching bcc2, bmk,
  `blitzmax.mod`, `brl.mod`, and `pico.mod` revisions
- The Raspberry Pi Pico SDK
- An Arm GNU embedded toolchain
- CMake and Ninja
- `picotool` for automatic USB upload
- `pioasm` when importing `.pio` sources

bmk searches `custom.bmk`, environment variables, the host `PATH` where
appropriate, and Raspberry Pi's managed `.pico-sdk` installation in the user's
home directory.

## Building and uploading

Build an optimised Pico 2 application:

```sh
bmk makeapp -a -r -l pico -g arm -board pico2 -o hello hello.bmx
```

Add `-x` to upload, verify, and start the generated firmware through `picotool`:

```sh
bmk makeapp -a -r -x -l pico -g arm -board pico2 -o hello hello.bmx
```

For the first upload, or when the running firmware does not expose automatic
USB reset, hold BOOTSEL while connecting the board and run the command again.
The generated UF2 can also be copied to the BOOTSEL drive manually.

Use `-d` instead of `-r` to produce a GDB-debuggable build with BlitzMax source
line information. Debug-probe launching is a separate OpenOCD/GDB step.

## Pico build options

| Option | Meaning |
| --- | --- |
| `-l pico` | Select the Pico target |
| `-g arm` | Select the ARM architecture |
| `-board pico` | Build for RP2040/Pico |
| `-board pico2` | Build for RP2350/Pico 2; this is the default Pico board |
| `-heap auto` | Use the board-aware managed heap; this is the default |
| `-heap <size>` | Set the managed heap in bytes or with `k`, `KiB`, `m`, or `MiB` |
| `-x` | Upload, verify, and start through `picotool` |
| `-d` | Build with source-level GDB information |
| `-r` | Build optimised release firmware |

The automatic managed heap is 192 KiB on Pico and 384 KiB on Pico 2. After
linking, bmk reports flash use, managed-heap use, other RAM use, and remaining
RAM headroom.

## Tool configuration

Tool locations can be set in `custom.bmk`:

```bmk
#addoption pico.sdk "/path/to/pico-sdk"
#addoption pico.toolchain "/path/to/arm-none-eabi-toolchain"
#addoption pico.cmake "/path/to/cmake"
#addoption pico.ninja "/path/to/ninja"
#addoption pico.picotool "/path/to/picotool"
#addoption pico.pioasm "/path/to/pioasm"
```

The corresponding environment variables are `PICO_SDK_PATH`,
`PICO_TOOLCHAIN_PATH`, `PICO_CMAKE`, `PICO_NINJA`, `PICOTOOL_DIR`, and
`PICO_PIOASM_DIR`.

## Current scope

The Pico runtime supports normal compiled scalar code, strings, arrays,
objects, inheritance, interfaces, structs, enums, generics, closures,
exceptions, finalizers, streams, block-memory operations, Incbin resources,
and precise managed collection. Native interrupt handlers record events for
BlitzMax code to consume outside interrupt context.

Multicore execution and the desktop interactive debugger are not currently
provided. Source stepping, variables, breakpoints, and `DebugStop` are
available through GDB and a compatible debug probe.
