# Description

This is an improvement of [doom-ng-odroid-go](https://github.com/mad-ady/doom-ng-odroid-go/) which itself is an improvement of [doom-odroid-go](https://github.com/OtherCrashOverride/doom-odroid-go/).

## New features

- Full sound support
- Save games
- Brightness controls
- Volume controls
- WAD selector on start
- Mods support (Limited by available memory)
- Cheats menu (Options -> Cheats)
- Levels menu (Options -> Level Select)

# Usage

Copy Doom-*.fw to your SD Card folder `odroid/firmware` and PRBOOM.WAD and DOOM.WAD to `roms/doom`. Power up the Odroid GO while holding B. Then flash Doom from the menu. You can add more than one WAD at a time, a prompt will be shown on power up.

# Controls

Key    | Action
-------|--
A      | Fire
B      | Strafe
START  | Action/Open doors
SELECT | Change weapon
VOLUME | Map
MENU   | Menu
START+UP    | Brightness up
START+DOWN  | Brightness down
START+LEFT  | Sound down
START+RIGHT | Sound up


# To do / Known issues

- Fix the random lags. It might be caused by SD card access on the shared bus.
- Fix the memory allocation failures due to memory fragmentation (especially when switching levels)
- Make it possible to delete savegames.
- Add support for external DAC

# Compilation

ESP-IDF version 4.0.1 or greater is required to compile.

- `idf.py app`
- `./tools/mkfw.py doom.fw "Doom" tile.raw 0 0 0 app build/doom-esp32.bin`
