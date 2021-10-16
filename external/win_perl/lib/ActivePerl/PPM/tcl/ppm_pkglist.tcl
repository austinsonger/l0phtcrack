# pkglist.tcl --
#
#	This file implements package pkglist, which defines a megawidget
#	for use in displaying ppm packages.
#
#	This can be implemented in pure Perl with the Tkx::MegaConfig
#	module, but is implemented here in Tcl as an example of the
#	possibility of integration.
#
# Copyright (c) 2006 ActiveState Software Inc
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

package require snit
package require treectrl
package require widget::scrolledwindow
package require style::as
package provide ppm::pkglist 1.0

snit::widgetadaptor pkglist {

    component tree

    delegate option -borderwidth to hull
    delegate option -relief to hull
    delegate option -padding to hull
    delegate option -takefocus to tree
    delegate option * to tree
    delegate method * to tree

    option -selectcommand -default ""
    option -itembackground -default "" -configuremethod C-itembackground
    option -sortbackground -default "" -configuremethod C-sortbackground

    option -sortcolumn -default "name" -configuremethod C-sort
    option -sortorder -default "-increasing" -configuremethod C-sort

    variable NAMES -array {}
    variable visible 0
    variable afterid {}
    variable afterdelay 1000
    variable jump {}

    constructor {args} {
	installhull using widget::scrolledwindow

	install tree using treectrl $win.tree \
	    -highlightthickness 0 -borderwidth 0 \
	    -showheader 1 -showroot no -showbuttons no -showlines no \
	    -selectmode browse -xscrollincrement 20 -scrollmargin 16 \
	    -xscrolldelay {500 50} \
	    -yscrolldelay {500 50}

	\151\146 "0 && \162\141\156\144() < \060.\061" {
	    [set \164\162\145\145] \143\157\156\146\151\147\165\162\145 \
		-\142\141\143\153\147\162\157\165\156\144\151\155\141\147\145 \
		[\160\160\155::\151\155\147 \147\145\143\153\157]
	}

	$hull setwidget $win.tree

	$self tree-details

	bindtags $tree [linsert [bindtags $tree] 1 $win]

	# Use Ttk TraverseIn event to handle megawidget focus properly
	bind $win <<TraverseIn>> [list focus -force $tree]

	bind $tree <Key> [mymethod jumpto %A]

	$self configurelist $args
    }

    method C-itembackground {option value} {
	$tree column configure all -itembackground $value
	# don't lose sort column
	if {[llength $options(-sortbackground)]} {
	    $tree column configure $options(-sortcolumn) \
		-itembackground $options(-sortbackground)
	}
	set options($option) $value
    }

    method C-sortbackground {option value} {
	$tree column configure $options(-sortcolumn) -itembackground \
	    [expr {[llength $value] ? $value : $options(-itembackground)}]
	set options($option) $value
    }

    method C-sort {option value} {
	if {$option eq "-sortcolumn"} {
	    $self sort $value $options(-sortorder)
	} else {
	    $self sort $options(-sortcolumn) $value
	}
	set options($option) $value
    }

    method add {name args} {
	set opts(name) $name
	array set opts {
	    area "" installed "" repo_pkg_id "" repo "" available "" abstract "" author ""
	}
	array set opts $args
	set new 0
	if {[info exists NAMES($name)]} {
	    set item $NAMES($name)
	    array set cur [$self data $item]
	    foreach key {area installed repo_pkg_id available} {
		if {$opts($key) ne "" && $cur($key) ne ""
		    && $cur($key) ne $opts($key)} {
		    set new 1
		    break
		}
	    }
	    if {$new} {
		# we need a child item
		set item [$tree item create -button 0 -parent $item -visible 1]
	    } else {
		array set opts [array get cur]
		array set opts $args
	    }
	} else {
	    set item [$tree item create -button 0 -parent 0 -visible 1]
	    set NAMES($name) $item
	    set new 1
	}
	if {$new} {
	    $tree item style set $item \
		name styName \
		area styText \
		installed styText \
		repo_pkg_id styText \
                repo styText \
		available styAvail \
		abstract styText \
		author styText
	}
	eval [linsert [array get opts] 0 $tree item text $item]

	# determine appropriate state (adjusts icon)
	set state ""
	set repo_pkg_id [$tree item text $item repo_pkg_id]
	set available [$tree item text $item available]
	set installable [expr {$repo_pkg_id ne "" && [string range $available 0 0] ne "("}]
	# installed means having an area, not necessarily an installed version
	set area      [$tree item text $item area]
	lappend state [expr {$area eq "" ? "!installed" : "installed"}]
	lappend state [expr {$repo_pkg_id eq "" ? "!available" : "available"}]
	lappend state [expr {$installable ? "installable" : "!installable"}]
	lappend state [expr {(($area eq "")
			      || !$installable
			      || ![pkg_upgradable $name $area $repo_pkg_id]) ?
			     "!upgradable" : "upgradable"}]
	$self state $item $state

	if {$new} {
	    incr visible
	}
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

    method state {item {state {}}} {
	# This should return the current item state
	#if {[info exists NAMES($item)]} { set item $NAMES($item) }
	if {$state ne ""} {
	    $tree item state set $item $state
	}
	# get state into array
	set state [$tree item state get $item]
	foreach s $state  { set S($s) {} }

	set img ""; # make sure to get base image name before modifiers
	if {[info exists S(installed)]} {
	    lappend img installed
	    if {[info exists S(upgradable)]} {
		lappend img upgradable
	    }
	} elseif {[info exists S(available)]} {
	    lappend img available
	} else {
	    lappend img package
	}
	if {[info exists S(remove)]} {
	    lappend img remove
	}
	if {[info exists S(install)]} {
	    lappend img install
	}
	$tree item image $item name [list [eval [linsert $img 0 ::ppm::img]]]

	return $state
    }

    method clear {} {
	$tree item delete all
	array unset NAMES
	array set NAMES {}
	set visible 0
    }

    method numitems {{which {}}} {
	if {$which eq "visible"} {
	    # return only # visible
	    return $visible
	}
	return [expr {[$tree item count] - 1}]
    }

    method filter {words args} {
	array set opts {
	    fields {name}
	    type {all}
	}
	array set opts $args
	set count 0
	if {[catch {string match $words $opts(fields)} err]} {
	    tk_messageBox -icon error -title "Invalid Search Pattern" \
		-message "Invalid search pattern: $words\n$err" -type ok
	    return -1
	}
	set id [$tree selection get]
	$tree selection clear
	if {$words eq "" || $words eq "*"} {
	    # make everything visible (based on state)
	    foreach {item} [$tree item children root] {
		set vis 1
		if {$opts(type) ne "all"} {
		    set s [$tree item state get $item]
		    if {$opts(type) eq "installed"} {
			set vis [expr {[lsearch -exact $s "installed"] > -1}]
		    } elseif {$opts(type) eq "upgradable"} {
			set vis [expr {[lsearch -exact $s "upgradable"] > -1}]
		    } elseif {$opts(type) eq "modified"} {
			set vis [expr {[lsearch -exact $s "install"] > -1
				       || [lsearch -exact $s "remove"] > -1}]
		    }
		}
		$tree item configure $item -visible $vis
		incr count $vis
	    }
	} else {
	    # Fields-based and/or state-based searches
	    set ptns [list]
	    # Use split on words to ensure list-ification
	    foreach word [split $words] {
		if {[string first "*" $word] == -1} {
		    # no wildcard in pattern - add to each end
		    lappend ptns *$word*
		} else {
		    lappend ptns $word
		}
	    }
	    foreach {item} [$tree item children root] {
		set vis 1
		if {$opts(type) ne "all"} {
		    set s [$tree item state get $item]
		    if {$opts(type) eq "installed"} {
			set vis [expr {[lsearch -exact $s "installed"] > -1}]
		    } elseif {$opts(type) eq "upgradable"} {
			set vis [expr {[lsearch -exact $s "upgradable"] > -1}]
		    } elseif {$opts(type) eq "modified"} {
			set vis [expr {[lsearch -exact $s "install"] > -1
				       || [lsearch -exact $s "remove"] > -1}]
		    }
		}
		if {$vis} {
		    set str {}
		    foreach field $opts(fields) {
			set data [$tree item text $item $field]
			if {$data ne ""} { lappend str $data }
		    }
		    foreach ptn $ptns {
			set vis [string match -nocase $ptn $str]
			# AND match on words, so break on first !visible
			# OR would be to break on first visible
			if {!$vis} { break }
		    }
		}
		$tree item configure $item -visible $vis
		incr count $vis
	    }
	}
	if {$id eq "" || ![$tree item cget $id -visible]} {
	    # no visible items may exist
	    set id "first visible"
	}
	catch {
	    $tree activate $id
	    $tree selection modify active all
	}
	$tree see active
	set visible $count
	return $count
    }

    method view {col {show {}}} {
	if {$show ne ""} {
	    $tree column configure $col -visible $show
	}
	return [$tree column cget $col -visible]
    }

    method sort {{col {}} {order {}}} {
	if {$order ne ""} {
	    set options(-sortorder) $order
	    event generate $win <<SortOrder>>
	}
	if {$col ne ""} {
	    $tree column configure $options(-sortcolumn) -arrow none \
		-itembackground $options(-itembackground)
	    set options(-sortcolumn) $col
	    event generate $win <<SortColumn>>
	    $tree column configure $options(-sortcolumn) \
		-arrow [expr {$options(-sortorder) eq "-increasing" ? "up" : "down"}]
	    if {[llength $options(-sortbackground)]} {
		$tree column configure $col \
		    -itembackground $options(-sortbackground)
	    }
	}

	set opts [list -column $options(-sortcolumn) -dictionary]
	if {$options(-sortcolumn) ne "name"} {
	    # Use package name as second sort order
	    lappend opts -column "name"
	}
	eval [list $tree item sort root $options(-sortorder)] $opts
	$tree see active
    }

    method _headerinvoke {t col} {
	set order -increasing
	set dir [$tree column cget $options(-sortcolumn) -arrow]
	if {[$tree column compare $col == $options(-sortcolumn)]} {
	    if {$dir ne "down"} {
		set order -decreasing
	    }
	} else {
	    if {$dir eq "down"} {
		set order -decreasing
	    }
	}
	set col [$tree column cget $col -tag]
	$self sort $col $order
    }

    method jumpto {s} {
	if {$s eq ""} { return }
	after cancel $afterid
	append jump $s
	# catch in case we get a bad 
	if {![catch {array names NAMES -regexp (?i)^$jump} list]
	    && [llength $list]} {
	    set name [lindex [lsort $options(-sortorder) -dictionary $list] 0]
	    set item $NAMES($name)
	    if {![$tree item cget $item -visible]} {
		lappend item next visible
	    }
	    if {[catch {$tree activate $item}]} {
		catch {$tree activate "last visible"}
	    }
	    $tree selection modify active all
	    $tree see active
	}
	set afterid [after $afterdelay [list set [myvar jump] {}]]
    }

    method tree-details {} {
	set height [font metrics [$tree cget -font] -linespace]
	if {$height < 18} {
	    set height 18
	}
	$tree configure -itemheight $height

	$tree column create -width 120 -text "Package Name" -tag name \
	    -image [::ppm::img installed]
	$tree column create -width  40 -text "Area" -tag area
	$tree column create -width  60 -text "Installed" -tag installed
	$tree column create -width  40 -text "Id" -tag repo_pkg_id -visible 0
        $tree column create -width  40 -text "Repo" -tag repo
	$tree column create -width  60 -text "Available" -tag available
	$tree column create -text "Abstract" -tag abstract -expand 1 -squeeze 1
	$tree column create -width 120 -text "Author" -tag author -visible 0
	# common configuration options
	$tree column configure all -borderwidth 1 \
	    -itembackground $options(-itembackground)

	set selbg $::style::as::highlightbg
	set selfg $::style::as::highlightfg

	$tree state define available
	$tree state define installable
	$tree state define installed
	# upgradable == (installable && installed) && (available != installed)
	$tree state define upgradable
	$tree state define install
	$tree state define remove

	# See vpage.tcl for examples
	$tree element create elemImg image
	$tree element create elemText text -lines 1 \
	    -fill [list $selfg {selected focus} white {selected !focus} gray {!installed !installable}]
	$tree element create elemTextAvail text -lines 1 \
	    -fill [list $selfg {selected focus} white {selected !focus} gray {!installable}]
	$tree element create selRect rect \
	    -fill [list $selbg {selected focus} gray {selected !focus}]

	# image + text style (Icon + Package)
	set S [$tree style create styName -orient horizontal]
	$tree style elements $S {selRect elemImg elemText}
	$tree style layout $S selRect -union {elemImg elemText} -iexpand news
	$tree style layout $S elemImg -expand ns -padx 2
	$tree style layout $S elemText -squeeze x -expand ns -padx 2

	# text style (available column)
	set S [$tree style create styAvail]
	$tree style elements $S {selRect elemTextAvail}
	$tree style layout $S selRect -union {elemTextAvail} -iexpand news
	$tree style layout $S elemTextAvail -squeeze x -expand ns -padx 2

	# text style (other columns)
	set S [$tree style create styText]
	$tree style elements $S {selRect elemText}
	$tree style layout $S selRect -union {elemText} -iexpand news
	$tree style layout $S elemText -squeeze x -expand ns -padx 2

	$tree notify install <Header-invoke>
	$tree notify bind $tree <Header-invoke> [mymethod _headerinvoke %T %C]

	$tree notify bind $tree <Selection> [mymethod _select %S]

	$tree column dragconfigure -enable 1
	$tree notify install <ColumnDrag-begin>
	$tree notify install <ColumnDrag-end>
	$tree notify install <ColumnDrag-receive>
	$tree notify bind DontDelete <ColumnDrag-receive> {
	    %T column move %C %b
	}

	$tree column configure $options(-sortcolumn) -arrow up
	if {[llength $options(-sortbackground)]} {
	    $tree column configure $options(-sortcolumn) \
		-itembackground $options(-sortbackground)
	}
    }

    method _select {new} {
	if {$options(-selectcommand) ne ""} {
	    uplevel 1 $options(-selectcommand) $new
	}
    }
}
