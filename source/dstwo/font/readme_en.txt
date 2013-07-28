In this directory, you will find the source file for the main font used by
TempGBA. It's an Adobe BDF file, which is fed into the emulator when running
in "font dump" mode to produce a more efficient representation in an "ODF"
format. The font is based on the one used by Pictochat, with a few more
characters that Pictochat does not have (but no Japanese characters).

You can edit the font in an application that reads BDF bitmap fonts, such as
FontForge. Open the font in the application then export it again as BDF.
One case where you would want to do this is to add new glyphs to support a
new language.

To include the more efficient representation (ODF) in TEMPGBA/system after
editing the BDF file:

 1. If your font added characters beyond U+2193 DOWNWARDS ARROW, adjust the
    maximum codepoint in source/nds/bdf_font.c, after the first instance of
  > #ifndef HAVE_ODF
 2. In source/nds/bdf_font.c,
  > #define DUMP_ODF
    and
  > // #define HAVE_ODF
    This will make the plugin read the BDF source and write an ODF file.
 3. make
 4. Copy the new plugin to your card, under /_dstwoplug.
 5. Copy the .bdf file to your card, under /TEMPGBA/system, as
    Pictochat-16.bdf.
 6. Run the plugin on the Supercard DSTWO. It will briefly load, then display
    "Font library initialisation error -1, press any key to exit". This is
    because it tries to load the Chinese font's source, Song.bdf, which you
    don't have. Regardless, it does dump an ODF file for Pictochat-16.bdf.
 7. Copy the .odf font somewhere on your hard drive if you want to keep a copy
    of it. Delete the .bdf file from your card.
 8. Reverse the changes made in step 2.
 9. make
10. Copy the new plugin to your card, under /_dstwoplug.

And you can use your new font!

Finally, you may want to send your .bdf source file to a TempGBA developer
or commit it to a fork on Github, for inclusion in the plugin. You may also
want to send your changes to TEMPGBA/system/language.msg for the same reason.