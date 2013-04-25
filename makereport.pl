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

sub make_plot
{
    my ($output, $title, $max, $data) = @_;
    open D, ">/tmp/$output.dat" or die "Can't open /tmp/$output.dat: $!\n";
    print D $data;
    close D;
    open P, ">/tmp/$output.plot" or die "Can't open /tmp/$output.plot: $!\n";
    print P <<PEOF;
set terminal png small size 200,70 xffffff x444444 xffffff x888888
set border 0
set noytics
set noxtics
#set ylabel "ASes reachable"
#set xlabel "hops"
#set key right top
set nokey
set boxwidth 0.7 relative
set style fill solid 1.0
set yrange [0:$max]
PEOF
    $n = 2;
    foreach $col (@cols) {
#	print P "set title \"$col\"\n";
	print P "set output \"$prefix-$output-$col.png\"\n";
	print P "plot \"/tmp/$output.dat\" using 1:$n with boxes\n";
        $n++;
    }
    close P;

    `gnuplot /tmp/$output.plot`;
    `rm /tmp/$output.dat /tmp/$output.plot`;
}

$prefix = $ARGV[0];
if(!$prefix) { $prefix = 'ascompare'; }

open HTML,">$prefix.html" or die "Can't open $prefix.html: $!\n";
print HTML <<HEOF;
<html>
<head><title>AS comparison</title></head>
<body>
HEOF

while(<STDIN>) {
    $raw .= $_;

#    if(/^AS\s+(\d+)\s+\((.*?)\)/) { $name{$1} = $2; }
    if(/^OUTPUT:\s+(.*?)$/) {
	$output = $1;
	unshift @outputs, $output;
    }
    if(/^NAME:\s+(.*?)$/) {

	if($data && $title) {
	    make_plot($output, $title, $max, $data);
	}

	$title = $1;
	$data = '';
    }
    if(/^MAX:\s+(.*?)$/) {
	$max = $1;
    }
    if(/^hops/) {
	chomp;
	@cols = split(/\s+/, $_);
	shift @cols;
    }
    if(/^\s+\d/) {
	s/^\s+//;
	s/ +/\t/g;
	$data .= $_;
    }
}

make_plot($output, $title, $max, $data);

print HTML <<HEOF;
<h3>Results</h3>

<p>Histograms and cumulative distributions for the number of AS hops
required to reach the Internet from a given set of networks.  In all
graphs, x-axis is number of hops.  Y-axes are:
<ul>
<li>as-hist: number of ASes reachable in exactly x hops
<li>as-cum: percentage of all ASes reached within x hops
<li>24-hist: number of /24s reachable in exactly x hops
<li>24-cum: percentage of all /24s reached within x hops
</ul>
In all cases, the more mass there is on the left side of the graph,
the better the network is in terms of connectivity.
</p>

<table><tr><td><b>AS-GROUP</b></td>
HEOF
@routputs = reverse(@outputs);
$prefix =~ s/.*\///;
foreach $output (@routputs) {
    print HTML "<td><center><b>$output</b></center></td>";
}
print HTML "</tr>\n";
foreach $col (@cols) {
    print HTML "<tr><td><b>$col</b></td>";
    foreach $output (@routputs) {
	print HTML "<td><img src=\"$prefix-$output-$col.png\"/></td>\n";
    }
    print HTML "</tr>\n";
}
print HTML <<HEOF;
</table>
<h3>Raw output</h3>
<pre>
$raw
</pre>
</body>
</html>
HEOF
close HTML;
