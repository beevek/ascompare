#!/usr/bin/perl

#   Copyright 2008-2013 Kristopher R Beevers and Internap Network
#   Services Corporation.

#   Permission is hereby granted, free of charge, to any person
#   obtaining a copy of this software and associated documentation files
#   (the "Software"), to deal in the Software without restriction,
#   including without limitation the rights to use, copy, modify, merge,
#   publish, distribute, sublicense, and/or sell copies of the Software,
#   and to permit persons to whom the Software is furnished to do so,
#   subject to the following conditions:

#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.

#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
#   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
#   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#   SOFTWARE.

# convert an aslinks file from CAIDA into an edge list

open FILE,"$ARGV[0]" or die "Can't open $ARGV[0]: $!\n";

while(<FILE>) {
    if(/^D\s+([\d\_\,]+)\s+([\d\_\,]+)/) {
	$as_from_raw = $1;
	$as_to_raw   = $2;
	$w           = 1;
    } elsif(/^I\s+([\d\_\,]+)\s+([\d\_\,]+)\s+(\d+)/) {
	$as_from_raw = $1;
	$as_to_raw   = $2;
	$w           = $3;
    } else {
	next;
    }

    @as_from = split(/[\_\,]/, $as_from_raw);
    @as_to   = split(/[\_\,]/, $as_to_raw);

    foreach $from (@as_from) {
	foreach $to (@as_to) {
	    print "$from $to $w\n";
	}
    }
}

close FILE;
