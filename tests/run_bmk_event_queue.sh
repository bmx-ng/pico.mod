#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 32k \
		-o "$work_dir/event-queue-$board" \
		"$module_root/tests/fixtures/event_queue_compile.bmx"

	test -s "$work_dir/event-queue-$board.elf"
	test -s "$work_dir/event-queue-$board.uf2"
done
