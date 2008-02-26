#! /usr/bin/perl

use strict;
use warnings;

use Getopt::Long;
use List::Util qw/sum/;

my ($do_ordered);

GetOptions(
    'ordered' => \$do_ordered,
);


my @cnt = map { 0 } 0..255;

while (<>) {
    foreach my $c (split '', $_) {
        $cnt[ord $c]++;
    }
}

if ($do_ordered) {
    my @order = sort { $cnt[$b] <=> $cnt[$a] } 0..255;
    print "#define USE_ORDERED_TABLE 1\n";
    print "static unsigned char from_ordered[] = {", join(',', @order), "};\n";
    my %r = map { $order[$_] => $_ } @order;
    print "static unsigned char to_ordered[] = {", join(',', map { $r{$_} } 0..255), "};\n";
    @cnt = map { $cnt[$order[$_]] } 0..255;
}

my @freq;
my $acc = 0;
my $cc = sum @cnt;
for (my $i = 0; $i < 256; $i++) {
    push @freq, $acc;
    $acc += int(($cnt[$i] / $cc) * 0xfe00);
}
push @freq, $acc;

print "static short freq[] __attribute__((aligned(16))) = {", join(',', map { $_ - 0x8000 } @freq), "};\n";
