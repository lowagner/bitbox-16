# DO NOT FORGET to define BITBOX environment variable 

USE_SDCARD = 1      # allow use of SD card for io
#DEFINES += DEBUG_CHIPTUNE

NAME = 16
# font files need to be first in order to be generated first:
GAME_C_FILES = font.c name.c run.c io.c map.c palette.c edit.c edit-properties.c go-sprite.c \
    save.c fill.c tiles.c sprites.c chiptune.c verse.c instrument.c anthem.c main.c
GAME_H_FILES = font.h name.h run.h io.h map.h palette.h edit.h edit-properties.h go-sprite.h \
    save.h fill.h tiles.h sprites.h chiptune.h verse.h instrument.h anthem.h common.h 
GAME_C_OPTS += -DVGAMODE_320

# see this file for options
include $(BITBOX)/lib/bitbox.mk

font.c: font_generator.py
	python2 font_generator.py

clean::
	rm -f font.c font.h

destroy:
	rm -f RECENT16.TXT *.*16

very-clean: clean destroy
