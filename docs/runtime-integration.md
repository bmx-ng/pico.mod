# Runtime integration and contributor notes

This document is for contributors adding native Pico SDK integrations or
debugging the embedded runtime. Application developers should normally use the
event-based `Embedded.*` and `Pico.*` modules described in the main README.

## Runtime ownership

The compact runtime ABI, allocator, precise garbage collector, strings, arrays,
objects, and exceptions are implemented by `embedded.mod`. `pico.mod` supplies
the managed arena, Pico SDK application integration, and target adapters.

The managed runtime is single-core. Native interrupt handlers must not allocate
managed objects, invoke collection, throw exceptions, or call arbitrary
BlitzMax code. They should record bounded native data and let normal application
execution drain it through the system or event APIs.

Raw managed pointers are borrowed only for the duration of a native call. A
native subsystem that retains managed state must use the embedded runtime's
rooting contract and release every retained root through a deterministic
teardown path.

## Interrupt and event adapters

Existing GPIO, timer, UART, DMA, PIO, Wi-Fi, and socket integrations demonstrate
the expected deferred-event pattern:

1. Validate configuration and reserve bounded native state during setup.
2. Keep interrupt callbacks short and allocation-free.
3. Copy only plain scalar data into a fixed queue or ring.
4. Drain that queue from normal BlitzMax execution.
5. Create managed event objects only while draining.
6. Count and expose dropped events when a fixed queue is full.
7. Disable callbacks and release retained state during teardown.

GPIO events retain the 64-bit microsecond timestamp captured by the native IRQ
callback. PIO and DMA adapters additionally retain managed transfer buffers for
the operation lifetime so the collector cannot reclaim or move ownership while
hardware is active.

## Adding an adapter

When adding a Pico SDK integration:

1. Keep SDK handles and interrupt payloads in native storage.
2. Validate pins, channels, state machines, buffers, and lengths before entering
   the SDK.
3. Copy or root managed data that must outlive the call.
4. Make initialization and teardown repeatable, including partial-failure
   cleanup.
5. Add a shared `embedded.mod` conformance fixture where the contract is
   portable and a Pico example for target-specific behaviour.
6. Test both RP2040 and ARM RP2350 when the peripheral implementation differs.
7. Verify that unused SDK libraries and firmware remain excluded when their
   modules are not imported.

## Validation

The `examples` directory contains application samples and targeted runtime
stress programs. The `tests` directory contains build scripts for individual
features and hardware fixtures.

Useful entry points include:

```sh
tests/run_bmk_embedded_runtime_conformance.sh
tests/run_bmk_board_matrix.sh
tests/run_bmk_littlefs.sh
tests/run_bmk_wifi.sh
```

PIO, DMA, ADC, GPIO IRQ, UART, watchdog, storage, Wi-Fi, float-ABI, and language
runtime paths also have focused scripts under `tests`. Hardware-facing changes
should be exercised on representative RP2040 and RP2350 devices before being
published.
