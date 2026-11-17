## RIPscrip: load 1I icon files from disk

A BBS that sends a RIP `1I` (load icon) command now gets the icon drawn, if the file is on the local disk. Before this change, the vtcon engine had no file reader and skipped every `1I` command.

### Lookup order

1. `<DownloadPath>/<board name>/<icon>`: the board name is the download host the sink already uses (`local` when none is set).
2. `PROGDIR:Icons/<icon>`: a shared folder next to the program.

The first file found wins. The engine passes a bare, sanitised name; the reader also refuses any name holding `/` or `:`, so a board cannot reach outside these two folders. Reads are capped at 4 MB.

### Changes

- `src/vtdownload.c/.h`: `VtDownload_Read` and `VtDownload_Release`, beside the existing download sink. Paths are built with `AddPart`; files are read with `Open`/`Seek`/`Read`.
- `src/vtbuiltin.c`: `VtOpen` registers the reader with `vt_set_file_reader` and the buffer release with `vt_set_image_decoder(NULL, release)`. The decoder stays NULL, so kitty `f=100` remains an error as before. No unlinker is set, so kitty `t=t` cannot delete files.
- `src/DCTelnet.c`: the `--replay` report prints `icons=<n>` (from `vt_rip_icons`).
- Tests: the dos shim gains `Read`/`Seek`/`MODE_OLDFILE`. `test_vtdownload` covers the lookup order, offset/size, path refusal and the fallback. `test_term_engine` feeds `tests/fixtures/rip-icon.rip` through `VtWrite`, as `--replay` does, and checks the icon counter and the drawn pixels. The test fails with the `vt_set_file_reader` line removed.

### Testing

- `make -C src host-ci`: green.
- Not yet run on an Amiga: `DCTelnet --replay rip-icon.rip` with `TEST.ICN` in `<DownloadPath>/local/` or `PROGDIR:Icons/` should print `icons=1` and draw the 8x2 icon at (4,3).
