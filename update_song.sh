#!/bin/sh
cd tools/bin2var && make
echo updating ./src/rom_song.hpp
./bin2var ../../vgm/song.vgm >../../src/rom_song.hpp 

