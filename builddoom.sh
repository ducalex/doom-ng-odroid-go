#!/bin/bash
make -j 4
release=`date +%Y%m%d`;
#../odroid-go-firmware/tools/mkfw/mkfw "Doom ($release)" tile.raw 0 16 1048576 app build/doom-esp32.bin 66 6 4198400 wad doom1-cut.wad
../odroid-go-firmware/tools/mkfw/mkfw "Doom ($release)" tile.raw 0 16 1572864 app build/doom-esp32.bin
mv firmware.fw Doom-alex.fw
