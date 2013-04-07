TempGBA and PokéGBA beta version 8, 2013-04-02

A Game Boy Advance emulator for the Supercard DSTWO.

Based on:
* gameplaySP (gpSP) 0.9 by Exophase, with help from various GBA developers
* gpSP Kai unofficial 3.3 by Takka
* The CATSFC GUI by ShadauxCat and Nebuleon (improving over BAGSFC)
  with language files contributed by the GBAtemp community for CATSFC

# Installing

(If you got the source code and want to compile it, see the `Compiling` section
 at the end of the file.)

To install the plugin to your storage card, copy `tempgba.plg`, `tempgba.ini`
and `tempgba.bmp` from the release archive to the card's `_dstwoplug`
directory. Then, copy the `TEMPGBA` subdirectory to the root of the card.

## The GBA BIOS

To function properly, TempGBA needs a dump of the GBA's BIOS. It is **not**
distributed in the release archive because of legal issues. Do not ask the
author where to find this file; any questions regarding the BIOS will be
quickly ignored. You, the user, are responsible for your use of the BIOS.

The file you dump or find should be 16 KiB (16384 bytes) and have the following
checksums:
* CRC32: `81977335`;
* MD5: `a860e8c0b6d573d191e4ec7db1b1e4f6`;
* SHA-1: `300c20df6731a33952ded8c436f7f186d25d3492`.

Once dumped or found, name the file `gba_bios.bin`, and place it in the
`TEMPGBA` directory of your storage card. So the path should look like this:
`/media/Your-Card/TEMPGBA/gba_bios.bin` on Linux; `E:\TempGBA\gba_bios.bin`
on Windows.

# TempGBA versus PokéGBA

For technical reasons related to audio rendering, it is better if you use
PokéGBA to play Pokémon games, and TempGBA to play other games.

* If you use PokéGBA to play Golden Sun - The Lost Age or some GBA Video
  cartridges, you will hear the audio overlap itself or slow down massively.
* If you use TempGBA to play Pokémon games, the music will appear to
  incorrectly synchronise some parts of itself with the others after about
  5 minutes of gameplay, becoming rather annoying after 10 minutes.

Saved states made by one should be readable by the other, but any audio
rendering glitch applicable to the other version will reappear in it.

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

# Cheats

The cheat support in TempGBA is untested, but should be equivalent to that of
the official NDSGBA, version 1.21.

# Rewinding

You can rewind the game up to 14 steps of a user-defined duration, ranging from
a quarter of second to 10 seconds. You are granted 140 seconds of rewinding by
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
  setting frame skipping to 1 or 0 will cause more frames to appear,
  but your DS button input may stop responding for 2 entire seconds every so
  often. The audio may also stop suddenly, be broken multiple times per second
  or simply crackle more often.
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

# The font

The font used by TempGBA is similar to the Pictochat font. To modify it,
see `source/font/README.txt`.

# Translations

Translations for TempGBA may be submitted to the author(s) under many forms,
one of which is the Github pull request. To complete a translation, you will
need to do the following:

* Open `TEMPGBA/system/language.msg`.
* Copy what's between `STARTENGLISH` and `ENDENGLISH` and paste it at the end
  of the file.
* Change the tags. For example, if you want to translate to Japanese, the tags
  will become `STARTJAPANESE` and `ENDJAPANESE`.
* Translate each of the messages, using the lines starting with `#MSG_` as a
  guide to the context in which the messages will be used.

If you are not comfortable editing C code, or cannot compile TempGBA after
changes, you may instead test your translation in the English block and submit
it. That allows you to look for message length issues and to align the option
names and values to your liking with spaces.

If you wish to also hook your language into the user interface, you will need
to do the following:

* Edit `source/nds/message.h`. Find `enum LANGUAGE` and add the name of your
  language there. For the example of Japanese, you would add this at the end of
  the list:
  ```
	,
	JAPANESE
  ```
* Still in `source/nds/message.h`, just below `enum LANGUAGE`, you will find
  `extern char* lang[` *some number* `]`. Add 1 to that number.
* Edit `source/nds/gui.c`. Find `char *lang[` *some number* `] =`.
  Add the name of your language, in the language itself. For the example of
  Japanese, you would add this at the end of the list:
  ```
	,
	"日本語"
  ```
* Still in `source/nds/gui.c`, find `char* language_options[]`, which is below
  the language names. Add an entry similar to the others, with the last number
  plus 1. For example, if the last entry is `, (char *) &lang[7]`, yours would
  be `, (char *) &lang[8]`.
* Still in `source/nds/gui.c`, find `case CHINESE_SIMPLIFIED`. Copy the lines
  starting at the `case` and ending with `break`, inclusively. Paste them
  before the `}`. Change the language name and tags. For the example of
  Japanese, you would use:
  ```
	case JAPANESE:
		strcpy(start, "STARTJAPANESE");
		strcpy(end, "ENDJAPANESE");
		break;
  ```

Compile again, copy the plugin and your new `language.msg` to your card
under `TEMPGBA/system`, and you can now select your new language in TempGBA!

# Compiling

Compiling TempGBA is best done on Linux. Make sure you have access to a Linux
system to perform these steps.

## The DS2 SDK
To compile TempGBA, you need to have the Supercard team's DS2 SDK.
The Makefile expects it at `/opt/ds2sdk`, but you can move it anywhere,
provided that you update the Makefile's `DS2SDKPATH` variable to point to it.

For best results, download version 0.13 of the DS2 SDK, which will have the
MIPS compiler (`gcc`), extract it to `/opt/ds2sdk`, follow the instructions,
then download version 1.2 of the DS2 SDK and extract its files into
`opt/ds2sdk`, overwriting version 0.13.

Additionally, you will need to add the updated `zlib`, DMA
(Direct Memory Access) and filesystem access routines provided by BassAceGold
and recompile `libds2a.a`. To do this:

> sudo rm -r /opt/ds2sdk/libsrc/{console,core,fs,key,zlib,Makefile} /opt/ds2sdk/include
> sudo cp -r sdk-modifications/{libsrc,include} /opt/ds2sdk
> sudo chmod -R 600 /opt/ds2sdk/{libsrc,include}
> sudo chmod -R a+rX /opt/ds2sdk/{libsrc,include}
> cd /opt/ds2sdk/libsrc
> sudo rm libds2a.a ../lib/libds2a.a
> sudo make

## The MIPS compiler (`gcc`)
You also need the MIPS compiler from the DS2 SDK.
The Makefile expects it at `/opt/mipsel-4.1.2-nopic`, but you can move it
anywhere, provided that you update the Makefile's `CROSS` variable to point to
it.

## Making the plugin
To make the plugin, `tempgba.plg`, use the `cd` command to change to the
directory containing your copy of the TempGBA source, then type
`make clean; make`. `tempgba.plg` should appear in the same directory.
