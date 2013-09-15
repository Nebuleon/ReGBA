TempGBA version 1.45, 2013-09-14

A Game Boy Advance emulator for the Supercard DSTWO.

Based on:
* gameplaySP (gpSP) 0.9 by Exophase, with help from various GBA developers
* gpSP Kai unofficial 3.3 by Takka
* The CATSFC GUI by ShadauxCat and Nebuleon (improving over BAGSFC)
  with language files contributed by the GBAtemp community for CATSFC

# Installing

(If you got the source code and want to compile it, see `modifying_en.txt`,
 included on GitHub as `doc/dstwo/modifying_en.txt`.)

To install the plugin to your storage card, copy `_dstwoplug` and `TEMPGBA`
from the release archive to the root of your storage card.

## The GBA BIOS

As of TempGBA version 1.45, an open-source BIOS replacement based on VBA-M's
high-level BIOS emulation is bundled in the release archive. This BIOS is
provided by Normmatt.

For better compatibility with certain games, you may instead use the real
GBA BIOS, if you have it.

The file you dump or find should be 16 KiB (16384 bytes) and have the following
checksums:
* CRC32: `81977335`;
* MD5: `a860e8c0b6d573d191e4ec7db1b1e4f6`;
* SHA-1: `300c20df6731a33952ded8c436f7f186d25d3492`.

Once dumped or found, name the file `gba_bios.bin`, and replace it in the
`TEMPGBA` directory of your storage card. So the path should look like this:
`/media/Your-Card/TEMPGBA/gba_bios.bin` on Linux; `E:\TempGBA\gba_bios.bin`
on Windows.

# Improving game compatibility with game_config.txt

Included with this release is a file called game_config.txt. The file is the
same as the one included in gpSP Kai. It goes in the `TEMPGBA` directory on
your storage card when you extract the release.

This file contains information to enhance game compatibility in two ways.
* Firstly, it tells TempGBA about the save type used by a certain game, so that
  battery-backed saves may be read and written correctly.
* Secondly, it tells TempGBA about idle loops in a certain game, so that it may
  stop emulating when the game reaches them. It runs the game faster.

game_config.txt receives updates from the community occasionally.
For the latest version, see:
<http://filetrip.net/nds-downloads/flashcart-files/latest-x-f12630.html>

# Using NDSGBA saved states

The NDSGBA saved state format was not suitable for modifying the emulator,
because each vew version meant that older saved states could corrupt the new
version's memory.

To play a NDSGBA saved state, you may do the following:

1. Load the version of NDSGBA you used to create your saved state.
2. Load the game, then the saved state. Save in-game, then exit NDSGBA.
   (Exiting NDSGBA is what creates the .sav file. TempGBA would not need
    to exit.)
3. Using your computer, copy the .sav file from the /NDSGBA/GAMERTS directory
   on your storage card into /TEMPGBA/SAVES. You can do this step for multiple
   games at once if you repeated steps 1 and 2 for them.
4. Load TempGBA.
5. If you want to create a saved state (a .sav file works too), load the game,
   which will load the .sav file, and then create a TempGBA saved state.

# Game compatibility

The GBATemp community maintains a compatibility list for TempGBA. You can
see and edit it here: <http://wiki.gbatemp.net/wiki/TempGBA_Compatibility>

# Cheats

The cheat support in TempGBA is untested, but should be equivalent to that of
the official NDSGBA, version 1.21.

# Rewinding

You can rewind the game up to 10 steps of a user-defined duration, ranging from
a quarter of second to 10 seconds. You are granted 100 seconds of rewinding by
default, in steps of 10 seconds, using the button combination L+Y. This allows
you to quickly back out of a dangerous situation in a game.

You can adjust the step time for rewinding in the Tools menu.

You can adjust the hotkey used for rewinding in the Tools menu's hotkey
submenus. See the `Hotkeys` section below for more information about these.

# Frame skipping

In the Video & audio menu, the **Frame skipping** option allows you to select
a number of frames to skip between rendered frames.

For some games, you may need to adjust frame skipping.
* If a game runs at about 10 frames per second, setting frame skipping
  to 1 will allow you to jump, move or shoot at the right times.
