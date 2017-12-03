# DO NOT FORGET to define BITBOX environment variable 

USE_SDCARD = 1      # allow use of SD card for io
#DEFINES += DEBUG_CHIPTUNE

NAME = 16
# font files need to be first in order to be generated first:
C_FILES = font.c name.c run.c io.c map.c palette.c edit.c edit-properties.c go-sprite.c hit.c \
    save.c fill.c tiles.c sprites.c chiptune.c verse.c instrument.c anthem.c unlocks.c main.c
H_FILES = font.h name.h run.h io.h map.h palette.h edit.h edit-properties.h go-sprite.h hit.h \
    save.h fill.h tiles.h sprites.h chiptune.h verse.h instrument.h anthem.h unlocks.h common.h 
DEFINES += VGA_MODE=320

GAME_C_FILES=$(C_FILES:%=src/%)
GAME_H_FILES=$(H_FILES:%=src/%)

# see this file for options
BITBOX=bitbox
include $(BITBOX)/kernel/bitbox.mk

src/font.c: src/mk_font.py
	python2 src/mk_font.py

clean::
	rm -f src/font.c src/font.h

destroy:
	rm -f RECENT16.TXT *.*16

very-clean: clean destroy
