#!/bin/bash
make
release=`date +%Y%m%d`;
#../odroid-go-firmware/tools/mkfw/mkfw "Doom ($release)" tile.raw 0 16 1048576 app build/esp32-doom.bin 66 6 4198400 wad doom1-cut.wad
../odroid-go-firmware/tools/mkfw/mkfw "Doom ($release)" tile.raw 0 16 1048576 app build/esp32-doom.bin 
mv firmware.fw Doom-standalone.fw
