# pico.mod

Raspberry Pi Pico SDK platform support for BlitzMax NG embedded applications.

`pico.mod` lets `bmk` build, upload, and run BlitzMax applications on RP2040
and ARM RP2350 boards. It provides Pico implementations of the portable
`Embedded.*` APIs alongside target-specific modules for applications that need
PIO, DMA, flash, PSRAM, or other Pico SDK capabilities.

## Supported boards

| Board | `-board` value | Processor |
| --- | --- | --- |
| Raspberry Pi Pico | `pico` | RP2040 |
| Raspberry Pi Pico W | `pico_w` | RP2040 with CYW43 wireless |
| Raspberry Pi Pico 2 | `pico2` | ARM RP2350; default |
| Raspberry Pi Pico 2 W | `pico2_w` | ARM RP2350 with CYW43 wireless |

Other RP2040 and ARM RP2350 boards supplied by the selected Pico SDK can be
used by passing their SDK board name, for example `-board adafruit_qtpy_rp2040`.
Custom definitions can be supplied through `PICO_BOARD_HEADER_DIRS` and
`PICO_BOARD_CMAKE_DIRS`. All current targets use `-g arm`.

## Requirements

- A Pico-enabled BlitzMax SDK containing matching `bcc2`, `bmk`,
  `embedded.mod`, `blitzmax.mod`, `brl.mod`, `pub.mod`, `random.mod`, and
  `pico.mod` revisions.
- Raspberry Pi Pico SDK 2.3.0 or newer.
- An Arm GNU embedded toolchain.
- CMake and Ninja.
- `picotool` for automatic USB upload.
- `pioasm` when importing `.pio` programs.

`bmk` searches `custom.bmk`, the corresponding environment variables, the host
`PATH` where appropriate, and Raspberry Pi's managed `.pico-sdk` installation
beneath the user's home directory. A `picotool` version matching the SDK is
recommended.

## Quick start

Create `blink.bmx`:

```blitzmax
SuperStrict

Import Pico.Board
Import Pico.Hardware.GPIO
Import Pico.System.Time

Local ledPin:UInt = DefaultLEDPin()
If ledPin = PicoUnavailablePin Then RuntimeError "This board has no default LED"

GPIOInit ledPin
GPIOSetOutput ledPin

While True
    GPIOPut ledPin, True
    SleepMilliseconds 500
    GPIOPut ledPin, False
    SleepMilliseconds 500
Wend
```

Build it for a Pico 2:

```sh
bmk makeapp -a -r -l pico -g arm -board pico2 -o blink blink.bmx
```

Add `-x` to upload, verify, reset, and start it through `picotool`:

```sh
bmk makeapp -a -r -x -l pico -g arm -board pico2 -o blink blink.bmx
```

For the first upload, or when the running firmware does not expose automatic
USB reset, hold BOOTSEL while connecting the board and run the command again.
The generated UF2 may also be copied to the BOOTSEL drive manually.

Inspect a connected RP-series device without building or flashing:

```sh
bmk deviceinfo -l pico
```

An optional `-board` value is shown separately as build configuration. The
retail board is never inferred solely from the detected silicon.

## Build options

| Option | Meaning |
| --- | --- |
| `-l pico` | Select the Pico target |
| `-g arm` | Select the ARM architecture |
| `-board pico` | Build for RP2040/Pico |
| `-board pico2` | Build for RP2350/Pico 2; this is the default |
| `-board <name>` | Use another board definition from the selected Pico SDK |
| `-heap auto` | Use the board-aware managed heap; this is the default |
| `-heap <size>` | Set the managed heap in bytes or with `k`, `KiB`, `m`, or `MiB` |
| `-heap-region sram` | Place the managed heap in internal SRAM; this is the default |
| `-heap-region psram` | Place the managed heap in board-defined external PSRAM |
| `-storage none` | Do not reserve persistent flash; this is the default |
| `-storage <size>` | Reserve sector-aligned persistent flash, for example `-storage 256k` |
| `-float-abi auto` | Use soft on RP2040 and softfp with hardware floating point on RP2350; this is the default |
| `-float-abi hard` | Use the hard-float calling convention on ARM RP2350 |
| `-x` | Upload, verify, reset, and start through `picotool` |
| `-d` | Build with source-level GDB information |
| `-r` | Build optimised release firmware |

After linking, `bmk` reports flash, internal RAM, managed-heap placement,
PSRAM, and the applicable reserves and headroom.

## Tool configuration

Tool locations and persistent defaults can be set in `custom.bmk`:

