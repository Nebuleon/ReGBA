#!/usr/bin/perl
#
# any2ucs.pl
#
# This Perl script allows you to generate an ISO10646-1 encoded
# BDF font from another BDF font in any possible encoding. It was
# based on the ucs2any.pl script by Markus Kuhn.

use strict 'subs';

sub is_control {
    my ($source) = @_;

    return (($source >= 0x00 && $source <= 0x1f) ||
            ($source >= 0x7f && $source <= 0x9f));
}

# calculate the bounding box that covers both provided bounding boxes
sub combine_bbx {
    my ($awidth, $aheight, $axoff, $ayoff,
        $cwidth, $cheight, $cxoff, $cyoff) = @_;

    if ($axoff < $cxoff) {
        $cwidth += $cxoff - $axoff;
        $cxoff = $axoff;
    }
    if ($ayoff < $cyoff) {
        $cheight += $cyoff - $ayoff;
        $cyoff = $ayoff;
    }
    if ($awidth + $axoff > $cwidth + $cxoff) {
        $cwidth = $awidth + $axoff - $cxoff;
    }
    if ($aheight + $ayoff > $cheight + $cyoff) {
        $cheight = $aheight + $ayoff - $cyoff;
    }

    return ($cwidth, $cheight, $cxoff, $cyoff);
}

print <<End if $#ARGV < 0;

Usage: any2ucs.pl <source-name> <mapping-file>

where

   <source-name>        is the name of the source BDF file

   <mapping-file>       is the name of a character set table like those on
                        <ftp://ftp.unicode.org/Public/MAPPINGS/>

Example:

   any2ucs.pl 6x13.bdf 8859-7.TXT

will generate the file 6x13-iso10646-1.bdf
from the iso-8859-7 encoded file 6x13.bdf

End

exit if $#ARGV < 0;

# open and read source file
$fsource = $ARGV[0];
open(FSOURCE,  "<$fsource")  || die ("Can't read file '$fsource': $!\n");

# read header
$properties = 0;
$default_char = 0;
while (<FSOURCE>) {
    last if /^CHARS\s/;
    if (/^STARTFONT/) {
        $startfont = $_;
    } elsif (/^_XMBDFED_INFO\s/ || /^_XFREE86_GLYPH_RANGES\s/) {
        $properties--;
    } elsif (/DEFAULT_CHAR\s+([0-9]+)\s*$/) {
        $default_char = $1;
        $header .= "DEFAULT_CHAR 0\n";
    } else {
        if (/^STARTPROPERTIES\s+(\d+)/) {
            $properties = $1;
        } elsif (/^FONT\s+(.*-([^-]*-\S*))\s*$/) {
            if ($2 eq "ISO10646-1") {
                die("Font seems to already be ISO10646-1!\n");
            };
        } elsif (/^CHARSET_REGISTRY\s+"(.*)"\s*$/) {
            if ($1 eq "ISO10646") {
                die("Font seems to already be ISO10646-1!\n");
            };
            $registry = $1;
        } elsif (/^CHARSET_ENCODING\s+"(.*)"\s*$/) {
            $encoding = $1;
        } elsif (/^SLANT\s+"(.*)"\s*$/) {
            $slant = $1;
            $slant =~ tr/a-z/A-Z/;
        } elsif (/^SPACING\s+"(.*)"\s*$/) {
            $spacing = $1;
            $spacing =~ tr/a-z/A-Z/;
        }
        s/^COMMENT\s+\"(.*)\"$/COMMENT $1/;
        s/^COMMENT\s+\$[I]d: (.*)\$\s*$/COMMENT Derived from $1\n/;
        $header .= $_;
    }
}

die ("No STARTFONT line found in '$fsource'!\n") unless $startfont;
die ("No CHARSET_REGISTRY line found in '$fsource'!\n") unless defined($registry);
die ("No CHARSET_ENCODING line found in '$fsource'!\n") unless defined($encoding);
$header =~ s/\nSTARTPROPERTIES\s+(\d+)\n/\nSTARTPROPERTIES $properties\n/;

# read characters
while (<FSOURCE>) {
    if (/^STARTCHAR/) {
        $sc = $_;
        $code = -1;
    } elsif (/^ENCODING\s+(-?\d+)/) {
        $code = $1;
        $startchar{$code} = $sc;
        $char{$code} = "";
    } elsif (/^ENDFONT$/) {
        $code = -1;
        $sc = "STARTCHAR ???\n";
    } else {
        $char{$code} .= $_;
        if (/^ENDCHAR$/) {
            $code = -1;
            $sc = "STARTCHAR ???\n";
        }
    }
}
close FSOURCE;
delete $char{-1};

