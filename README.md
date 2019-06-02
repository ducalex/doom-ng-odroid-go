# Description

This is an improvement of [doom-ng-odroid-go](https://github.com/mad-ady/doom-ng-odroid-go/) which itself is an improvement of [doom-odroid-go](https://github.com/OtherCrashOverride/doom-odroid-go/).

## New features

- Full sound support
- Save games
- Brightness controls
- Volume controls
- WAD selector on start


# Usage

Copy Doom-*.fw to your sdcard folder `odroid/firmware` and PRBOOM.WAD and DOOM.WAD to `roms/doom`. Power up the Odroid GO while holding B. Then flash Doom from the menu. You can add more than one WAD at a time, a prompt will be shown on power up.

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


### Cheats:

Key | Action
-|-
MENU+UP      | 
MENU+DOWN    |
MENU+LEFT    |  
MENU+RIGHT   | 
VOLUME+UP    |
VOLUME+DOWN  | 
VOLUME+LEFT  |
VOLUME+RIGHT | 



# To do / Known issues

- Proper mixing. Currently the music is mixed in the audio stream as if it were a sound effect,  it causes the volume to go down when actual sound effects are playing.

- Fix the options menu and support its sound levels. (Mostly done but not yet gamepad compatible).

- Cheat menu instead of the weird key combinations.

- Fix the random lags. It might be caused by SD card access on the shared bus.

- Fix the memory allocation failures due to memory fragmentation (especially when switching levels)

- Make it possible to delete savegames.

- Support mods and expansions such as SIGIL


# Compilation

