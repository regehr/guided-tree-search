#!/usr/bin/perl -w

use strict;
use File::Temp;
use File::Copy;

my $ALIVETV = "/home/regehr/alive2-regehr/build/alive-tv";

my $file = $ARGV[0];

open my $INF, "$ALIVETV --disable-undef-input $file |" or die;
my $ok = 0;
while (my $line = <$INF>) {
    chomp $line;
    # print "$line\n";
    $ok = 1 if ($line =~ /0 incorrect/);
}
close $INF;

if (!$ok) {
    my $filename = File::Temp::tempnam("/tmp", "bug_");
    print "$filename\n";
    File::Copy::copy($file, $filename);
}

exit(0);
