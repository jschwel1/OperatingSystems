#!/usr/bin/tclsh

set charset [split "abcdefghijklmnopqrstuvwxyz" ""];

lassign $argv lines len

for {set i 0} {$i < $lines} {incr i} {
	for {set j 0} {$j < $len} {incr j} {
		lappend output [lindex $charset [expr int(rand()*[llength $charset])]]
	}
	puts [join $output ""]
	set output ""
}
