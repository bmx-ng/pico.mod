# Pico modules for BlitzMax

`pico.mod` provides the BlitzMax module namespace for Raspberry Pi Pico
microcontrollers. It is used with the bcc2 `pico` target, the Pico-enabled bmk
build pipeline, and the Raspberry Pi Pico SDK.

## Supported boards

- Raspberry Pi Pico, selected with `-board pico`
- Raspberry Pi Pico 2, selected with `-board pico2`
- Other RP2040 and ARM RP2350 boards defined by the selected Pico SDK

Pass the Pico SDK board-definition name to `-board`, for example
`-board adafruit_qtpy_rp2040`. Custom board definitions can be supplied through
`PICO_BOARD_HEADER_DIRS` and `PICO_BOARD_CMAKE_DIRS`. All targets currently use
the ARM toolchain (`-g arm`).

## Modules

The namespace includes:

- `Pico.Core` and `Pico.Runtime`
- `Pico.Board` for defaults supplied by the selected SDK board
- `Pico.Board.Pico` and `Pico.Board.Pico2` compatibility modules
- `Pico.Hardware.GPIO`
- `Pico.Hardware.ADC`
- `Pico.Hardware.DMA`
- `Pico.Hardware.I2C`
- `Pico.Hardware.PIO`
- `Pico.Hardware.PWM`
- `Pico.Hardware.SPI`
- `Pico.Hardware.UART`
- `Pico.Hardware.Watchdog`
- `Pico.Storage.Flash` for bounded access to a build-reserved flash region
- `Pico.Storage.LittleFS` for persistent `TStream` and `BRL.FileSystem` storage
- `Pico.System.Calendar`
- `Pico.System.Time`
- `Pico.System.Power`
- `Pico.System.Device`
- `Pico.IO.StandardIO`

The target also reuses compatible standard modules such as `BRL.Blitz`,
`BRL.StandardIO`, `BRL.Stream`, `Pub.Time`, and selected collection modules.
`Pico.System.Calendar` controls the board's UTC calendar clock, while familiar
`Pub.Time` functions such as `CurrentDateTime`, `CurrentUnixTime`,
`CurrentDate`, and `CurrentTime` read it. The
[`examples`](examples) directory contains language, runtime, peripheral, PIO,
DMA, stream, and debugging examples.

## Requirements

- A Pico-enabled BlitzMax SDK containing the matching bcc2, bmk,
  `blitzmax.mod`, `brl.mod`, and `pico.mod` revisions
- Raspberry Pi Pico SDK 2.3.0 or newer
- An Arm GNU embedded toolchain
- CMake and Ninja
- `picotool` for automatic USB upload
- `pioasm` when importing `.pio` sources

bmk searches `custom.bmk`, environment variables, the host `PATH` where
appropriate, and Raspberry Pi's managed `.pico-sdk` installation in the user's
home directory.

Pico SDK 2.3.0 is the supported baseline. `Pico.System.Power` uses its official
`pico_low_power` library and is linked only when that module is imported. A
matching `picotool` version is recommended.

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
| `-board pico2` | Build for RP2350/Pico 2; this is the default |
| `-board <name>` | Use another board definition from the selected Pico SDK |
| `-heap auto` | Use the board-aware managed heap; this is the default |
| `-heap <size>` | Set the managed heap in bytes or with `k`, `KiB`, `m`, or `MiB` |
| `-storage none` | Do not reserve persistent flash; this is the default |
| `-storage <size>` | Reserve sector-aligned persistent flash, for example `-storage 256k` |
| `-x` | Upload, verify, and start through `picotool` |
| `-d` | Build with source-level GDB information |
| `-r` | Build optimised release firmware |

The automatic managed heap is 192 KiB on RP2040 and 384 KiB on ARM RP2350.
External PSRAM is not used automatically. After linking, bmk reports the
board-configured flash capacity, managed-heap use, other internal RAM use, and
remaining internal-RAM headroom.

Importing `Pico.Storage.LittleFS` installs LittleFS as the default
`BRL.FileSystem` backend, so ordinary paths work with familiar APIs including
`ReadFile`, `WriteFile`, `CreateDir`, `BRL.Path`, and `BRL.Glob`. The module
requires `-storage`; a completely blank region is formatted automatically,
while nonblank unrecognised data is never erased automatically. Normal
`picotool` application uploads preserve the reserved region. Creation and
modification times are maintained when calendar time is available, and all
three standard file times can be set explicitly. Access time is not changed by
reads, avoiding an otherwise costly flash write for every read operation.

## Tool configuration

Tool locations can be set in `custom.bmk`:

```bmk
#addoption pico.sdk "/path/to/pico-sdk"
#addoption pico.toolchain "/path/to/arm-none-eabi-toolchain"
#addoption pico.cmake "/path/to/cmake"
#addoption pico.ninja "/path/to/ninja"
#addoption pico.picotool "/path/to/picotool"
#addoption pico.pioasm "/path/to/pioasm"
#addoption pico.board.header.dirs "/path/to/custom/board/headers"
#addoption pico.board.cmake.dirs "/path/to/custom/board/cmake"
#addoption pico.storage "256k"
```

The corresponding environment variables are `PICO_SDK_PATH`,
`PICO_TOOLCHAIN_PATH`, `PICO_CMAKE`, `PICO_NINJA`, `PICOTOOL_DIR`, and
`PICO_PIOASM_DIR`, plus `PICO_BOARD_HEADER_DIRS` and
`PICO_BOARD_CMAKE_DIRS` for custom boards.

## Current scope

The Pico runtime supports normal compiled scalar code, strings, arrays,
objects, inheritance, interfaces, structs, enums, generics, closures,
exceptions, finalizers, streams, block-memory operations, Incbin resources,
and precise managed collection. Native interrupt handlers record events for
BlitzMax code to consume outside interrupt context.

`Pico.System.Power` provides interrupt and timed clock-gated sleep on both
processors, GPIO-triggered dormant sleep on both processors, and timed dormant
sleep on RP2350. Timed dormant sleep on RP2040 needs an external always-on clock
and is not exposed by the initial API. RP2350 Pstate sleep, which resumes through
a reboot path, is also deferred until its persistent-state contract is defined.

Multicore execution and the desktop interactive debugger are not currently
provided. Source stepping, variables, breakpoints, and `DebugStop` are
available through GDB and a compatible debug probe.
