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

# convert as-info.txt from CAIDA into an AS list we can read

open FILE,"$ARGV[0]" or die "Can't open $ARGV[0]: $!\n";

@lines = <FILE>;
$data = join '', @lines;

$data =~ s/\n\t/\t/g;

@lines = split(/\n/, $data);

foreach (@lines) {
    if(/^(\d+)\t(\d+)\t(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*?)\t(.*?)/) {
	$asn = $2; $name = $3; $c24 = $5; $cp = $6; $ca = $7;
	$name =~ s/\s+$//;
	$name =~ s/\s+/-/g;
	if(length $name < 2) { $name = 'UNKNOWN'; }
	$c24 =~ s/\,//g;
	$cp =~ s/\,//g;
	$ca =~ s/\,//g;
	print "$asn $name $c24 $cp $ca\n";
    } else { 
	next;
    }
}

close FILE;