shift @ARGV;

$fmap = $ARGV[0];

# open and read source file
open(FMAP,  "<$fmap")
    || die ("Can't read mapping file '$fmap': $!\n");
%map = ();
while (<FMAP>) {
    next if /^\s*(\#.*)?$/;
    if (/^\s*(0[xX])?([0-9A-Fa-f]{4}|[0-9A-Fa-f]{2})\s+(0[xX]|U\+|U-)?([0-9A-Fa-f]{4})/) {
        $target = hex($4);
        $source = hex($2);
        if (!is_control($source)) {
            if ($startchar{$source}) {
                $map{$target} = $source;
            } else {
                printf STDERR "No glyph for character 0x%04x " .
                    "(U+%04X) available.\n", $source, $target;
            }
        }
    } else {
        printf STDERR "Unrecognized line in '$fmap':\n$_";
    }
}
close FMAP;

# add default character
if (!(defined($map{0}) && $startchar{$map{0}})) {
    if (defined($default_char) && $startchar{$default_char}) {
        $map{0} = $default_char;
        $startchar{$default_char} = "STARTCHAR defaultchar\n";
    } else {
        printf STDERR "No default character defined.\n";
    }
}

# list of characters that will be written out
@chars = sort {$a <=> $b} keys(%map);
if ($#chars < 0) {
    print STDERR "No characters found for $registry-$encoding.\n";
};

# find overall font bounding box
undef @bbx;
for $target (@chars) {
    $source = $map{$target};
    if ($char{$source} =~ /^BBX\s+(\d+)\s+(\d+)\s+(-?\d+)\s+(-?\d+)\s*$/m) {
        if (defined @bbx) {
            @bbx = combine_bbx(@bbx, $1, $2, $3, $4);
        } else {
            @bbx = ($1, $2, $3, $4);
        }
    } else {
        printf STDERR "Warning: No BBX found for 0x%04x!\n", $source;
    }
}

# generate output file name
if ($fsource =~ /^(.*).bdf$/i) {
    $fout = $1 . "-iso10646-1.bdf";
} else {
    $fout = $fsource . "-iso10646-1";
}
$fout =~ s/^(.*\/)?([^\/]+)$/$2/;  # remove path prefix

# write new BDF file
printf STDERR "Writing %d characters into file '$fout'.\n", $#chars + 1;
open(FOUT,  ">$fout")
    || die ("Can't write file '$fout': $!\n");

print FOUT $startfont;
print FOUT "COMMENT AUTOMATICALLY GENERATED FILE. DO NOT EDIT!\n";
print FOUT "COMMENT Generated with 'any2ucs.pl $fsource $fmap'\n";
print FOUT "COMMENT from a(n) $registry-$encoding encoded source BDF font.\n";
print FOUT "COMMENT any2ucs.pl is based on ucs2any.pl by Markus Kuhn <mkuhn\@acm.org>, 2000.\n";
$newheader = $header;
$newheader =~
    s/^FONTBOUNDINGBOX\s+.*$/FONTBOUNDINGBOX @bbx/m
        || print STDERR "Warning: FONTBOUNDINGBOX not fixed!\n";
$newheader =~
    s/^FONT\s+(.*)-\w+-\w+\s*$/FONT $1-ISO10646-1/m
        || print STDERR "Warning: FONT property not fixed!\n";
$newheader =~
    s/^CHARSET_REGISTRY\s+.*$/CHARSET_REGISTRY "ISO10646"/m
        || print STDERR "Warning: CHARSET_REGISTRY not fixed!\n";
$newheader =~
    s/^CHARSET_ENCODING\s+.*$/CHARSET_ENCODING "1"/m
        || print STDERR "Warning: CHARSET_ENCODING not fixed!\n";
print FOUT $newheader;
printf FOUT "CHARS %d\n", $#chars + 1;

# Write characters
for $target (@chars) {
    $source = $map{$target};
    print FOUT $startchar{$source};
    print FOUT "ENCODING $target\n";
    print FOUT $char{$source};
}

print FOUT "ENDFONT\n";

close(FOUT);
