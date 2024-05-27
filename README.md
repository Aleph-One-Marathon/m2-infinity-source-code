# Marathon® 2/Infinity source code

This repository contains the source code released by Bungie™️ for Marathon 2 and Marathon Infinity, available at [infinitysource.bungie.org](https://infinitysource.bungie.org/). This is intended as an accessible reference where the original engine code can be read or linked from bug reports or other Marathon-related discussions. For a continuation of this code which runs on modern computers, see the [Aleph One](https://github.com/Aleph-One-Marathon/alephone) project.

A limited commit history was reconstructed from the two releases, integrating earlier versions and obsolete files present in the Infinity source archive. Select the `marathon2` tag to view the Marathon 2 code.

## License

All source code here can be used under the GPL 3 license. Marathon 2 source was originally released under GPL 2, and all files visible at the `marathon2` tag can be used under either license.

## Marathon 2 source code edits

When Marathon 2’s source code was released in 2000, some components of the game engine were still commercially relevant, and were excluded from the public release. These included serial number generation and the multi-game `cseries.lib` code. Based on file dates in the original archive, the following files were edited for release:

* game_dialogs.c
* game_wad.c
* interface.c
* interface.h
* makefile
* player.c
* preferences.c
* shell.c

The 2011 release of Infinity’s source was more comprehensive, and presumably matches the commercial product. When comparing across commits, keep in mind the commit tagged `marathon2` does not fully reflect Marathon 2 as shipped.

## Files omitted from this archive

For clarity, only Bungie-authored source code and makefiles have been included in this repository. The full releases included binary data files, headers from Apple and other third parties, and other items of historical interest. For those items, which are not GPL-licensed, see the original archives at [infinitysource.bungie.org](https://infinitysource.bungie.org/) or here in the Releases section.