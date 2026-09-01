# BlitzMax and MicroPython matched Pico benchmark

This benchmark compares the same deterministic workloads on the same Pico at
the same system-clock frequency. Each result is the best of three warmed runs.
Checksums must match before timing ratios are accepted.

The suite separates CPU work, calls, byte-buffer access, managed method calls,
object allocation plus explicit collection, and incremental String building.
The MicroPython program additionally reports Viper results for the two kernels
that map cleanly onto Viper's restricted machine types. Hardware-driver and PIO
benchmarks are deliberately excluded because their transfer or peripheral time
would dominate language execution overhead.

## BlitzMax

Build for the board being tested, adding `-x` to upload directly:

```sh
BlitzMax-pico/bin/bmk makeapp -a -r \
  -l pico -g arm -board pico \
  -o /tmp/bmx_micropython_benchmark \
  BlitzMax-pico/mod/pico.mod/benchmarks/micropython_comparison/matched_benchmark.bmx
```

Use `-board pico2` for Pico 2. Capture the USB serial lines from
`BENCH_HEADER` through `BENCH_DONE` into `blitzmax.txt`.

## MicroPython

Install the current MicroPython firmware for the same board, copy
`matched_benchmark.py` to the device, and run it without changing
`machine.freq()`. Confirm that its reported `cpu_hz` matches the BlitzMax
result. Capture its output into `micropython.txt`.

## Compare

```sh
python3 compare_results.py blitzmax.txt micropython.txt
```

Do not compare results from different clock frequencies, debug builds, or
different board models. USB serial output occurs outside every timed region.

## Recorded Pico 1 result

The checked-in baseline uses an original Pico at 125 MHz, release-mode
BlitzMax, and official MicroPython 1.29.0. All checksums match.

| Workload | BlitzMax | MicroPython | BlitzMax speedup |
| --- | ---: | ---: | ---: |
| Integer mix | 72,000 us | 43,785,093 us | 608.13x |
| Function calls | 496,000 us | 27,295,355 us | 55.03x |
| Byte buffer | 167,964 us | 13,142,674 us | 78.25x |
| Method calls | 7,519,207 us | 22,710,482 us | 3.02x |
| Object allocation and GC | 103,941 us | 929,584 us | 8.94x |
| String construction and GC | 45,985 us | 553,402 us | 12.03x |

MicroPython Viper completed the integer mix in 200,032 us and the byte-buffer
workload in 258,239 us. BlitzMax was respectively 2.78x and 1.54x faster.

## Recorded optimized Pico 1 result

After adding release-fast receiver checks, safepoint-driven root-frame
elision, and exact dispatch for `Final` methods and Types, the same non-final
dynamic-dispatch source was rebuilt and rerun on the same Pico. The original
result above is retained as the before-optimization control.

| Workload | Before | Optimized | Improvement | MicroPython speedup |
| --- | ---: | ---: | ---: | ---: |
| Integer mix | 72,000 us | 72,000 us | 1.00x | 608.13x |
| Function calls | 496,000 us | 36,000 us | 13.78x | 758.20x |
| Byte buffer | 167,964 us | 167,964 us | 1.00x | 78.25x |
| Method calls | 7,519,207 us | 86,400 us | 87.03x | 262.85x |
| Object allocation and GC | 103,941 us | 48,424 us | 2.15x | 19.20x |
| String construction and GC | 45,985 us | 50,354 us | 0.91x | 10.99x |

All optimized checksums match both the control BlitzMax run and MicroPython.
The unchanged integer and byte-buffer kernels provide useful controls: the
large call improvements come from removing unnecessary call-boundary work,
not from changing the benchmark workload. The small String variation should
be treated as a measurement to revisit rather than attributed to the call
optimization without further profiling.
