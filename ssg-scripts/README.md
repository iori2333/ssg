# Script assets

The `ssg-scripts` static library embeds the ECL, SCL, and localized text
catalogs used by the game. Its CMake target builds `script_tool`, converts the
source assets, and compiles the generated lookup tables into the library.

`script_tool` is also available in `build/bin/` for manual conversion:

```sh
script_tool disasm-scl <in_binary> <out_text>
script_tool asm-scl <in_text> <out_binary>
script_tool disasm-ecl <in_binary> <out_text>
script_tool asm-ecl <in_text> <out_binary>
script_tool asm-text <in_text> <out_binary>
```

SCL uses `MSGREF id=@key` for localized messages. Catalogs are stored under
`i18n/<language>/`:

```text
messages.txt  story dialogue
ui.txt        menus and dialogs
music.txt     Music Room titles and comments
```

Supported escapes are `\xNN`, `\\`, `\"`, `\n`, `\r`, and `\t`. The text
assembler rejects invalid syntax, duplicate keys, and 32-bit text ID
collisions. The build validates that every supported language has the same key
set.
