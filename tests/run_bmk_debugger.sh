#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
source_file="$module_root/examples/debugger_smoke.bmx"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT
debug_output="$work_dir/new/debug/output/debugger-smoke"
release_output="$work_dir/new/release/output/debugger-smoke-release"

"$bmk" makeapp -a -d -l pico -g arm -board pico -heap 16k \
	-o "$debug_output" "$source_file"

debug_build="$module_root/examples/.bmx/debugger_smoke.debug.pico.arm.pico"
generated_c="$debug_build/bcc/application/application.c"
test -s "$debug_output.elf"
test -s "$debug_output.uf2"
grep -Eq '#line [0-9]+ ".*/debugger_smoke\.bmx"' "$generated_c"
grep -q 'bmx_pico_debug_stop' "$generated_c"
grep -q 'int32_t counter' "$generated_c"
grep -q 'BMXPicoString \* message' "$generated_c"
grep -q 'int32_t debugConditional' "$generated_c"
if grep -Eq 'bmx_v[0-9_]*_(counter|message|debugConditional)' "$generated_c"; then
	echo "Pico debug build exposed generated rather than source variable names" >&2
	exit 1
fi
grep -q -- '-O0' "$debug_build/build.ninja"

"$bmk" makeapp -a -r -l pico -g arm -board pico -heap 16k \
	-o "$release_output" "$source_file"

release_c="$module_root/examples/.bmx/debugger_smoke.release.pico.arm.pico/bcc/application/application.c"
test -s "$release_output.elf"
if grep -q '#line ' "$release_c"; then
	echo "release Pico build unexpectedly contains GDB source mappings" >&2
	exit 1
fi

echo "Pico -d debugger build passed (BMX source lines, -O0, DebugStop, source-named locals, release isolation)"
