#!/usr/bin/env python3

import csv
import sys


def load(path):
    results = {}
    with open(path, "r", encoding="utf-8") as source:
        for row in csv.reader(source):
            if not row or row[0] != "BENCH" or len(row) != 7:
                continue
            engine, cpu_hz, name, work, microseconds, checksum = row[1:]
            results[(engine, name)] = {
                "cpu_hz": int(cpu_hz),
                "work": int(work),
                "us": int(microseconds),
                "checksum": int(checksum),
            }
    return results


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: compare_results.py blitzmax.txt micropython.txt")

    results = load(sys.argv[1])
    results.update(load(sys.argv[2]))
    names = sorted(name for engine, name in results if engine == "blitzmax")

    print("test                 BlitzMax us  MicroPython us  speedup  checksums")
    print("-------------------  -----------  --------------  -------  ---------")
    for name in names:
        blitzmax = results.get(("blitzmax", name))
        python = results.get(("micropython", name))
        if not blitzmax or not python:
            continue
        clocks_match = blitzmax["cpu_hz"] == python["cpu_hz"]
        checksums_match = blitzmax["checksum"] == python["checksum"]
        speedup = python["us"] / blitzmax["us"] if blitzmax["us"] else float("inf")
        status = "ok" if checksums_match else "MISMATCH"
        if not clocks_match:
            status += ", CLOCKS DIFFER"
        print(f"{name:19}  {blitzmax['us']:11d}  {python['us']:14d}  {speedup:6.2f}x  {status}")

    for name in ("integer_mix", "byte_buffer"):
        blitzmax = results.get(("blitzmax", name))
        viper = results.get(("micropython-viper", name))
        if not blitzmax or not viper:
            continue
        speedup = viper["us"] / blitzmax["us"] if blitzmax["us"] else float("inf")
        status = "ok" if blitzmax["checksum"] == viper["checksum"] else "MISMATCH"
        print(f"{name + ' (Viper)':19}  {blitzmax['us']:11d}  {viper['us']:14d}  {speedup:6.2f}x  {status}")


if __name__ == "__main__":
    main()