```bmk
#addoption pico.sdk "/path/to/pico-sdk"
#addoption pico.toolchain "/path/to/arm-none-eabi-toolchain"
#addoption pico.cmake "/path/to/cmake"
#addoption pico.ninja "/path/to/ninja"
#addoption pico.picotool "/path/to/picotool"
#addoption pico.pioasm "/path/to/pioasm"
#addoption pico.board.header.dirs "/path/to/custom/board/headers"
#addoption pico.board.cmake.dirs "/path/to/custom/board/cmake"
#addoption pico.heap.region "psram"
#addoption pico.storage "256k"
#addoption pico.float.abi "hard"
```

The corresponding environment variables are `PICO_SDK_PATH`,
`PICO_TOOLCHAIN_PATH`, `PICO_CMAKE`, `PICO_NINJA`, `PICOTOOL_DIR`, and
`PICO_PIOASM_DIR`, plus `PICO_BOARD_HEADER_DIRS`, `PICO_BOARD_CMAKE_DIRS`, and
`PICO_FLOAT_ABI`.

## Portable and target-specific APIs

Use `Embedded.*` when an application should compile unchanged for both Pico and
ESP32. Use the corresponding `Pico.*` module when the application needs Pico
SDK-specific capabilities. Familiar operation names are retained where the
underlying hardware concepts align.

For example, portable GPIO code imports:

```blitzmax
Import Embedded.Hardware.GPIO
```

Pico-specific code can instead import:

```blitzmax
Import Pico.Hardware.GPIO
```

An application may ignore `Embedded.*` entirely and use the Pico modules
directly without losing access to target-specific functionality.

## Feature guide

| Area | Primary modules | Start with |
| --- | --- | --- |
| Board defaults | `Pico.Board` | [`pico_blink.bmx`](examples/pico_blink.bmx) |
| GPIO | `Embedded.Hardware.GPIO`, `Pico.Hardware.GPIO` | [`gpio_input_irq.bmx`](examples/gpio_input_irq.bmx), [`gpio_irq_event_queue.bmx`](examples/gpio_irq_event_queue.bmx) |
| Time and calendar | `Embedded.System.Time`, `Pico.System.Time`, `Pico.System.Calendar` | [`timer_alarm.bmx`](examples/timer_alarm.bmx), [`calendar.bmx`](examples/calendar.bmx) |
| Power | `Embedded.System.Power`, `Pico.System.Power` | [`low_power_sleep.bmx`](examples/low_power_sleep.bmx) |
| UART | `Embedded.Hardware.UART`, `Pico.Hardware.UART`, `Pico.IO.BufferedUART` | [`uart_controller.bmx`](examples/uart_controller.bmx) |
| I2C and SPI | `Embedded.Hardware.I2C`, `Embedded.Hardware.SPI`, Pico facades | [`i2c_controller.bmx`](examples/i2c_controller.bmx), [`spi_controller.bmx`](examples/spi_controller.bmx) |
| Random data | `Embedded.Random`, `Pico.Random` | [`random_pico.bmx`](examples/random_pico.bmx) |
| ADC and PWM | `Embedded.Hardware.ADC`, `Embedded.Hardware.PWM`, Pico facades | [`adc_temperature.bmx`](examples/adc_temperature.bmx), [`pwm_led.bmx`](examples/pwm_led.bmx) |
| Watchdog and identity | Shared APIs and Pico facades | [`watchdog.bmx`](examples/watchdog.bmx), [`device_info.bmx`](examples/device_info.bmx) |
| PIO | `Pico.Hardware.PIO` | [`pio_square_wave.bmx`](examples/pio_square_wave.bmx), [`pio_irq_event_queue.bmx`](examples/pio_irq_event_queue.bmx) |
| DMA | `Pico.Hardware.DMA` | [`dma_managed_transfer.bmx`](examples/dma_managed_transfer.bmx), [`dma_event_queue.bmx`](examples/dma_event_queue.bmx) |
| PSRAM | `Pico.Hardware.PSRAM` | [`psram_info.bmx`](examples/psram_info.bmx) |
| Filesystems | `Pico.Storage.Flash`, `Pico.Storage.LittleFS` | [`stream_memory.bmx`](examples/stream_memory.bmx) |
| Wi-Fi and sockets | `Embedded.Network.WiFi`, `Pico.Network.WiFi`, `BRL.Socket` | [`wifi_scan.bmx`](examples/wifi_scan.bmx), [`wifi_connect.bmx`](examples/wifi_connect.bmx) |

The target also reuses compatible standard modules such as `BRL.Blitz`,
`BRL.StandardIO`, `BRL.Stream`, `Pub.Time`, and selected collection modules.

## Managed memory and PSRAM

The automatic internal-SRAM managed heap is 192 KiB on RP2040 and 384 KiB on
ARM RP2350. Set an explicit size with `-heap`, using bytes or a suffix such as
`256k` or `1MiB`.

