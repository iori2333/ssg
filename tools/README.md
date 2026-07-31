# Tools

## pack_tool

`pack_tool` extracts and repacks PBG archives. It intentionally exposes only
two commands:

```sh
pack_tool extract <packfile> <out_dir>
pack_tool pack <in_dir> <packfile>
```

Build output is `build/bin/pack_tool.exe` on Windows and
`build/bin/pack_tool` on Linux.

### Generic archives

Extracting a generic PBG archive writes contiguous numeric entries:

```text
work/
  000.bin
  001.bin
  ...
```

Packing a directory with this layout creates a generic PBG archive. Entry
numbers must start at zero and have no gaps.

### Unified game data

The game loads an extracted `data/` directory when one exists next to the
executable. If the directory is absent, it loads `data.pak` instead. Its
extracted layout is:

```text
data/
  maps/      000.map ... 006.map
  images/    000.bmp ... 038.bmp
  music/     000.mid ... 019.mid
  music-arranged/
              000.mid ... 019.mid
  sounds/    000.wav ... 019.wav
  demos/     000.dat ... 005.dat
```

The maps section contains Stage 1–6 followed by Extra. The demos section uses
the same Replay v2 PBG format as files in `replays/`; it does not have a
separate header or input encoding. Each demo must contain exactly one stage,
and `000.dat`–`005.dat` must correspond to Stage 1–6. The section may be empty
or contain a contiguous prefix while demos are being regenerated.

When all six section directories are present, `pack` writes a versioned
manifest as archive entry 0 and appends the sections in manifest order. Music
entries in both variants must be raw SMF files beginning with `MThd`; titles
and Music Room comments are compiled from
`scripts/i18n/<language>/music.txt` instead.

```sh
pack_tool extract bin/data.pak work/data
pack_tool pack work/data bin/data.pak
```

For migration only, extracting a legacy music archive recognizes entries in
the old `[title][comment][MIDI]` wrapper and emits raw MIDI files. Repacking
never recreates the wrapper.

### Regenerating demos

Use the Debug menu recorder. It maintains an unsaved deterministic pre-roll
from the start of the stage, but only marks and displays recording after the
capture key is pressed:

1. Enable Debug → Demo Recording.
2. Start a normal game. To jump to a particular stage, hold its number key
   (`1`–`6`) while confirming the weapon with `Z`.
3. At the desired visible starting point, press `R`. The recording indicator
   appears from this point onward.
4. Press Escape and choose Exit to finish. The game writes the complete Replay
   v2 file to `bin/demos/000.dat` through `bin/demos/005.dat`. `data.pak` is not
   modified.
5. Title-screen Demo Play loads local files before corresponding embedded
   demos.
6. After approving the recordings, extract `data.pak`, copy the local files
   into the extracted `demos/` directory, and explicitly repack it.

`pack_tool` rejects files without the visible-start marker, multi-stage files,
wrong stage IDs, non-Replay data, and gaps in demo numbering.

```powershell
build\bin\pack_tool.exe extract bin\data.pak build\data_work
Copy-Item bin\demos\*.dat build\data_work\demos\
build\bin\pack_tool.exe pack build\data_work bin\data.pak
```

## script_tool

`script_tool` assembles and disassembles ECL/SCL programs and compiles i18n
text catalogs. Build output is `build/bin/script_tool(.exe)`.

```sh
script_tool disasm-scl <in_binary> <out_text>
script_tool asm-scl <in_text> <out_binary>
script_tool disasm-ecl <in_binary> <out_text>
script_tool asm-ecl <in_text> <out_binary>
script_tool asm-text <in_text> <out_binary>
```

SCL uses `MSGREF id=@key` for localized messages. The corresponding catalogs
are stored separately under `scripts/i18n/<language>/`:

```text
messages.txt  story dialogue
ui.txt        menus and dialogs
music.txt     Music Room titles and comments
```

All catalogs share the same key/value syntax. Single-line values use quoted
strings. Multi-line values use triple quotes on their own lines:

```text
ui.menu.game_start.title = "Game Start"

music.track_00.comment = """
First line

Third line
"""
```

Supported escapes are `\xNN`, `\\`, `\"`, `\n`, `\r`, and `\t`. The text
assembler rejects invalid syntax, duplicate keys, and 32-bit text ID
collisions. The build validates that every supported language has the same key
set.
