import gc
import machine
import micropython
import time

INTEGER_ITERATIONS = 1_000_000
CALL_ITERATIONS = 500_000
BUFFER_SIZE = 1024
BUFFER_ROUNDS = 500
METHOD_ITERATIONS = 300_000
ALLOCATION_WIDTH = 64
ALLOCATION_ROUNDS = 100
STRING_WIDTH = 128
STRING_ROUNDS = 50
MASK32 = 0xFFFFFFFF


class BenchCounter:
    def __init__(self, initial):
        self.value = initial

    def step(self, value):
        self.value = (self.value * 1664525 + value + 1013904223) & MASK32
        return self.value


class BenchAllocation:
    def __init__(self, value):
        self.value = value


def mix_step(value):
    return (value * 1664525 + 1013904223) & MASK32


def integer_mix(iterations):
    value = 0x12345678
    for _ in range(iterations):
        value = (value * 1664525 + 1013904223) & MASK32
    return value


def function_calls(iterations):
    value = 0x12345678
    for _ in range(iterations):
        value = mix_step(value)
    return value


def buffer_mix(buffer, rounds):
    total = 0
    length = len(buffer)
    for round_index in range(rounds):
        for index in range(length):
            value = (index * 13 + round_index) & 0xFF
            buffer[index] = value
            total = (total + value) & MASK32
    return total


def method_calls(iterations):
    counter = BenchCounter(0x12345678)
    value = 0
    for index in range(iterations):
        value = counter.step(index)
    return value


def object_alloc_gc(rounds):
    objects = [None] * ALLOCATION_WIDTH
    total = 0
    for round_index in range(rounds):
        for index in range(ALLOCATION_WIDTH):
            item = BenchAllocation(round_index + index)
            objects[index] = item
            total = (total + item.value) & MASK32
        for index in range(ALLOCATION_WIDTH):
            objects[index] = None
        item = None
        gc.collect()
    return total


def string_build_gc(rounds):
    total = 0
    for _ in range(rounds):
        text = ""
        for _ in range(STRING_WIDTH):
            text += "x"
        total += len(text)
        text = None
        gc.collect()
    return total


@micropython.viper
def integer_mix_viper(iterations: int) -> uint:
    value: uint = uint(0x12345678)
    multiplier: uint = uint(1664525)
    increment: uint = uint(1013904223)
    index: int = 0
    while index < iterations:
        value = value * multiplier + increment
        index += 1
    return value


@micropython.viper
def buffer_mix_viper(buffer, rounds: int) -> uint:
    data = ptr8(buffer)
    length: int = int(len(buffer))
    total: uint = uint(0)
    round_index: int = 0
    while round_index < rounds:
        index: int = 0
        while index < length:
            value: uint = uint((index * 13 + round_index) & 0xFF)
            data[index] = value
            total += value
            index += 1
        round_index += 1
    return total


def measured(function, *arguments):
    gc.collect()
    started = time.ticks_us()
    checksum = function(*arguments)
    elapsed = time.ticks_diff(time.ticks_us(), started)
    return elapsed, int(checksum) & MASK32


def best_of_three(function, *arguments):
    best = None
    checksum = 0
    for _ in range(3):
        elapsed, checksum = measured(function, *arguments)
        if best is None or elapsed < best:
            best = elapsed
    return best, checksum


def report(engine, name, work, result):
    elapsed, checksum = result
    print("BENCH,{},{},{},{},{},{}".format(
        engine, machine.freq(), name, work, elapsed, checksum
    ))


buffer = bytearray(BUFFER_SIZE)

# Warm up decorators, allocation paths, and the USB serial connection.
integer_mix(1000)
function_calls(1000)
buffer_mix(buffer, 1)
method_calls(1000)
integer_mix_viper(1000)
buffer_mix_viper(buffer, 1)
gc.collect()

print("BENCH_HEADER,engine,cpu_hz,test,work_units,microseconds,checksum")
report("micropython", "integer_mix", INTEGER_ITERATIONS,
       best_of_three(integer_mix, INTEGER_ITERATIONS))
report("micropython", "function_calls", CALL_ITERATIONS,
       best_of_three(function_calls, CALL_ITERATIONS))
report("micropython", "byte_buffer", BUFFER_SIZE * BUFFER_ROUNDS,
       best_of_three(buffer_mix, buffer, BUFFER_ROUNDS))
report("micropython", "method_calls", METHOD_ITERATIONS,
       best_of_three(method_calls, METHOD_ITERATIONS))
report("micropython", "object_alloc_gc", ALLOCATION_WIDTH * ALLOCATION_ROUNDS,
       best_of_three(object_alloc_gc, ALLOCATION_ROUNDS))
report("micropython", "string_build_gc", STRING_WIDTH * STRING_ROUNDS,
       best_of_three(string_build_gc, STRING_ROUNDS))
report("micropython-viper", "integer_mix", INTEGER_ITERATIONS,
       best_of_three(integer_mix_viper, INTEGER_ITERATIONS))
report("micropython-viper", "byte_buffer", BUFFER_SIZE * BUFFER_ROUNDS,
       best_of_three(buffer_mix_viper, buffer, BUFFER_ROUNDS))
print("BENCH_DONE")
