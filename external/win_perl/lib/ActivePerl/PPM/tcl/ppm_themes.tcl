# ppm_themes.tcl --
#
#	This file implements package ...
#
# Copyright (c) 2006 ActiveState Software Inc
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

package require img::png
package require tile
package require treectrl ; # for imagetint
package provide ppm::themes 1.0

namespace eval ::ppm {
    variable IMGDIR [file dirname [file dirname [info script]]]/images

    variable IMG ; # array of creates images
    variable MOD ; # array of modifier images
    variable MAP ; # array mapping of semantic name -> image file
    array set IMG {}
    array set MOD {
	install		{bullet_add.png}
	remove		{bullet_delete.png}
	upgrade		{bullet_add.png}
	upgradable	{bullet_star.png}
	filter		{zoom_corner.png}
	modified	{bullet_go.png}
    }
    array set MAP {
	dir		{folder.png}
	package		{package.png}
	available	{package_disabled.png}
	installed	{package.png}
	refresh		{refresh.png}
	config		{cog.png}
	gecko		{gecko.png}
        perl            {perl_48x48.png}
	add		{add.png}
	delete		{delete.png}
	accept		{accept.png}
	go		{go.png}
	radio-on	{radio-on.gif}
	radio-off	{radio-off.gif}
	locked		{lock.png}
	unlocked	{lock_open.png}
    }
}
namespace eval ::ppm::img {
    # namespace for image commands
}

proc ::ppm::img {what args} {
    variable MAP
    variable MOD
    variable IMG
    if {[info exists MAP($what)]} {
	set file $MAP($what)
    } elseif {[info exists MOD($what)]} {
	set file $MOD($what)
    } else {
	set file $what
    }
    set key [join [linsert $args 0 $what] /]
    if {![info exists IMG($key)]} {
	variable IMGDIR
	if {![file isdirectory $IMGDIR] || ![file exists $IMGDIR/$file]} {
	    return -code error \
		"unable to find image '$IMGDIR/$file' for '$what'"
	}
	set IMG($key) [image create photo ::ppm::img::$key -file $IMGDIR/$file]
	foreach mod $args {
	    if {$mod eq "grey" || $mod eq "gray"} {
		imagetint $IMG($key) gray 200
	    } else {
		set mimg [img $MOD($mod)]
		$IMG($key) copy $mimg; # overlay modifier
	    }
	}
    }
    return $IMG($key)
}

proc ::ppm::imgname {img} {
    # get regular name from image made up of [list $what ?$mod ...?]
    return [split [namespace tail $img] /]
}
