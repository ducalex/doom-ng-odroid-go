#!/bin/sh

release=`date +%Y%m%d`;
idf.py app
./tools/mkfw.py "Doom ($release)" tile.raw 0 16 1572864 app build/doom-esp32.bin
mv firmware.fw Doom-$release.fw
