#!/usr/bin/perl -w

use strict;
use File::Find;

my %dirs = ("/home/regehr/fuzz_results_llc/resync" => "resync.txt",
            "/home/regehr/fuzz_results_llc/no-resync" => "no-resync.txt",
            "/home/regehr/tmp" => "default.txt");
my $reps = 16;
my $block_size = 300;

my $good;
my $bad;
my %count;
my %total_edges;

sub wanted {
    if ($_ eq "plot_data") {
        my $fn = $File::Find::name;
        print "$fn\n";
        open my $INF, "<$fn" or die;
        my $line = <$INF>;
        while ($line = <$INF>) {
            chomp $line;
            my @s = split (/,/, $line);
            if (scalar(@s) == 13) {
                ++$good;
                my $time = $s[0];
                my $edges = $s[12];
                # print "'$edges'\n";
                if (exists($count{$time})) {
                    $count{$time} += 1;
                    die unless exists($total_edges{$time});
                    $total_edges{$time} += $edges;
                } else {
                    $count{$time} = 1;
                    $total_edges{$time} = $edges;
                }
            } else {
                ++$bad;
            }
        }
        close $INF;
    }
}

sub bynum {
    return $a <=> $b;
}

sub go($) {
    (my $dir) = @_;
    $good = 0;
    $bad = 0;
    %count = ();
    %total_edges = ();
    my %block_total;
    my %block_count;
    finddepth(\&wanted, $dir);
    print "$good good lines, $bad bad lines\n";
    foreach my $t (keys %count) {
        my $block = $block_size * int($t / $block_size);
        if (!(exists($block_count{$block}))) {
            $block_count{$block} = 0;
            $block_total{$block} = 0;
        }
        $block_count{$block} += $count{$t};
        $block_total{$block} += $total_edges{$t};
    }
    open my $OUTF, ">$dirs{$dir}" or die;
    foreach my $k (sort bynum keys %block_count) {
        my $avg = (0.0 + $block_total{$k}) / $block_count{$k};
        my $hours = $k / (60 * 60.0);
        print $OUTF "$hours $avg\n";
    }
    close $OUTF;
}

foreach my $dir (keys %dirs) {
    go($dir);
}

open my $GP, '|-', 'gnuplot';

print {$GP} <<'__GNUPLOT__';
set terminal pdf
set output "plot.pdf"
plot "resync.txt" with lines, "mutate.txt" with lines, "default.txt" with lines
__GNUPLOT__

close $GP;
