#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/imported_type_memory" \
	"$module_root/examples/imported_type_memory.bmx"

test -s "$work_dir/imported_type_memory.elf"
test -s "$work_dir/imported_type_memory.uf2"

generated_dir="$module_root/examples/.bmx/imported_type_memory.release.pico.arm.pico2/generated"
bootstrap="$generated_dir/blitzmax_pico_modules.c"
test -s "$bootstrap"

state_line="$(grep -n '    (void)__bb_pico_tests_importedtypes_support_state_' "$bootstrap" | cut -d: -f1)"
counter_line="$(grep -n '    (void)__bb_pico_tests_importedtypes_types_counter_' "$bootstrap" | cut -d: -f1)"
module_line="$(grep -n '    (void)__bb_pico_tests_importedtypes_importedtypes();' "$bootstrap" | cut -d: -f1)"
derived_line="$(grep -n '    (void)__bb_pico_tests_derivedtypes_derivedtypes();' "$bootstrap" | cut -d: -f1)"
test "$state_line" -lt "$counter_line"
test "$counter_line" -lt "$module_line"
test "$module_line" -lt "$derived_line"

producer_unit_count="$(grep -l '^int __bb_pico_tests_importedtypes_' "$generated_dir"/module_*.c | wc -l | tr -d ' ')"
test "$producer_unit_count" -eq 3

application_c="$generated_dir/imported_type_memory.c"
grep -q '^struct pico_tests_importedtypes_TImportedCounter_obj {$' "$application_c"
grep -q 'BMXPicoObject object;' "$application_c"
grep -q '_pico_tests_importedtypes_timportedcounter_value;' "$application_c"
grep -q '_pico_tests_importedtypes_timportedcounter_label;' "$application_c"
grep -q '_pico_tests_importedtypes_timportedcounter_peer;' "$application_c"
grep -q '_pico_tests_importedtypes_timportedcounter_samples;' "$application_c"
grep -q 'struct pico_tests_importedtypes_TCounterShape _pico_tests_importedtypes_timportedcounter_shape;' "$application_c"
grep -q 'struct pico_tests_importedtypes_TCounterMetadata _pico_tests_importedtypes_timportedcounter_metadata;' "$application_c"
grep -q 'struct pico_tests_importedtypes_TCounterMetadata _pico_tests_importedtypes_timportedcounter_checkpoints\[2\];' "$application_c"
grep -q 'bmx_pico_object_assert.*->_pico_tests_importedtypes_timportedcounter_value' "$application_c"
grep -q '^struct pico_tests_derivedtypes_TModuleCounter_obj {$' "$application_c"
grep -q '_pico_tests_derivedtypes_tmodulecounter_bonus;' "$application_c"
grep -q '_pico_tests_derivedtypes_tmodulecounter_note;' "$application_c"
grep -q '_pico_tests_derivedtypes_tmodulecounter_history;' "$application_c"
grep -q 'struct bmx_cls.*TLeafCounter_obj {' "$application_c"
grep -q '\.super = &bmx_pico_imported_type_.*TModuleCounter' "$application_c"
grep -q 'TModuleCounter = .*\.super = &bmx_pico_imported_type_.*TImportedCounter' "$application_c"
grep -q '(BMXPicoMethod)_pico_tests_importedtypes_TImportedCounter_Current' "$application_c"
grep -q '(BMXPicoMethod)_pico_tests_derivedtypes_TModuleCounter_Label' "$application_c"
grep -q 'static const BMXPicoInterfaceEntry .*TLeafCounter_interfaces\[2\]' "$application_c"
grep -q 'bmx_pico_interface_methods' "$application_c"
grep -q '_pico_tests_derivedtypes_TModuleCounter_New__Bint__Bstring__Bint((struct pico_tests_derivedtypes_TModuleCounter_obj \*)bmx_self_self' "$application_c"
grep -q '_pico_tests_derivedtypes_TModuleCounter_Add__Bint((struct pico_tests_derivedtypes_TModuleCounter_obj \*)bmx_self_self' "$application_c"
grep -q '_pico_tests_derivedtypes_TModuleCounter_PicoBaseFinalize(object);' "$application_c"
grep -q 'offsetof.*_pico_tests_importedtypes_timportedcounter_label' "$application_c"
grep -q 'offsetof.*_pico_tests_importedtypes_timportedcounter_peer' "$application_c"
grep -q 'offsetof.*_pico_tests_importedtypes_timportedcounter_samples' "$application_c"
grep -q 'offsetof.*_pico_tests_derivedtypes_tmodulecounter_note' "$application_c"
grep -q 'offsetof.*_pico_tests_derivedtypes_tmodulecounter_history' "$application_c"
grep -q 'BMX_PICO_ARRAY_ELEMENT_VALUE, bbStructElementInit_pico_tests_importedtypes_TCounterShape, 0' "$application_c"
grep -q 'BMX_PICO_ARRAY_ELEMENT_VALUE, bbStructElementInit_pico_tests_importedtypes_TCounterMetadata, &bmx_pico_imported_value_' "$application_c"
grep -q 'BMX_PICO_VALUE_STRUCT, &bmx_pico_imported_value_.*TCounterMetadata' "$application_c"
grep -q 'sizeof(struct pico_tests_importedtypes_TCounterMetadata), 2, BMX_PICO_VALUE_STRUCT' "$application_c"
grep -q '_fixedMetadata\[0\].*BMX_PICO_ROOT_STRUCT' "$application_c"
grep -q '_fixedMetadata\[1\].*BMX_PICO_ROOT_STRUCT' "$application_c"
grep -q 'pico_tests_importedtypes_CreateCounterShape__Bint__Bint' "$application_c"
grep -q 'pico_tests_importedtypes_ResizeCounterShape__NTCounterShapeEV__Bint__Bint' "$application_c"

