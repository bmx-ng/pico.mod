# Floating-point ABI benchmark

This benchmark compares call-heavy integer, `Float`, and `Double` workloads
between the default RP2350 softfp ABI and the optional hard-float ABI. The work
functions live in a separate native translation unit and are explicitly not
inlined, so floating-point argument and result passing remains part of the
measurement.

Build and upload each variant to the same RP2350 board:

```text
bmk makeapp -r -t console -l pico -g arm -board <board> -o float_softfp -x float_abi_benchmark.bmx
bmk makeapp -r -t console -l pico -g arm -board <board> -float-abi hard -o float_hard -x float_abi_benchmark.bmx
```

Each row reports the fastest of three timed one-million-call runs. Integer calls
are the control: a hard-float speedup should be concentrated in `float_call4`,
while `Double` remains software-assisted on RP2350.

## Pico Plus 2 result

Measured on a Pimoroni Pico Plus 2 at 150 MHz with Pico SDK 2.3.0 and the GCC
15.2.1 ARM toolchain. A second interleaved firmware run reproduced every result
within one microsecond.

| One million calls | softfp | hard | Hard-float effect |
| --- | ---: | ---: | ---: |
| Four `UInt` arguments | 106,666 us | 106,666 us | unchanged |
| Four `Float` arguments | 220,000 us | 153,333 us | 30.3% less time; 1.43x throughput |
| Four `Double` arguments | 1,106,666 us | 1,146,666 us | 3.6% more time |

The integer control confirms that the change is isolated to floating-point
calling convention. `Float` benefits substantially at a native-call boundary.
RP2350 has no double-precision FPU, and moving hard-ABI `Double` arguments back
to the core-register/stack form used by its software-assisted routines adds a
small cost. The benchmark firmware grew by 48 bytes and used no additional RAM.
