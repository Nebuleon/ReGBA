#!/usr/bin/perl

# ログファイル名を指定
$log = "build_count.inc";
$/ = " ";
open(IN,"$log");
$define = <IN>;
$count = <IN>;
close(IN);

$count++;

open(OUT,"> $log");
print OUT "BUILD_COUNT= ";
print OUT $count;
print OUT "\n";
close(OUT);