producer_c="$(grep -l 'bmx_pico_type_.*TImportedCounter_references' "$generated_dir"/module_*.c)"
test -n "$producer_c"
grep -q 'offsetof.*_pico_tests_importedtypes_timportedcounter_label' "$producer_c"
grep -q 'offsetof.*_pico_tests_importedtypes_timportedcounter_peer' "$producer_c"
grep -q 'offsetof.*_pico_tests_importedtypes_timportedcounter_samples' "$producer_c"
grep -q '^void _pico_tests_importedtypes_TImportedCounter_PicoBaseInit' "$producer_c"
grep -q '^void _pico_tests_importedtypes_TImportedCounter_PicoBaseFinalize' "$producer_c"
grep -q '^void bbStructElementInit_pico_tests_importedtypes_TCounterShape' "$producer_c"
grep -q '^void bbStructElementInit_pico_tests_importedtypes_TCounterMetadata' "$producer_c"

shared_c="$(grep -l 'bmx_global_.*sharedMetadata\[2\]' "$generated_dir"/module_*.c)"
test -n "$shared_c"
grep -q 'bmx_global_.*sharedMetadata\[0\].*BMX_PICO_ROOT_STRUCT' "$shared_c"
grep -q 'bmx_global_.*sharedMetadata\[1\].*BMX_PICO_ROOT_STRUCT' "$shared_c"

derived_c="$(grep -l 'bmx_pico_type_.*TModuleCounter_references' "$generated_dir"/module_*.c)"
test -n "$derived_c"
grep -q 'interface_count = 2' "$derived_c"
grep -q 'offsetof.*_pico_tests_importedtypes_timportedcounter_peer' "$derived_c"
grep -q 'offsetof.*_pico_tests_derivedtypes_tmodulecounter_history' "$derived_c"
grep -q '^void _pico_tests_derivedtypes_TModuleCounter_PicoBaseInit' "$derived_c"
grep -q '^void _pico_tests_derivedtypes_TModuleCounter_PicoBaseFinalize' "$derived_c"

derived_interface="$module_root/tests.mod/derivedtypes.mod/derivedtypes.release.pico.arm.i"
grep -q '^TModuleCounter\^TImportedCounter{' "$derived_interface"
grep -q '^\.bonus%&' "$derived_interface"
grep -q '^\.note\$&' "$derived_interface"
grep -q '^\.history%&\[\]&' "$derived_interface"
grep -q 'fixedMetadata:TCounterMetadata&\[2\]&' "$derived_interface"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/imported_type_memory.elf" | awk 'NR == 2')
test "$text_size" -le 55000
test "$bss_size" -ge 19000
test "$bss_size" -le 22000

echo "Pico imported-Type module image: text=$text_size bss=$bss_size"