* If you want to show more frames per second in a game that already shows 20,
  setting frame skipping to 1 or 0 will cause more frames to appear.
  If using frame skipping 0 and the audio crackles too much, the current game
  may crash the Supercard DSTwo itself, and may require rebooting your Nintendo
  DS.
* Setting this to 10 will skip 10 frames and render one. You will find yourself
  unable to perform actions during the correct frame with the DS buttons. It is
  advised to set frame skipping to the lowest value with which you can play a
  game.

# Hotkeys

**Important: Hotkeys are resolved as DS keys, not remapped GBA keys.**
You can set hotkeys to use buttons that are left over from button remapping if
you don't use the rapid fire A and B buttons.

You can set buttons to press to perform certain actions. For each action,
there is a *global hotkey* and a *game-specific override hotkey*. You might,
for example, want to have the DS R button bound to Rewind, but a specific game
uses a button mapped to it (see the section `Button mapping` below for more
information) for something important. In that case, you can set the
global hotkey to R and make an override with X for that game.

Hotkeys are sent to the current game, after being remapped, as well as
to their corresponding action. The criterion for a hotkey is met when
**at least** all of its buttons are held. Additional keys are sent to the game
and can trigger another hotkey. For example, setting a hotkey to L and another
to R+X, then pressing L+R+X+Y will trigger both and send all GBA buttons mapped
to L, R, X and Y to the game.

Available actions are:
* Go to main menu. In addition to tapping the Touch Screen to return to
  the main menu, you can set a hotkey to do the same.
* Temporary fast-forward. While this hotkey is held, the fast-forward option
  will be forced on.
* Rewind. A hotkey needs to be set here if you want to use the rewind feature
  in a game. After each second the hotkey is held, the game will rewind one
  step. See the `Rewinding` section above for more information.
* Toggle sound. Each time this hotkey is held, the sound will be disabled if
  it's currently enabled, and vice-versa.
* Save state #1. Each time this hotkey is held, saved state #1 will be written,
  without confirmation if it exists.
* Load state #1. Each time this hotkey is held, saved state #1 will be loaded.

# Button mapping

A Nintendo DS has more buttons than a Game Boy Advance. This emulator allows
you to remap your GBA buttons to different DS buttons as well as use rapid fire
A and B buttons that trigger at 30 Hz (held for one frame, released the next).

You can find the button mapping configuration in the Tools menu. For each
mapping, there is a *global mapping* and a *game-specific mapping override*.
This allows you to keep your usual mapping, but change it if you are more
comfortable with another mapping for another game; for example, a platformer
which is best played with Y and B, or a re-release of a game from the SFC/SNES
which you would like to play with the original controller arrangement.

Buttons are sent to the GBA after mappings are applied.

The default is to map DS buttons to the like-named GBA buttons:
A to A, B to B, L to L, R to R, Start to Start and Select to Select. The DS
buttons X and Y are unused; you can set them to be rapid fire buttons or use
them as hotkeys.

When a game-specific mapping is present, it cancels a global mapping for the
same GBA button, *not* the same DS button.

Here's an example:

`Global mappings                     |  Game-specific overrides`

`GBA key A      = DS key A           |  GBA key A      = DS key B`

`GBA key B      = DS key B           |  GBA key B      = DS key Y`

`GBA key Start  = DS key Start       |  GBA key Start  = No override`

`GBA key Select = DS key Select      |  GBA key Select = No override`

`GBA key L      = DS key L           |  GBA key L      = DS key R`

`GBA key R      = DS key R           |  GBA key R      = No override`

`Rapid fire A   = DS key X           |  Rapid fire A   = No override`

`Rapid fire B   = DS key Y           |  Rapid fire B   = No override`

Here's what each DS button would do in this example:

* DS button A would not send any GBA buttons.
* DS button B would send GBA button A.
* DS button X would send GBA button A in rapid fire mode.
* DS button Y would send GBA button B **and** GBA button B in rapid fire mode.
  The behavior is undefined in this case; it may decide to send button B always
  or only in rapid fire mode. The solution is to override *Rapid fire B*.
* DS button L would not send any GBA buttons.
* DS button R would send GBA buttons L **and** R. You may want to do this, but
  if you didn't, the solution would be to map DS button L to GBA button R or
  remove the override on GBA button L.
* Select and Start behave as expected.
