# arealist.tcl --
#
#	This file implements package arealist, which  ...
#
# Copyright (c) 2006 ActiveState Software Inc
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

package require snit
package require treectrl
package require style::as
package require widget::scrolledwindow
package require tooltip
package provide ppm::arealist 1.0

snit::widgetadaptor arealist {

    component tree

    delegate option -borderwidth to hull
    delegate option -relief to hull
    delegate option -padding to hull
    delegate option * to tree
    delegate method * to tree

    option -selectcommand -default ""
    option -itembackground -default "" -configuremethod C-itembackground
    option -sortbackground -default "" -configuremethod C-sortbackground

    variable AREAS -array {}

    # color to use on details view sorted column
    variable sortcolumn "name"
    variable sortorder "-increasing"

    constructor {args} {
	installhull using widget::scrolledwindow

	install tree using treectrl $win.tree \
	    -highlightthickness 0 -borderwidth 0 \
	    -showheader 1 -showroot no -showbuttons no -showlines no \
	    -selectmode browse -xscrollincrement 20 -scrollmargin 16 \
	    -xscrolldelay {500 50} \
	    -yscrolldelay {500 50}

	$hull setwidget $win.tree

	$self tree-details

	# add "TreeCtrlFileList" too if we want to edit
	bindtags $tree [linsert [bindtags $tree] 1 $win]

	$self configurelist $args
    }

    method C-itembackground {option value} {
	$tree column configure all -itembackground $value
	# don't lose sort column
	if {[llength $options(-sortbackground)]} {
	    $tree column configure $sortcolumn \
		-itembackground $options(-sortbackground)
	}
	set options($option) $value
    }

    method C-sortbackground {option value} {
	$tree column configure $sortcolumn -itembackground \
	    [expr {[llength $value] ? $value : $options(-itembackground)}]
	set options($option) $value
    }

    method add {name args} {
	# There should be no duplication of areas
	set item [$tree item create -button 0 -open 0 -parent 0 -visible 1]
	set AREAS($name) $item
	$tree item style set $item \
	    name styMixed \
	    num styText \
	    prefix styText \
	    inc styText
	array set opts $args
	set opts(name) $name
	eval [linsert [array get opts] 0 $tree item text $item]
	# should we schedule a sort, or make the user force it?
	# currently the user must request it.
	return $item
    }

    method data {id {col {}}} {
	if {$col ne ""} {
	    return [$tree item text $id $col]
	} else {
	    set out [list]
	    foreach col [$tree column list] {
		lappend out [$tree column cget $col -tag] \
		    [$tree item text $id $col]
	    }
	    return $out
	}
    }

    method state {name {state {}}} {
	# This should return the current item state
	set item [lindex $AREAS($name) 0]
	if {$state ne ""} {
	    $tree item state forcolumn $item name $state
	}
	return [$tree item state forcolumn $item name]
    }

    method clear {} {
	$tree item delete all
	array unset AREAS
	array set AREAS {}
    }

    method numitems {} {
	return [$tree item numchildren root]
    }

    method view {col {show {}}} {
	if {$show ne ""} {
	    $tree column configure $col -visible $show
	}
	return [$tree column cget $col -visible]
    }

    method sort {} {
	if {[llength $options(-sortbackground)]} {
	    $tree column configure $sortcolumn \
		-itembackground $options(-sortbackground)
	}
	$tree column configure $sortcolumn \
	    -arrow [expr {$sortorder eq "-decreasing" ? "down" : "up"}]
	$tree item sort root $sortorder -column $sortcolumn -dictionary
    }

    method _headerinvoke {t col} {
	if {[$tree column compare $col == $sortcolumn]} {
	    set sortorder [expr {$sortorder eq "-increasing" ?
				 "-decreasing" : "-increasing"}]
	} else {
	    # Don't change sortorder
	    $tree column configure $sortcolumn -arrow none \
		-itembackground $options(-itembackground)
	    set sortcolumn $col
	}
	$self sort
    }

    method tree-details {} {
	set height [font metrics [$tree cget -font] -linespace]
	if {$height < 20} {
	    set height 20
	}
	$tree configure -itemheight $height

	foreach {lbl tag opts} {
	    "Area"         name    {-width 80}
	    "\# Pkgs"      num     {-width 60}
	    "Prefix"       prefix  {-minwidth 100 -expand 1 -squeeze 1}
	    "@INC"         inc     {-minwidth 100 -expand 1 -squeeze 1}
	} {
	    eval [list $tree column create -text $lbl -tag $tag] $opts
	}
	# common configuration options
	$tree column configure all -borderwidth 1 \
	    -itembackground $options(-itembackground)

	set selbg $::style::as::highlightbg
	set selfg $::style::as::highlightfg

	$tree state define default
	$tree state define readonly

	# Create elements
	$tree element create elemDefault image \
	    -image [list [::ppm::img radio-on] {default} \
			[::ppm::img radio-off] {}]
	$tree element create elemReadonly image \
	    -image [list [::ppm::img locked] {readonly} \
			[::ppm::img unlocked] {}]
	$tree element create elemText text -lines 1 \
	    -fill [list $selfg {selected focus}] \
	    -font [list ASfontBold {default} ASfont {}]
	$tree element create selRect rect \
	    -fill [list $selbg {selected focus} gray {selected !focus}]

	# text + image style
	set S [$tree style create styMixed -orient horizontal]
	$tree style elements $S {selRect elemDefault elemReadonly elemText}
	$tree style layout $S selRect -iexpand news \
	    -union {elemDefault elemReadonly elemText}
	$tree style layout $S elemDefault -expand ns -padx 2
	$tree style layout $S elemReadonly -expand ns -padx 2
	$tree style layout $S elemText -squeeze x -expand ns -padx 2

	# text style
	set S [$tree style create styText]
	$tree style elements $S {selRect elemText}
	$tree style layout $S selRect -union {elemText} -iexpand news
	$tree style layout $S elemText -squeeze x -expand ns -padx 2

	# Tree bindings
	#$tree notify install <Header-invoke>
	#$tree notify bind $tree <Header-invoke> [mymethod _headerinvoke %T %C]

	$tree notify bind $tree <Selection> [mymethod _select %S]

	bind $tree <1> [mymethod _check_default %W %x %y]

	$tree notify install <Edit-begin>
	$tree notify install <Edit-end>
	$tree notify install <Edit-accept>

	# List of lists: {column style element ...} specifying elements
	# the user can click on or select with the selection rectangle
	#TreeCtrl::SetSensitive $tree { }

	# List of lists: {column style element ...} specifying elements
	# added to the drag image when dragging selected items
	#TreeCtrl::SetDragImage $tree { }

	# List of lists: {column style element ...} specifying text elements
	# the user can edit
	#TreeCtrl::SetEditable $tree { }

	# During editing, hide the text and selection-rectangle elements.
	$tree notify bind $tree <Edit-begin> {
		%T item element configure %I %C \
		    elemText -draw no + selRect -draw no
	}
	$tree notify bind $tree <Edit-accept> \
	    [mymethod _edit_accept %I %C %E %t]
	$tree notify bind $tree <Edit-end> {
		%T item element configure %I %C \
		    elemText -draw yes + selRect -draw yes
	}
    }

    method _edit_accept {item col elem txt} {
	if {$txt eq ""} { return }
	$self _select $item setname $txt
	$tree item element configure $item $col $elem -text $txt
    }

    method _check_default {w x y} {
	if {$w eq $tree} {
	    set item [$tree identify $x $y]
	    if {[lindex $item end] eq "elemDefault"} {
		$self _select [lindex $item 1] default
		return -code break
	    }
	}
    }

    method _select {new args} {
	if {$options(-selectcommand) ne ""} {
	    uplevel 1 $options(-selectcommand) $new $args
	}
    }
}
