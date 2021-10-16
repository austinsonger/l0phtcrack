# repolist.tcl --
#
#	This file implements package repolist, which  ...
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
package provide ppm::repolist 1.0

snit::widgetadaptor repolist {

    component tree

    delegate option -borderwidth to hull
    delegate option -relief to hull
    delegate option -padding to hull
    delegate option * to tree
    delegate method * to tree

    option -selectcommand -default ""
    option -itembackground -default "" -configuremethod C-itembackground
    option -sortbackground -default "" -configuremethod C-sortbackground

    variable REPOIDS -array {}

    # color to use on details view sorted column
    variable sortcolumn "repo"
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

	bindtags $tree [linsert [bindtags $tree] 1 $win TreeCtrlFileList]

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

    method add {id args} {
	# There should be no duplication of repos
	set item [$tree item create -button 0 -open 0 -parent 0 -visible 1]
	set REPOIDS($id) $item
	set REPOIDS($id,enabled) 1
	$tree item style set $item \
	    id styText \
	    repo styMixed \
	    url styText \
	    num styText \
	    checked styDate
	array set opts $args
	set opts(id) $id
	# Use classic buttons for better "inlined" look
	set db [button $tree.repo_remove$id \
		    -image [::ppm::img delete] \
		    -padx 0 -pady 0 \
		    -background white -borderwidth 1 -highlightthickness 0 \
		    -command [mymethod _select $item "remove"] \
		   ]
	tooltip::tooltip $db "Delete Repository"
	$tree item element configure $item repo elemRemove -window $db
	set cb [checkbutton $tree.repo_enable$id \
		    -selectcolor white -background white \
		    -padx 0 -pady 0 -offrelief flat \
		    -indicatoron 0 -borderwidth 1 -highlightthickness 0 \
		    -variable [myvar REPOIDS($id,enabled)] \
		    -image [::ppm::img available] \
		    -selectimage [::ppm::img package] \
		    -command [mymethod _select $item "enable"] \
		   ]
	tooltip::tooltip $cb "Enable/Disable Repository"
	$tree item element configure $item repo elemEnable -window $cb
	if {[info exists opts(checked)]} {
	    $tree item element configure \
		$item checked elemDate -data $opts(checked)
	    unset opts(checked)
	}
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

    method enable {repoid {bool {}}} {
	if {$bool ne ""} {
	    set REPOIDS($repoid,enabled) [string is true -strict $bool]
	}
	return $REPOIDS($repoid,enabled)
    }

    method clear {} {
	$tree item delete all
	array unset REPOIDS
	array set REPOIDS {}
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
	$tree item sort root $sortorder -column $sortcolumn -dictionary
    }

    method _headerinvoke {t col} {
	set sortorder -increasing
	set arrow up
	set dir [$tree column cget $sortcolumn -arrow]
	if {[$tree column compare $col == $sortcolumn]} {
	    if {$dir ne "down"} {
		set sortorder -decreasing
		set arrow down
	    }
	} else {
	    if {$dir eq "down"} {
		set sortorder -decreasing
		set arrow down
	    }
	    $tree column configure $sortcolumn -arrow none \
		-itembackground $options(-itembackground)
	    set sortcolumn $col
	}
	$tree column configure $col -arrow $arrow
	if {[llength $options(-sortbackground)]} {
	    $tree column configure $col \
		-itembackground $options(-sortbackground)
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
	    ""             id      {-width 0}
	    "Repository"   repo    {-minwidth 120 -expand 1 -squeeze 1}
	    "URL"          url     {-minwidth 100 -expand 1 -squeeze 1}
	    "\# Pkgs"      num     {-width 60}
	    "Last Checked" checked {-width 120}
	} {
	    eval [list $tree column create -text $lbl -tag $tag \
		      -borderwidth 1] $opts
	}

	set selbg $::style::as::highlightbg
	set selfg $::style::as::highlightfg

	# Create elements
	$tree element create elemRemove window -destroy 1
	$tree element create elemEnable window -destroy 1
	$tree element create elemText text -lines 1 \
	    -fill [list $selfg {selected focus}]
	$tree element create elemDate text -lines 1 \
	    -fill [list $selfg {selected focus}] \
	    -datatype time -format "%x %X"
	$tree element create selRect rect \
	    -fill [list $selbg {selected focus} gray {selected !focus}]

	# text + image style
	set S [$tree style create styMixed -orient horizontal]
	$tree style elements $S {selRect elemRemove elemEnable elemText}
	$tree style layout $S selRect -iexpand news \
	    -union {elemRemove elemEnable elemText}
	$tree style layout $S elemRemove -expand ns -padx 2
	$tree style layout $S elemEnable -expand ns -padx 2
	$tree style layout $S elemText -squeeze x -expand ns -padx 2

	# text style
	set S [$tree style create styText]
	$tree style elements $S {selRect elemText}
	$tree style layout $S selRect -union {elemText} -iexpand news
	$tree style layout $S elemText -squeeze x -expand ns -padx 2

	# date style
	set S [$tree style create styDate]
	$tree style elements $S {selRect elemDate}
	$tree style layout $S selRect -union {elemDate} -iexpand news
	$tree style layout $S elemDate -squeeze x -expand ns -padx 2

	$tree notify install <Header-invoke>
	$tree notify bind $tree <Header-invoke> [mymethod _headerinvoke %T %C]

	$tree notify bind $tree <Selection> [mymethod _select %S]

	#$tree column dragconfigure -enable 1
	$tree notify install <ColumnDrag-begin>
	$tree notify install <ColumnDrag-end>
	$tree notify install <ColumnDrag-receive>
	$tree notify bind DontDelete <ColumnDrag-receive> {
	    %T column move %C %b
	}


	$tree notify install <Edit-begin>
	$tree notify install <Edit-end>
	$tree notify install <Edit-accept>

	# List of lists: {column style element ...} specifying elements
	# the user can click on or select with the selection rectangle
	TreeCtrl::SetSensitive $tree {
	    {repo styMixed selRect elemText}
	    {url styText selRect elemText}
	    {num styText selRect elemText}
	    {checked styDate selRect elemDate}
	}

	if {0} {
	    $tree notify install <Drag>
	    $tree notify install <Drag-begin>
	    $tree notify install <Drag-end>
	    $tree notify install <Drag-receive>

	    # List of lists: {column style element ...} specifying elements
	    # added to the drag image when dragging selected items
	    TreeCtrl::SetDragImage $tree {
		{repo styMixed selRect elemText}
	    }
	}
	TreeCtrl::SetDragImage $tree {}

	# List of lists: {column style element ...} specifying text elements
	# the user can edit
	TreeCtrl::SetEditable $tree {
	    {repo styMixed elemText}
	    {url styText elemText}
	}

	# Hack to use FileList bindings without being a file list
	set ::TreeCtrl::Priv(DirCnt,$tree) 0

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

	$tree column configure $sortcolumn -arrow up
	if {[llength $options(-sortbackground)]} {
	    $tree column configure $sortcolumn \
		-itembackground $options(-sortbackground)
	}
    }

    method _edit_accept {item col elem txt} {
	set curval [$tree item element cget $item $col $elem -text]
	if {$txt eq "" || $txt eq $curval} { return }
	if {$col eq "url"} {
	    # we don't need to adjust the value, because if this succeeds,
	    # we should end up doing a full refresh
	    $self _select $item seturl $txt
	} else {
	    # setting the name should always work
	    $self _select $item setname $txt
	    $tree item element configure $item $col $elem -text $txt
	}
    }

    method _select {new args} {
	if {$options(-selectcommand) ne ""} {
	    return [uplevel 1 $options(-selectcommand) $new $args]
	}
    }
}
