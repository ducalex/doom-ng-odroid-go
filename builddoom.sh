#!/bin/bash
idf.py app
release=`date +%Y%m%d`;
../odroid-go-firmware/tools/mkfw/mkfw "Doom ($release)" tile.raw 0 16 1572864 app build/doom-esp32.bin
mv firmware.fw Doom-alex.fw