External PSRAM is never selected implicitly. On a board whose SDK definition
publishes a fixed PSRAM capacity:

```sh
bmk makeapp -a -r -l pico -g arm -board <board> \
    -heap-region psram -heap auto application.bmx
```

This uses the declared PSRAM capacity while leaving 64 KiB outside the managed
arena. Import `Pico.Hardware.PSRAM` to query availability and capacity or test
whether an address lies in PSRAM.

The managed runtime is single-core. Ordinary applications should consume
interrupt-driven work through the provided event APIs. Authors of native
integrations should read the [runtime integration guide](docs/runtime-integration.md).

## Filesystems and persistent flash

Reserve a region at the end of flash when building:

```sh
bmk makeapp -a -r -l pico -g arm -board pico2 \
    -storage 256k application.bmx
```

Importing `Pico.Storage.LittleFS` installs that region as the default
`BRL.FileSystem` backend. Ordinary paths then work with `ReadFile`, `WriteFile`,
`CreateDir`, `BRL.Path`, `BRL.Glob`, streams, and text APIs.

A completely blank region is formatted automatically. Nonblank unrecognised
data is never erased automatically, and normal `picotool` application uploads
preserve the reserved region. Creation and modification times are maintained
when calendar time is available. Reads do not update access time, avoiding a
flash write on every read.

`Pico.Storage.Flash` provides bounded access to the same build-reserved region
for applications that need a lower-level storage format.

## PIO and DMA

`Pico.Hardware.PIO` supports imported `pioasm` programs, state-machine
configuration, managed ownership, synchronized starts, FIFO and DMA access, and
timestamped IRQ delivery through `BRL.EventQueue`. RP2350 builds also expose
PIO2, extended FIFO joins, and GPIO-base selection.

`Pico.Hardware.DMA` supports managed-buffer retention, completion events,
pacing timers, chaining, ring addressing, priority, byte swapping, and quiet
IRQs. The PIO and DMA examples include standalone and combined transfer paths.

## Wi-Fi and sockets

On wireless board definitions, `Pico.Network.WiFi` initializes the CYW43 radio,
controls its GPIO 0 output, and delivers asynchronous scan and link-state events
through `BRL.EventQueue`. Station connections use DHCP and expose their IPv4
address, netmask, and gateway.

`WiFiConnect` begins one asynchronous join. `WiFiConnectWait` services the
system until DHCP completes and can retry transient join failures, which is
useful when several mesh access points advertise the same SSID.

Importing `Pub.Net`, directly or through `BRL.Socket`, enables IPv4 DNS, TCP
clients and servers, UDP, readiness polling, and `BRL.SocketStream`. The adapter
supports eight simultaneous sockets, four queued TCP accepts per listener, a
4 KiB receive ring per active TCP connection, and four queued UDP datagrams per
UDP socket. Close all sockets before calling `WiFiDeinitialize`.

Applications which do not import networking do not link lwIP, the wireless
driver, or its firmware. IPv6, TLS, and higher-level HTTP clients are not yet
provided.

## Power and debugging

`Pico.System.Power` provides interrupt-driven and timed clock-gated sleep on
both processors, GPIO-triggered dormant sleep on both, and timed dormant sleep
on RP2350. Timed dormant sleep on RP2040 needs an external always-on clock and
is not exposed by the current API. RP2350 Pstate sleep remains deferred until
its persistent-state contract is defined.

Use `-d` for a GDB-debuggable build with BlitzMax source-line information.
Source stepping, variables, breakpoints, and `DebugStop` are available through
GDB and a compatible debug probe. OpenOCD/GDB launching is a separate step; the
desktop interactive debugger is not used on embedded targets.

Multicore BlitzMax execution is not currently provided.

## Troubleshooting

- **First upload is not detected:** reconnect while holding BOOTSEL, then retry
  `-x` or copy the generated UF2 to the mounted drive.
- **Unknown board:** use the exact Pico SDK board name and check any custom board
  header/CMake directories.
- **LittleFS does not mount:** ensure the application was built with a nonzero
  `-storage` reservation. Formatting nonblank unrecognised data is never
  automatic.
- **Hard-float build fails:** `-float-abi hard` is available only for ARM
  RP2350; the default already uses its FPU through the softfp ABI.
- **No deferred GPIO, DMA, PIO, Wi-Fi, or socket events:** call `PollSystem`,
  `WaitSystem`, or use `BRL.EventQueue` regularly.

## Further documentation

- [Runtime integration and contributor notes](docs/runtime-integration.md)
- [Examples](examples)
- [Raspberry Pi Pico SDK documentation](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
