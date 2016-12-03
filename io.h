#ifndef IO_H
#define IO_H

#include <stdint.h>

typedef enum {
    NoError = 0,
    MountError,
    ConstraintError,
    OpenError,
    ReadError,
    WriteError,
    NoDataError,
    MissingDataError,
    BotchedIt
} FileError;

FileError io_init();
FileError io_set_recent_filename();
FileError io_get_recent_filename();
void io_message_from_error(uint8_t *msg, FileError error, int save_not_load);
FileError io_save_palette();
FileError io_load_palette();
FileError io_save_tile(unsigned int i);
FileError io_load_tile(unsigned int i);
FileError io_save_sprite(unsigned int i, unsigned int f);
FileError io_load_sprite(unsigned int i, unsigned int f);
FileError io_save_map();
FileError io_load_map();
FileError io_save_instrument(unsigned int i);
FileError io_load_instrument(unsigned int i);
FileError io_save_verse(unsigned int i);
FileError io_load_verse(unsigned int i);
FileError io_save_anthem();
FileError io_load_anthem();
FileError io_save_go(unsigned int i);
FileError io_load_go(unsigned int i);
FileError io_save();
FileError io_load();


/* G16 file format
a bunch of curly brace patterns, with nested groups in square brackets
NIX the comments in the format, require fixed length stuff.  you can 
reorganize some things (like brackets or numberings), but number of bytes between actual
data must remain the same.

G16
{IN
 0[
   0:0g  # this comment is not allowed.  offset: 8 bytes, data at byte 13 and 14
   1:S5  # offset 16 bytes, data at 21 and 22
   ...
   f:0g  # offset 128 bytes, data at 133, 134
 0] # offset 136
 1[ # offset 140
   0:0g  # break has permitted values 00 to 0g.  (g == 0)
   1:S8  # side has permitted values S0 to Sf, S8 is centered, S0 is silent, S1 = L, Sf = R
   2:V3  # volume can be V0 to Vf
   3:#=  # waveform: #s (sine), #^ (triangle), #/ (saw), #_ (square), #= (white noise), #& (red noise), #! (violet noise), #* (anything, space, blank)
   4:C0  # note.  permitted values C0 to Cf (C0 = C, Cf = Eb octave above)
   5:Wg  # wait, permitted values W1 to Wg
   6:<0  # crescendo, values <0 to <f
   7:>3  # decrescendo, values >1 to >g
   8:if  # inertia, note slides, i0 to if
   9:~f  # vibrato, ~0 to ~f.  2 bits for rate, 2 bits for amplitude
   a:/7  # bend up, /0 to /7.  bend down: \ from \1 to \8
   b:B3  # bitcrush, B0 to Bf
   c:D1  # duty, D0 to Df
   d:d3  # duty delta, d0 to df
   e:Ra  # randomize, permitted values: Ra (any), Ro (odd), Re (even), Rs (scattered, 1, 8, 15), Rl (low), Rm (mid), Rh (high), Rb (book case, top or bottom), RL (very low), RK (mid-low), RJ (mid-hi), RH (very high), R0 (remainder zero by four), R1 (remainder1), R2 (remainder 2), R3 (remainder 3)
   f:J3  # jump, permitted values 0 to f
 1]
 ...
 f[    # 16th instrument, last one ends a bit different
   ...
 f]
}IN
{VS
 0[
  0:0g 0g 0g 0g  break or wait until quarter note
  1:O3 O3 O3 O3  octave (O0 to O6), absolute octave (O=), wrap (/1 \1 or /2 \2) or up/down only (+1 -1 +2 -2)
  2:I3 I3 I3 I3  instrument, I0 to If
  3:V3 V3 V3 V3  volume, V0 to Vf
  4:V3 C3 C3 C3  note, C0 (C) to Cf (Eb next octave up)
  5:V3 W3 W3 W3  wait, W1 to Wg
  6:N0 N0 N0 N0  note+wait, N0 to Nf.  2 bits for relative note, 2 bits for wait
  ...
  v:0g 0g 0g 0g
 0]
...
 f[
   ...
 f]
}VS
{Ag      # song length 'g' (= 16), valid g to w (=32).
 0:1111  # 0 to f possible for each of 4 instrument players.  errors otherwise.
 1:2222  
   ...   
 f:6666  # you can provide the index number (0-v) if you like, just use colon.
   ...
 v:0000  # NOTE THAT we fill out the anthem up to 32 positions long, even if we don't use them all
}Ag
{PC # palette colors
  0:v33  # RGB, from 0 to v each
  1:5cf  # mind only TWO spaces at start here.
  2:234
  3:680 
  5:bbb
  ...
  f:506
}PC
{TL
 0[
M0 T0 t0 WARP3f 999
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0]
 1[
M0 T0 t0 Jf(fff,ff)
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 1]
 2[
M0 T1 t0 Af S5 R3 D
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000100000000000
 0000000000000000
 0000000000000000
 0000000000100000
 0010000000000000
 0000000000000000
 0000000000000001
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 0000000000000000
 2]
 3[
M8 T0 t0 Vf 1 1 1 1
 ...
 3]
 ...
}TL
{SP
0R[ Ig W3 If Vf 1 1 1 1
 ...
0R]
0r[
 ...
0r]
...
}SP
{GO

}GO
*/


#endif
