# PPM specific packages
package ifneeded ppm::pkglist 1.0 [list source $dir/ppm_pkglist.tcl]
package ifneeded ppm::repolist 1.0 [list source $dir/ppm_repolist.tcl]
package ifneeded ppm::arealist 1.0 [list source $dir/ppm_arealist.tcl]
package ifneeded ppm::themes 1.0 [list source $dir/ppm_themes.tcl]

# For debugging
package ifneeded tkcon 2.4 [subst {
    namespace eval ::tkcon {}
    set ::tkcon::PRIV(showOnStartup) 0
    set ::tkcon::PRIV(protocol) {tkcon hide}
    set ::tkcon::PRIV(root) ".tkcon"
    set ::tkcon::OPT(exec) ""
    package require Tk
    tclPkgSetup [list $dir] tkcon 2.4 {
	{tkcon.tcl source {tkcon dump idebug observe}}
    }
}]
