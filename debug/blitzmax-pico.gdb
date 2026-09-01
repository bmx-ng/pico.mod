set pagination off
set print pretty on
set breakpoint pending on

# DebugStop is a harmless no-op without GDB. Use a plain hardware breakpoint
# here: debugger adapters such as Cortex-Debug cannot reliably drive GDB's
# interactive multi-line `commands` facility. The calling BlitzMax frame is
# immediately below this marker in the call stack.
break bmx_pico_debug_stop

define bmx-string
  set $bmx_string = (BMXPicoString *)$arg0
  if $bmx_string == 0
    printf "Null\n"
  else
    printf "String(length=%d, utf16=[", $bmx_string->length
    set $bmx_index = 0
    while $bmx_index < $bmx_string->length && $bmx_index < 64
      if $bmx_index != 0
        printf " "
      end
      printf "%04x", $bmx_string->buf[$bmx_index]
      set $bmx_index = $bmx_index + 1
    end
    if $bmx_index < $bmx_string->length
      printf " …"
    end
    printf "])\n"
  end
end
document bmx-string
Show a BlitzMax String's length and up to 64 UTF-16 code units.
Usage: bmx-string stringExpression
end

define bmx-array
  set $bmx_array = (BMXPicoArray *)$arg0
  if $bmx_array == 0
    printf "Null\n"
  else
    printf "Array(length=%d, elementSize=%u, kind=%u, data=%p)\n", $bmx_array->length, $bmx_array->element_size, $bmx_array->element_kind, ((unsigned char *)$bmx_array + ((sizeof(BMXPicoArray) + 15) & ~15))
  end
end
document bmx-array
Show a BlitzMax Array's length, element layout, kind, and data address.
Element kind 0 is value, 1 is String, and 2 is Object.
Usage: bmx-array arrayExpression
end

define bmx-object
  set $bmx_object = (BMXPicoObject *)$arg0
  if $bmx_object == 0
    printf "Null\n"
  else
    printf "%s at %p\n", $bmx_object->type->name, $bmx_object
  end
end
document bmx-object
Show the dynamic BlitzMax type name and address of an Object.
Use GDB's normal print command on a typed variable to see its fields.
Usage: bmx-object objectExpression
end

echo BlitzMax Pico GDB helpers loaded\n
