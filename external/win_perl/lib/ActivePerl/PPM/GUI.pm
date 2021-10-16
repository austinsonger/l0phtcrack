package ActivePerl::PPM::GUI;
use utf8;

BEGIN {
    # Don't allow these env vars to disrupt ppm Tkx usage unless we are
    # ourselves in debug mode.
    unless ($ENV{ACTIVEPERL_PPM_DEBUG}) {
        delete $ENV{$_} for qw(PERL_TCL_DLL PERL_TCL_DL_PATH);
    }
    return unless $^O eq "MSWin32";
    # Make sure we run in a STA in case the GUI pulls in shell extension code (bug 106246)
    require Win32::OLE;
    Win32::OLE->Initialize(Win32::OLE::COINIT_OLEINITIALIZE);
}

use strict;
use Tkx ();
use ActiveState::Browser ();
use ActivePerl::PPM::Logger qw(ppm_log ppm_status);
use ActivePerl::PPM::Util qw(is_cpan_package clean_err join_with update_html_toc);

# get our cwd for Tcl files
use File::Basename qw(dirname);
use File::Spec::Functions qw(devnull);
use Cwd qw(cwd abs_path);

my @pending_status_message;
my $ppm = $::ppm;
$ActiveState::Browser::HTML_DIR = $ppm->area("perl")->html;

status_message("$::bad_proxy\n\n", "abstract") if $::bad_proxy;

# these will be filled in the sync()
my %REPOS;
my $INSTALL_AREA;

my $mw = Tkx::widget->new(".");
$mw->g_wm_withdraw();
Tkx::tk(appname => "Perl Package Manager");

Tkx::lappend('::auto_path', abs_path(dirname(__FILE__)) . "/tcl");

my $windowingsystem = Tkx::tk('windowingsystem');
my $AQUA = ($windowingsystem eq "aqua");
my $plat_acc_ctrl = ($AQUA ? "Command-" : "Ctrl+");
my $plat_evt_ctrl = ($AQUA ? "Command-" : "Control-");

if ($ENV{'ACTIVEPERL_PPM_DEBUG'}) {
    Tkx::package_require('comm');
    print "DEBUG COMM PORT: " . Tkx::comm__comm('self') . "\n";

    Tkx::package_require('tkcon');
    Tkx::bind(all => "<${plat_evt_ctrl}F12>", 'catch {tkcon show}');
    Tkx::bind(all => "<${plat_evt_ctrl}F11>", 'catch {tkcon hide}');
    Tkx::catch("tkcon hide");
}

Tkx::package_require('tile');
Tkx::package_require('img::png');
Tkx::package_require('ppm::themes');
Tkx::package_require('tooltip');
Tkx::package_require('widget::dialog');
Tkx::package_require('widget::statusbar');
Tkx::package_require('widget::toolbar');
Tkx::package_require('widget::menuentry');
Tkx::package_require('widget::panelframe');
Tkx::package_require('ppm::pkglist');
Tkx::package_require('ppm::repolist');
Tkx::package_require('ppm::arealist');
Tkx::package_require('style::as');
Tkx::package_require('BWidget');
Tkx::Widget__theme(1);

Tkx::style__as__init();
# This enables font-size control via <Control-MouseWheel> for
# pkglist and text widgets
Tkx::style__as__enable("control-mousewheel");

if ($AQUA) {
    Tkx::set("::tk::mac::useThemedToplevel" => 1);
    # The console can pop up unexpectedly from the tkkit
    Tkx::catch("console hide");
    eval {
	Tkx::package_require('tclCarbonProcesses');
	my $psn = Tkx::carbon__getCurrentProcess();
	Tkx::carbon__setProcessName("$psn", 'PPM');
    };
}

if ($windowingsystem eq "win32") {
    # Due to a bug in Tk when wrapped as tkkit, we need to call the
    # iconbitmap setting on the main window an extra time.
    my $icon = Tkx::wm_iconbitmap($mw);
    Tkx::wm_iconbitmap($mw, $icon);
    Tkx::wm_iconbitmap($mw, -default => $^X);
}
elsif ($windowingsystem eq "x11") {
    Tkx::wm_iconphoto($mw, "-default", Tkx::ppm__img('perl'));
}

# This code makes themed frames use the notebook's background color.
# We restrict the use of this to those frames in notebooks.
Tkx::ttk__style(layout => "NotebookPane",
		["NotebookPane.background", -sticky => "news", -expand => 1]);
Tkx::option_add("*TNotebook.TFrame.style", "NotebookPane");

Tkx::option_add("*TEntry.cursor", "xterm");

# Make invalid state entry/label widget change color scheme
Tkx::ttk__style(map => "TEntry", -foreground => [invalid => "red"],
		-fieldbackground => [invalid => "yellow"]);
Tkx::ttk__style(map => "TLabel", -foreground => [invalid => "red"]);

# get 'tooltip' as toplevel command
Tkx::namespace_import("::tooltip::tooltip");

# make tree widgets use theming on non-x11 platforms
if ($windowingsystem ne "x11") {
    Tkx::option_add("*TreeCtrl.useTheme", 1);
}

# Since our treectrl's don't scroll horizontal, make sure Home/End work
# for the vertical direction as well
Tkx::bind(TreeCtrl => "<Key-Home>",
	  Tkx::bind(TreeCtrl => "<Control-Key-Home>"));
Tkx::bind(TreeCtrl => "<Shift-Key-Home>",
	  Tkx::bind(TreeCtrl => "<Control-Shift-Key-Home>"));
Tkx::bind(TreeCtrl => "<Key-End>",
	  Tkx::bind(TreeCtrl => "<Control-Key-End>"));
Tkx::bind(TreeCtrl => "<Shift-Key-End>",
	  Tkx::bind(TreeCtrl => "<Control-Shift-Key-End>"));

# purely for reciprocity debugging, expose the ppm command in Tcl
Tkx::interp_alias("", "ppm", "", [\&ppm]);

# Use Tk scroll on OS X, but Ttk scrollbar elsewhere by default
if ($AQUA) {
    Tkx::interp_alias("", "::ttk::scrollbar", "", "::scrollbar");
    Tkx::option('add', "*Scrollbar.borderWidth", 0);
}
else {
    Tkx::interp_alias("", "::scrollbar", "", "::ttk::scrollbar");
}

# These variables are tied to UI elements
my %FILTER;
$FILTER{'id'} = "";
$FILTER{'delay'} = 1000; # filter delay on key in millisecs
$FILTER{'filter'} = "";
$FILTER{'lastfilter'} = "";
$FILTER{'fields'} = "name abstract";
$FILTER{'lastfields'} = $FILTER{'fields'};
$FILTER{'type'} = "installed"; # all installed upgradable modified
$FILTER{'lasttype'} = $FILTER{'type'};

my %VIEW;
$VIEW{'name'} = 1;
$VIEW{'area'} = 1;
$VIEW{'installed'} = 1;
$VIEW{'repo'} = 0;
$VIEW{'available'} = 1;
$VIEW{'abstract'} = 1;
$VIEW{'author'} = 0;

$VIEW{'toolbar'} = 1;
$VIEW{'statusbar'} = 1;

$VIEW{'sortcolumn'} = 'name';
$VIEW{'sortorder'} = '-increasing';

my %ACTION;
my $dummy = 0; # used as a dummy tied variable

my %IMG;
$IMG{'refresh'} = [Tkx::ppm__img('refresh')];
$IMG{'prefs'} = [Tkx::ppm__img('config')];
$IMG{'install'} = [Tkx::ppm__img('package', 'install')];
$IMG{'remove'} = [Tkx::ppm__img('package', 'remove')];
$IMG{'go'} = [Tkx::ppm__img('go')];
$IMG{'f_all'} = [Tkx::ppm__img('available', 'filter')];
$IMG{'f_upgradable'} = [Tkx::ppm__img('package', 'filter', 'upgradable')];
$IMG{'f_installed'} = [Tkx::ppm__img('package', 'filter')];
$IMG{'f_modified'} = [Tkx::ppm__img('package', 'filter', 'modified')];

my $action_menu;
my $fields_menu;
my $sort_menu;
my $view_menu;
my $file_menu;

# Create the menu structure
menus();

Tkx::bind($mw, "<Destroy>", [
    sub {
	my $w = shift;
	on_exit() if $w eq $mw->_mpath;
    },
    Tkx::Ev('%W')
]);
$mw->g_wm_protocol('WM_DELETE_WINDOW', [\&on_exit]);

Tkx::option_add("*takeFocus", "0");
Tkx::option_add("*TEntry.takeFocus", "1");

# Main interface
my $pw = $mw->new_ttk__panedwindow(-orient => "vertical");
my $pkglist = $pw->new_pkglist(
    -width => 550, -height => 350,
    -selectcommand => [\&select_item],
    -borderwidth => 1,
    -relief => 'sunken',
    -itembackground => ["#F7F7FF", ""],
    -takefocus => 1,
);

Tkx::bind($pkglist, "<<PackageMenu>>", [
    sub {
	my ($x, $y, $X, $Y) = @_;
	if ($pkglist->identify($x, $y) =~ "header") {
	    $fields_menu->g_tk___popup($X, $Y);
	}
	else {
	    $pkglist->selection('clear');
	    eval {
		# This will error silently if the list is empty
		$pkglist->selection('add', "nearest $x $y");
		$action_menu->g_tk___popup($X, $Y);
	    };
	}
    },
    Tkx::Ev("%x", "%y", "%X", "%Y"),
]);
# Aqua swaps buttons 2 and 3 for historical reasons
Tkx::event('add', "<<PackageMenu>>", ($AQUA ? "<Button-2>" : "<Button-3>"),
	   "<Control-Button-1>");
# Catch changes in sort behavior by widget for menus
Tkx::bind($pkglist, "<<SortOrder>>",
	  sub { $VIEW{'sortorder'} = $pkglist->cget('-sortorder'); });
Tkx::bind($pkglist, "<<SortColumn>>",
	  sub { $VIEW{'sortcolumn'} = $pkglist->cget('-sortcolumn'); });

# Details / Status areas
my @smallfont = (-font => "ASfont");
my @smallfontbold = (-font => "ASfontBold");
if ($AQUA) {
    @smallfont = (-font => "ASfont-1");
    @smallfontbold = (-font => "ASfontBold-1");
}
my @text_opts = (
    -height => 7, -width => 40,
    -cursor => "",
    -borderwidth => 3,
    -highlightthickness => 0,
    -relief => "flat",
    -font => "ASfont",
    -state => "disabled",
    -wrap => "word",
);
my $pw_nb = $pw->new_ttk__notebook(-padding => 0);

my $status_sw = $pw_nb->new_widget__scrolledwindow();
my $status_box = $status_sw->new_text(@text_opts);
$status_sw->setwidget($status_box);

my $details_sw = $pw_nb->new_widget__scrolledwindow();
my $details = $details_sw->new_text(
    @text_opts,
    -tabs => ["10", "left", "90", "left"],
);
$details_sw->setwidget($details);

for my $tw ($details, $status_box) {
    # Allow each text widget the same tag set
    $tw->tag_configure('h1', -font => 'ASfontBold2');
    $tw->tag_configure('h2', -font => 'ASfontBold1');
    $tw->tag_configure('abstract',
        -font => 'ASfontBold',
	-lmargin1 => 10, -lmargin2 => 10,
        -rmargin => 10,
    );
    $tw->tag_configure('link', -underline => 1, -foreground => 'blue');
    $tw->tag_bind('link', "<Enter>", sub {
        $tw->configure(-cursor => "hand2");
    });
    $tw->tag_bind('link', "<Leave>", sub {
	$tw->configure(-cursor => "");
    });
}

$pw_nb->add($status_sw, -text => "Status");
$pw_nb->add($details_sw, -text => "Details");
$pw_nb->select($status_sw);
status_message(@$_) for @pending_status_message;

$pw->add($pkglist, -weight => 3);
$pw->add($pw_nb, -weight => 1);

my $scroll_cmd = sub {
    my $dir = shift;
    my $tw = ($pw_nb->select() eq $status_sw ? $status_box : $details);
    $tw->yview("scroll", $dir, "page");
};
Tkx::bind($mw, "<Key-space>", [$scroll_cmd, 1]);
Tkx::bind($mw, "<Shift-Key-space>", [$scroll_cmd, -1]);

my $toolbar = $mw->new_widget__toolbar();
my $statusbar = $mw->new_widget__statusbar(-ipad => [1, 2]);

Tkx::grid($toolbar, -sticky => "ew", -padx => 2);
Tkx::grid($pw, -sticky => "news", -padx => 0, -pady => 0);
Tkx::grid($statusbar, -sticky => "ew");

Tkx::grid(rowconfigure => $mw, 1, -weight => 1);
Tkx::grid(columnconfigure => $mw, 0, -weight => 1);

my $prefs_dialog;
my $repolist;
my $arealist;

## Toolbar items

# Filter state buttons
my $filter_all = $toolbar->new_ttk__radiobutton(
    -text => "All",
    -image => $IMG{'f_all'},
    -style => "Toolbutton",
    -variable => \$FILTER{'type'},
    -command => [\&filter],
    -value => "all",
);
$toolbar->add($filter_all, -pad => [0, 2]);
Tkx::tooltip($filter_all, "View all packages [${plat_acc_ctrl}1]");
my $filter_inst = $toolbar->new_ttk__radiobutton(
    -text => "Installed",
    -image => $IMG{'f_installed'},
    -style => "Toolbutton",
    -variable => \$FILTER{'type'},
    -command => [\&filter],
    -value => "installed",
);
$toolbar->add($filter_inst, -pad => [0, 2]);
Tkx::tooltip($filter_inst, "View installed packages [${plat_acc_ctrl}2]");
my $filter_upgr = $toolbar->new_ttk__radiobutton(
    -text => "Upgradable",
    -image => $IMG{'f_upgradable'},
    -style => "Toolbutton",
    -variable => \$FILTER{'type'},
    -command => [\&filter],
    -value => "upgradable",
);
$toolbar->add($filter_upgr, -pad => [0, 2]);
Tkx::tooltip($filter_upgr, "View upgradable packages [${plat_acc_ctrl}3]");
my $filter_mod = $toolbar->new_ttk__radiobutton(
    -text => "Modified",
    -image => $IMG{'f_modified'},
    -style => "Toolbutton",
    -variable => \$FILTER{'type'},
    -command => [\&filter],
    -value => "modified",
);
$toolbar->add($filter_mod, -pad => [0, 2]);
Tkx::tooltip($filter_mod, "View packages to install/remove [${plat_acc_ctrl}4]");

Tkx::bind(all => "<${plat_evt_ctrl}Key-1>" => sub { $filter_all->invoke(); });
Tkx::bind(all => "<${plat_evt_ctrl}Key-2>" => sub { $filter_inst->invoke(); });
Tkx::bind(all => "<${plat_evt_ctrl}Key-3>" => sub { $filter_upgr->invoke(); });
Tkx::bind(all => "<${plat_evt_ctrl}Key-4>" => sub { $filter_mod->invoke(); });

# Filter entry with filter.fields menu
my $filter_menu = $toolbar->new_menu(-name => "filter_menu");
my $filter = $toolbar->new_widget__menuentry(
    -width => 1,
    -takefocus => 1,
    -menu => $filter_menu,
    -textvariable => \$FILTER{'filter'},
);
Tkx::tooltip($filter, "Filter packages [${plat_acc_ctrl}F]");
$toolbar->add($filter, -weight => 2);
$filter_menu->add('radiobutton',
    -label => "Name",
    -value => "name",
    -variable => \$FILTER{'fields'},
    -command => [\&filter],
);
$filter_menu->add('radiobutton',
    -label => "Abstract",
    -value => "abstract",
    -variable => \$FILTER{'fields'},
    -command => [\&filter],
);
$filter_menu->add('radiobutton',
    -label => "Name or Abstract",
    -value => "name abstract",
    -variable => \$FILTER{'fields'},
    -command => [\&filter],
);
$filter_menu->add('radiobutton',
    -label => "Author",
    -value => "author",
    -variable => \$FILTER{'fields'},
    -command => [\&filter],
);
$filter->g_bind('<Return>', [\&filter]);
$filter->g_bind('<Key>', [\&filter_onkey]);
Tkx::bind(all => "<${plat_evt_ctrl}f>" => sub { Tkx::focus($filter); });

# Action buttons
my $install_btn = $toolbar->new_ttk__checkbutton(
    -text => "Install",
    -variable => \$dummy,
    -image => $IMG{'install'},
    -style => "Toolbutton",
    -state => "disabled",
);
$toolbar->add($install_btn, -separator => 1, -pad => [4, 2, 0]);
Tkx::tooltip($install_btn, "Mark for install [+]");
my $remove_btn = $toolbar->new_ttk__checkbutton(
    -text => "Remove",
    -variable => \$dummy,
    -image => $IMG{'remove'},
    -style => "Toolbutton",
    -state => "disabled",
);
$toolbar->add($remove_btn, -pad => [0, 2]);
Tkx::tooltip($remove_btn, "Mark for remove [-]");
my $go_btn = $toolbar->new_ttk__button(
    -text => "Go",
    -image => $IMG{'go'},
    -style => "Toolbutton",
    -state => "disabled",
    -command => [\&run_actions],
);
$toolbar->add($go_btn, -pad => [4, 2, 0]);
Tkx::tooltip($go_btn, "Run marked actions [${plat_acc_ctrl}Enter]");

# Add [+] and [-] key bindings for install/remove to pkglist
Tkx::bind($pkglist, "<Key-plus>", sub { $install_btn->invoke(); });
Tkx::bind($pkglist, "<Key-minus>", sub { $remove_btn->invoke(); });

# Sync/config buttons
my $sync_btn = $toolbar->new_ttk__button(
    -text => "Refresh",
    -image => $IMG{'refresh'},
    -style => "Toolbutton",
    -command => \&full_refresh,
);
Tkx::bind("all", "<Key-F5>", sub { $sync_btn->invoke(); });
Tkx::tooltip($sync_btn, "Refresh all data [F5]");
$toolbar->add($sync_btn, -separator => 1, -pad => [4, 2]);

my $prefs_btn = $toolbar->new_ttk__button(
    -text => "Preferences",
    -image => $IMG{'prefs'},
    -style => "Toolbutton",
    -command => [\&show_prefs_dialog],
);
Tkx::bind("all", "<${plat_evt_ctrl}p>", sub { $prefs_btn->invoke(); });
Tkx::tooltip($prefs_btn, "PPM Preferences [${plat_acc_ctrl}P]");
$toolbar->add($prefs_btn);

if ($AQUA) {
    # Aqua isn't properly displaying the button disabled, so get the
    # effect through a greyed image
    for my $w ($filter_all, $filter_inst, $filter_upgr, $filter_mod,
	       $install_btn, $remove_btn, $go_btn, $sync_btn, $prefs_btn)
    {
	my $img = $w->cget("-image");
	if ($img) {
	    my $disimg = Tkx::ppm__img(
		Tkx::SplitList(Tkx::ppm__imgname($img)), "gray",
            );
	    $w->configure(-image => [$img, disabled => $disimg]);
	}
    }
}

## Statusbar items
my %NUM;
$NUM{'total'} = 0;
$NUM{'listed'} = 0;
$NUM{'installed'} = 0;
$NUM{'install'} = 0;
$NUM{'remove'} = 0;
my $lbl;
$lbl = $statusbar->new_ttk__label(-textvariable => \$NUM{'total'}, @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Total number of known packages");
$lbl = $statusbar->new_ttk__label(-text => "packages,", @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Total number of known packages");
$lbl = $statusbar->new_ttk__label(-textvariable => \$NUM{'listed'}, @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Number of packages in filtered view");
$lbl = $statusbar->new_ttk__label(-text => "listed", @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Number of packages in filtered view");
$lbl = $statusbar->new_ttk__label(-textvariable => \$NUM{'installed'}, @smallfont);
$statusbar->add($lbl, -separator => 1);
Tkx::tooltip($lbl, "Number of packages installed");
$lbl = $statusbar->new_ttk__label(-text => "installed,", @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Number of packages installed");
$lbl = $statusbar->new_ttk__label(-textvariable => \$NUM{'install'}, @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Number of packages selected for install");
$lbl = $statusbar->new_ttk__label(-text => "to install,", @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Number of packages selected for install");
$lbl = $statusbar->new_ttk__label(-textvariable => \$NUM{'remove'}, @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Number of packages selected for removal");
$lbl = $statusbar->new_ttk__label(-text => "to remove", -anchor => 'w', @smallfont);
$statusbar->add($lbl);
Tkx::tooltip($lbl, "Number of packages selected for removal");

$lbl = $statusbar->new_ttk__label(-text => "Install Area:", -anchor => 'e', @smallfont);
$statusbar->add($lbl, -separator => 1);
$lbl = $statusbar->new_ttk__label(-textvariable => \$INSTALL_AREA, @smallfontbold);
$statusbar->add($lbl);

my $progress = -1;
my $progressbar = $statusbar->new_ttk__progressbar(
    -mode => "determinate",
    -variable => \$progress,
    -max => 1,
    -length => 80,
);
$statusbar->add($statusbar->new_ttk__frame, -separator => 1, -weight => 10);
#$statusbar->add($progressbar, -weight => 1);

# Run preferences loading handler after UI has been instantiated
on_load();

# map all view items, but only call one of the sort* view items
map view($_), grep($_ ne "sortorder", keys %VIEW);

Tkx::interp_alias("", "pkg_upgradable", "", [sub {
    my($name, $area, $repo_pkg_id) = @_;
    my $ret = eval {
	my $installed = $ppm->area($area)->package($name);
	my $available = $ppm->package($repo_pkg_id);
	$available->better_than($installed);
    };
    if ($@) {
	ppm_log("WARN", $@);
    }
    return $ret || 0;  # Tcl need a real zero
}]);

# Now let's get started ...
Tkx::update('idletasks');

Tkx::after(idle => sub {
    $mw->g_wm_deiconify();
    Tkx::focus(-force => $filter);
    full_refresh(1);
});

Tkx::MainLoop();

1;

sub refresh {
    $pkglist->clear();
    $NUM{'listed'} = 0;
    my $area = merge_area_items();
    $NUM{'installed'} = $pkglist->numitems();
    my $repo = merge_repo_items();
    $NUM{'total'} = $pkglist->numitems();
    %ACTION = ();
    $NUM{'install'} = 0;
    $NUM{'remove'} = 0;
    update_actions();
    $pkglist->sort();
    filter(1);
    $NUM{'listed'} = $pkglist->numitems('visible');
}

sub repo_sync {
    my $sync = shift;
    if ($sync) {
	%REPOS = ();
	$ppm->repo_sync;
    }
    $repolist->clear() if $repolist;
    for my $repo_id ($ppm->repos) {
	my $repo = $REPOS{$repo_id} = $ppm->repo($repo_id);
	if ($repolist) {
	    $repolist->add($repo_id,
	        repo => $repo->{name},
		url => $repo->{packlist_uri},
		num => $repo->{pkgs},
		checked => $repo->{packlist_last_access},
	    );
	    $repolist->enable($repo_id, $repo->{enabled});
	}
    }
}

sub area_sync {
    $arealist->clear() if $arealist;
    for my $area_name ($ppm->areas) {
	my $area = $ppm->area($area_name);
	if ($arealist) {
	    $arealist->add($area->name,
	        num => scalar $area->packages,
		prefix => $area->prefix,
		inc => $area->inc,
	    );
	    $arealist->state($area->name, "readonly")
		if $area->readonly || $area_name eq "perl";
	}
    }
    my $i_area = $ppm->area($INSTALL_AREA);
    if (!$i_area || $i_area->readonly) {
	$INSTALL_AREA = $ppm->default_install_area || "";
    }
    if ($INSTALL_AREA && $arealist) {
	$arealist->state($INSTALL_AREA, "default");
    }
}

my %GRAB;

sub set_focus_grab {
    my $grab = shift;
    my $focus = shift || $grab;
    my $oldGrab = $GRAB{$grab}{$focus}{'grab'} = Tkx::grab(current => $grab);
    $GRAB{$grab}{$focus}{'focus'} = Tkx::focus();
    $GRAB{$grab}{$focus}{'status'} =
	Tkx::winfo_exists($oldGrab) ? Tkx::grab(status => $oldGrab) : "";
    eval { Tkx::grab($grab); Tkx::focus($focus); };
}

sub restore_focus_grab {
    my $grab = shift;
    my $focus = shift || $grab;
    Tkx::grab(release => $grab);
    return unless defined($GRAB{$grab}{$focus}{'grab'});
    my $oldFocus = $GRAB{$grab}{$focus}{'focus'};
    my $oldGrab = $GRAB{$grab}{$focus}{'grab'};
    my $oldStatus = $GRAB{$grab}{$focus}{'status'};
    if (Tkx::winfo_exists($oldFocus)) {
	Tkx::focus($oldFocus);
    }
    if (Tkx::winfo_exists($oldGrab) && Tkx::winfo_ismapped($oldGrab)) {
	if ($oldStatus eq "global") {
	    Tkx::grab("-global", $oldGrab);
	}
	else {
	    Tkx::grab($oldGrab);
	}
    }
    $GRAB{$grab}{$focus} = undef;
}

sub full_refresh {
    my $first_time = shift;
    my $activity = ppm_status("begin", "Synchronizing Database");
    $mw->configure(-cursor => "watch");
    Tkx::update();
    set_focus_grab($status_box);
    if ($first_time && $ppm->config_get("gui.skip_initial_repo_sync")) {
	status_message("\nExternal repositories not synced.  Press F5 for manual sync.\n");
	repo_sync();
    }
    else {
	repo_sync(1);
    }
    area_sync();
    refresh();
    $activity->end;

    $mw->configure(-cursor => "");
    restore_focus_grab($status_box);
    if ($first_time && $INSTALL_AREA eq "") {
	my $msg = "All activated install areas are read-only.  You cannot currently install or remove packages.";
	# do we have any uninitialized areas
	if (grep !$_->initialized, map $ppm->area($_), $ppm->areas) {
	    $msg .= "  Try enabling the uninitialized Areas in the Preferences dialog";
	}
	Tkx::tk___messageBox(
	    -title => "No writable installarea",
            -icon => "info",
            -type => "ok",
            -message => $msg,
	);
    }
}

sub merge_area_items {
    my $count = 0;
    for my $area_name ($ppm->areas) {
	my $area = $ppm->area($area_name);
	my @fields = ("name", "version", "abstract", "author");
	for my $pkg ($area->packages(@fields)) {
	    for (@$pkg) { $_ = "" unless defined }  # avoid "Use of uninitialized value" warnings
	    my ($name, $version, $abstract, $author) = @$pkg;
	    $pkglist->add($name,
		area => $area_name,
		installed => $version,
		abstract => $abstract,
		author => $author,
	    );
	    $count++;
	}
    }
    return $count;
}

sub merge_repo_items {
    my @fields = ("id", "name", "version", "abstract", "author", "repo_id", "cannot_install");
    my @res = $ppm->packages(@fields);
    my $count = @res;
    for (@res) {
	for (@$_) { $_ = "" unless defined }  # avoid "Use of uninitialized value" warnings
	my ($id, $name, $version, $abstract, $author, $repo_id, $cannot_install) = @$_;
	$version = "($version)" if $cannot_install;
	$pkglist->add($name,
            repo_pkg_id => $id,
            repo => $REPOS{$repo_id}->{name} || $repo_id,
	    available => $version,
	    abstract => $abstract,
	    author => $author,
	);
    }
    return $count;
}

sub filter {
    my $force = shift || 0;
    Tkx::after('cancel', $FILTER{'id'});
    return if !$force &&
              $FILTER{'filter'} eq $FILTER{'lastfilter'} &&
              $FILTER{'fields'} eq $FILTER{'lastfields'} &&
              $FILTER{'type'} eq $FILTER{'lasttype'};
    my $fields = $FILTER{'fields'};
    $fields =~ s/ / or /g;
    Tkx::tooltip($filter, "Filter packages by $fields [${plat_acc_ctrl}F]");
    my $count = $pkglist->filter($FILTER{'filter'},
        fields => $FILTER{'fields'},
	type => $FILTER{'type'},
    );
    if ($count == -1) {
	# Something wrong with the filter
	$filter->delete(0, "end");
	$filter->insert(0, $FILTER{'lastfilter'});
	# No need to refilter - should not have changed
    }
    else {
	$FILTER{'lastfilter'} = $FILTER{'filter'};
	$FILTER{'lastfields'} = $FILTER{'fields'};
	$FILTER{'lasttype'} = $FILTER{'type'};
	$NUM{'listed'} = $count;
    }
}

sub filter_onkey {
    Tkx::after('cancel', $FILTER{'id'});
    $FILTER{'id'} = Tkx::after($FILTER{'delay'}, [\&filter]);
}

sub ppm {
    my $func = shift;
    $ppm->$func(@_);
}

sub view {
    my $view = shift;
    if ($view =~ '^sort') {
	$pkglist->sort($VIEW{'sortcolumn'}, $VIEW{'sortorder'});
    }
    elsif ($view =~ 'bar$') {
	my $w = ($view eq 'statusbar' ? $statusbar : $toolbar);
	if ($VIEW{$view}) {
	    Tkx::grid($w);
	}
	else {
	    Tkx::grid('remove', $w);
	}
    }
    else {
	$pkglist->view($view, $VIEW{$view});
    }
}

sub verify {
    my $package = shift;
    my @areas = grep $_->initialized, map $ppm->area($_), $ppm->areas;
    if ($package) {
	@areas = grep $_->package_id($package), @areas;
	unless (@areas) {
	    # can't happen
	    status_message("Package '$package' is not installed\n");
	    return;
	}
	status_message("Verifying $package ...\n", "h2");
    }
    else {
	status_message("Verifying all packages ...\n", "h2");
    }
    my %status;
    for my $area (@areas) {
	my %s = $area->verify(
	    package => $package,
            badfile_cb => sub {
               my $what = shift;
               if ($what eq "wrong_mode") {
                   status_message(sprintf "%s: wrong mode %03o expected %03o\n", @_);
               }
               else {
		   status_message("$_[0]: $what\n");
               }
            },
        );
	while (my($k,$v) = each %s) {
	    $status{$k} += $v;
	}
    }
    for my $v (qw(verified missing modified)) {
	next if $v ne "verified" && !$status{$v};
	my $s = $status{$v} == 1 ? "" : "s";
	status_message("$status{$v} file$s $v\n");
    }
}

sub menus {
    Tkx::option_add("*Menu.tearOff", 0);
    my $menu = $mw->new_menu();

    my ($sm, $ssm);

    # Special menu on OS X.  For Cocoa the .apple menu must be the first
    # and must exist prior to attaching the menu to a toplevel.
    if ($AQUA) {
	$sm = $menu->new_menu(-name => 'apple'); # must be named "apple"
	$menu->add_cascade(-label => "PPM", -menu => $sm);
	$sm->add_command(
	    -label => "About PPM",
	    -command => sub { about(); },
        );
	$sm->add_separator();
	# OS X enables the Preferences item when you create this proc
	Tkx::proc("tk::mac::ShowPreferences", "args", [\&show_prefs_dialog]);
	Tkx::bind(all => "<Command-comma>", "tk::mac::ShowPreferences");
    }

    # File menu
    $sm = $file_menu = $menu->new_menu(-name => "file");
    $menu->add_cascade(-label => "File", -menu => $sm, -underline => 0);
    $sm->add_command(
        -label => "Refresh All Data",
        -underline => 1,
        -accelerator => "F5",
	-command => sub { $sync_btn->invoke(); },
    );
    $sm->add_command(
        -label => "Verify Packages",
        -underline => 0,
	-command => [\&verify],
    );
    $sm->add_command(
        -label => "Run Marked Actions",
        -underline => 0,
	-state => "disabled",
	-accelerator => "${plat_acc_ctrl}Enter",
	-command => sub { $go_btn->invoke(); },
    );
    $mw->g_bind("<<RunActions>>" => sub { $go_btn->invoke(); });
    Tkx::event("add", "<<RunActions>>", "<${plat_evt_ctrl}Key-Return>",
	       "<${plat_evt_ctrl}Key-KP_Enter>");
    Tkx::bind(all => "<${plat_evt_ctrl}q>" => [\&on_exit]);
    if (!$AQUA) {
	$sm->add_separator();
	$sm->add_command(
	    -label => "Exit",
            -underline => 1,
	    -accelerator => "${plat_acc_ctrl}Q",
	    -command => [\&on_exit],
        );
    }

    # Edit menu
    $sm = $menu->new_menu(-name => "edit");
    $menu->add_cascade(-label => "Edit", -menu => $sm, -underline => 0);
    $sm->add_command(
        -label => "Cut",
        -underline => 2,
	-accelerator => "${plat_acc_ctrl}X",
	-command => sub {
	    Tkx::event_generate(Tkx::focus(), "<<Cut>>");
	},
    );
    $sm->add_command(
        -label => "Copy",
        -underline => 0,
	-accelerator => "${plat_acc_ctrl}C",
	-command => sub {
	    Tkx::event_generate(Tkx::focus(), "<<Copy>>");
	},
    );
    $sm->add_command(
        -label => "Paste",
        -underline => 0,
	-accelerator => "${plat_acc_ctrl}V",
	-command => sub {
	    Tkx::event_generate(Tkx::focus(), "<<Paste>>");
	},
    );
    if (!$AQUA) {
	$sm->add_separator();
	$sm->add_command(
	    -label => "Preferences",
            -underline => 1,
	    -accelerator => "${plat_acc_ctrl}P",
	    -command => [\&show_prefs_dialog],
        );
    }

    # View menu
    $sm = $view_menu = $menu->new_menu(-name => "view");
    $menu->add_cascade(-label => "View", -menu => $sm, -underline => 0);
    $sm->add_checkbutton(
        -label => "Toolbar",
        -underline => 0,
	-variable => \$VIEW{'toolbar'},
	-command => [\&view, 'toolbar'],
    );
    $sm->add_checkbutton(
        -label => "Status Bar",
        -underline => 0,
	-variable => \$VIEW{'statusbar'},
	-command => [\&view, 'statusbar'],
    );
    $sm->add_separator();
    $sm->add_radiobutton(
        -label => "All Packages",
        -underline => 0,
	-variable => \$FILTER{'type'},
        -value => "all",
	-accelerator => "${plat_acc_ctrl}1",
	-command => [\&filter],
    );
    $sm->add_radiobutton(
        -label => "Installed Packages",
        -underline => 0,
	-variable => \$FILTER{'type'},
        -value => "installed",
	-accelerator => "${plat_acc_ctrl}2",
	-command => [\&filter],
    );
    $sm->add_radiobutton(
        -label => "Upgradable Packages",
        -underline => 0,
	-variable => \$FILTER{'type'},
        -value => "upgradable",
	-accelerator => "${plat_acc_ctrl}3",
	-command => [\&filter],
    );
    # this text linked in update_actions for entryconfigure
    $sm->add_radiobutton(
        -label => "Packages to Install/Remove",
        -underline => 0,
	-variable => \$FILTER{'type'},
        -value => "modified",
	-accelerator => "${plat_acc_ctrl}4",
	-command => [\&filter],
    );

    $sm->add_separator();
    $ssm = $sm->new_menu(-name => "text");
    $sm->add_cascade(-label => "Text Size", -menu => $ssm, -underline => 7);
    # These commands rely on the internal bindings by style::as MouseWheel
    my $incr_text_cmd = sub {
	my $X = Tkx::winfo_rootx($pkglist);
	my $Y = Tkx::winfo_rooty($pkglist);
	Tkx::style__as__CtrlMouseWheel($pkglist, $X, $Y, 120, "");
    };
    my $decr_text_cmd = sub {
	my $X = Tkx::winfo_rootx($pkglist);
	my $Y = Tkx::winfo_rooty($pkglist);
	Tkx::style__as__CtrlMouseWheel($pkglist, $X, $Y, -120, "");
    };
    $ssm->add_command(
	-label => "Increase",
	-underline => 0,
	-accelerator => "${plat_acc_ctrl}+",
	-command => $incr_text_cmd,
    );
    $ssm->add_command(
	-label => "Decrease",
	-underline => 0,
	-accelerator => "${plat_acc_ctrl}-",
	-command => $decr_text_cmd,
    );
    if (0) {
	# style::as currently has no sense of "Normal", just relative
	$ssm->add_separator();
	$ssm->add_command(
	    -label => "Normal",
	    -underline => 0,
	);
    }
    # Use both equal and plus so users don't have to hit Shift on US kbds
    $mw->g_bind("<${plat_evt_ctrl}plus>" => $incr_text_cmd);
    $mw->g_bind("<${plat_evt_ctrl}equal>" => $incr_text_cmd);
    $mw->g_bind("<${plat_evt_ctrl}minus>" => $decr_text_cmd);

    $sm->add_separator();
    $ssm = $fields_menu = $sm->new_menu(-name => "cols");
    $sm->add_cascade(-label => "View Columns", -menu => $ssm, -underline => 5);
    $ssm->add_checkbutton(
        -label => "Area",
        -underline => 1,
	-variable => \$VIEW{'area'},
	-command => [\&view, 'area'],
    );
    $ssm->add_checkbutton(
        -label => "Installed",
        -underline => 0,
	-variable => \$VIEW{'installed'},
	-command => [\&view, 'installed'],
    );
    $ssm->add_checkbutton(
        -label => "Repo",
        -underline => 1,
	-variable => \$VIEW{'repo'},
	-command => [\&view, 'repo'],
    );
    $ssm->add_checkbutton(
        -label => "Available",
        -underline => 1,
	-variable => \$VIEW{'available'},
	-command => [\&view, 'available'],
    );
    $ssm->add_checkbutton(
        -label => "Abstract",
        -underline => 1,
	-variable => \$VIEW{'abstract'},
	-command => [\&view, 'abstract'],
    );
    $ssm->add_checkbutton(
        -label => "Author",
        -underline => 1,
	-variable => \$VIEW{'author'},
	-command => [\&view, 'author'],
    );
    $ssm = $sort_menu = $sm->new_menu(-name => "sort");
    $sm->add_cascade(-label => "Sort Column", -menu => $ssm, -underline => 1);
    my @sort_opts = (
        -variable => \$VIEW{'sortcolumn'},
	-command => [\&view, 'sort'],
    );
    $ssm->add_radiobutton(
        -label => "Package Name",
	-underline => 0,
        -value => 'name',
        @sort_opts,
    );
    $ssm->add_radiobutton(
        -label => "Area",
	-underline => 1,
        -value => 'area',
        @sort_opts,
    );
    $ssm->add_radiobutton(
        -label => "Installed",
	-underline => 0,
        -value => 'installed',
        @sort_opts,
    );
    $ssm->add_radiobutton(
        -label => "Repo",
	-underline => 1,
        -value => 'repo',
        @sort_opts,
    );
    $ssm->add_radiobutton(
        -label => "Available",
	-underline => 1,
        -value => 'available',
        @sort_opts,
    );
    $ssm->add_radiobutton(
        -label => "Abstract",
	-underline => 1,
        -value => 'abstract',
        @sort_opts,
    );
    $ssm->add_radiobutton(
        -label => "Author",
	-underline => 1,
        -value => 'author',
        @sort_opts,
    );
    $ssm->add_separator();
    @sort_opts = (
        -variable => \$VIEW{'sortorder'},
	-command => [\&view, 'sort'],
    );
    $ssm->add_radiobutton(
        -label => "Ascending",
	-underline => 0,
        -value => '-increasing',
        @sort_opts,
    );
    $ssm->add_radiobutton(
        -label => "Descending",
	-underline => 0,
        -value => '-decreasing',
        @sort_opts,
    );

    if ($ENV{'ACTIVEPERL_PPM_DEBUG'}) {
	$sm->add_separator();
	$sm->add_command(
	    -label => "Debug Console",
	    -command => sub { Tkx::catch("tkcon show"); },
	);
    }

    # Action menu
    $action_menu = $sm = $menu->new_menu(-name => "action");
    $menu->add_cascade(-label => "Action", -menu => $sm, -underline => 0);
    $sm->add_command(-label => "No Selected Package", -state => "disabled");

    # Help menu - name it help for special behavior, but not on OS X, where
    # that causes us to not get cascades allowed.
    my $mname = ($AQUA ? "nothelp" : "help");
    $sm = $menu->new_menu(-name => $mname);
    $menu->add_cascade(-label => "Help", -menu => $sm, -underline => 0);
    if (ActiveState::Browser::can_open("faq/ActivePerl-faq2.html")) {
	my $help_cmd = [\&ActiveState::Browser::open, "faq/ActivePerl-faq2.html"];
	$sm->add_command(
	    -label => "Contents",
            -underline => 0,
	    -accelerator => "<F1>",
	    -command => $help_cmd,
	);
	Tkx::bind("all", "<Key-F1>", $help_cmd);
    }
    if (ActiveState::Browser::can_open("index.html")) {
	$sm->add_command(
	    -label => "ActivePerl User Guide",
            -underline => 11,
	    -command => [\&ActiveState::Browser::open, "index.html"],
	);
    }
    if (ActiveState::Browser::can_open("http://www.activestate.com")) {
	my $web = $sm->new_menu(-tearoff => 0);
	$sm->add_cascade(
	    -label => "Web Resources", -underline => 0,
	    -menu => $web,
        );

        $web->add_command(
            -label => "Report Bug",
            -command => [\&ActiveState::Browser::open,
			 "http://bugs.activestate.com/enter_bug.cgi?product=ActivePerl"],
        );
        $web->add_command(
            -label => "ActiveState Repository",
            -command => [\&ActiveState::Browser::open,
			 "http://ppm.activestate.com/"],
        );
        $web->add_command(
            -label => "ActivePerl Home",
            -command => [\&ActiveState::Browser::open,
			 "http://www.activestate.com/Products/ActivePerl/"],
        );
	$web->add_command(
	    -label => "ActivePerl Business Edition",
	    -command => [\&ActiveState::Browser::open,
		         "http://www.activestate.com/business_edition/"],
	);
	if ($ppm->be_state eq "expired") {
	    $web->add_command(
		-label => "Renew ActivePerl Business Edition subscription",
		-command => [\&ActiveState::Browser::open,
			     "https://account.activestate.com/"],
	    );
	}
        $web->add_command(
            -label => "ActiveState Home",
            -command => [\&ActiveState::Browser::open, "http://www.activestate.com"],
        );
    }

    if (!$AQUA) {
	$sm->add_separator;
	$sm->add_command(
	    -label => "About",
            -underline => 0,
	    -command => sub { about(); },
        );
    }

    $mw->configure(-menu => $menu);
    return $menu;
}

sub select_item {
    my $item = shift;
    $details->configure(-state => "normal");
    $details->delete('1.0', 'end');
    $details->configure(-state => "disabled");
    my $menu = $action_menu;
    $menu->delete(0, 'end');
    $menu->add_command(-label => "No selected package", -state => "disabled");
    return unless $item;

    # We need to figure out how we want details formatted
    my %data = Tkx::SplitList($pkglist->data($item));
    my $name = $data{'name'};
    my $areaid = $data{'area'};
    my $area = $ppm->area($areaid) if $areaid;
    my ($pkg, $repo_pkg);
    $pkg = $repo_pkg = $ppm->package($data{'repo_pkg_id'});
    if ($areaid) {
	if (my $p = $area->package($name)) {
	    $pkg = $p;
	}
    }
    return unless $pkg;

    my $installable = $data{'available'};

    my $pad = "\t";
    $details->configure(-state => "normal");
    $details->insert('1.0', "$pkg->{name}\n", 'h1');
    $details->insert('end', "$pkg->{abstract}\n", 'abstract') if $pkg->{abstract};
    $details->insert('end', "${pad}Version:\t$pkg->{version}\n");
    if (my $why = $repo_pkg && $ppm->cannot_install($repo_pkg)) {
	if ($why =~ /business edition/) {
	    my @why = split(/((?:activeperl )?business edition)/, $why, 2);
	    $details->insert("end -2 char", " - can't install: $why[0]", "abstract");
	    $details->insert("end -2 char", "$why[1]", "abstract link be-link");
	    $details->insert("end -2 char", "$why[2]", "abstract");
	    $details->tag_bind('be-link', "<ButtonRelease-1>", [
	        \&ActiveState::Browser::open, "http://www.activestate.com/ActivePerl-BE",
	    ]);
	}
	else {
	    $details->insert('end -2 char', " — can't install: $why", 'abstract');
	}
	$installable = 0;
    }
    if (my $date = $pkg->{release_date}) {
	$date =~ s/ .*//;  # drop time
	$details->insert('end', "${pad}Released:\t$date\n");
    }
    $details->insert('end', "${pad}Author:\t$pkg->{author}\n") if $pkg->{author};
    if (my $name = is_cpan_package($pkg->{name})) {
	my $v = $pkg->{version};
	$v =~ s/-r\d+$//;
	my $cpan_url = "http://search.cpan.org/dist/$name-$v/";
	if ($pkg->{name} eq "Perl") {
	    $cpan_url = sprintf "http://search.cpan.org/dist/perl-%vd", $^V;
	}
	$details->insert('end', "${pad}CPAN:\t");
	if (ActiveState::Browser::can_open($cpan_url)) {
	    $details->insert('end', $cpan_url, "link cpan-link");
	    $details->tag_bind('cpan-link', "<ButtonRelease-1>", [
	        \&ActiveState::Browser::open, $cpan_url
	    ]);
	}
	else {
	    $details->insert('end', $cpan_url);
	}
	$details->insert('end', "\n");
    }
    if ($areaid) {
	my @files = $area->package_files($pkg->{id});
	if (@files) {
	    $details->insert('end', "\nInstalled files:\n", 'h2');
	    for my $file (@files) {
		$details->insert('end', "\t$file\n");
	    }
	}
    }
    # Remove trailing newline and prevent editing of widget
    $details->delete('end-1c');
    $details->configure(-state => "disabled");
    $pw_nb->select($details_sw);

    ## Record "allowable" actions based on package info
    # XXX work on constraints
    unless ($ACTION{$item}) {
	$ACTION{$item}{'install'} = 0;
	$ACTION{$item}{'remove'} = 0;
	$ACTION{$item}{'area'} = $area;
	$ACTION{$item}{'pkg'} = $pkg;
	$ACTION{$item}{'repo_pkg'} = $repo_pkg;
	$ACTION{$item}{'deps'} = [];
    }
    # The icon represents the current actionable state:
    #   default installed upgradable install remove upgrade
    $remove_btn->configure(-state => "disabled", -variable => \$dummy);
    $install_btn->configure(-state => "disabled", -variable => \$dummy);
    $menu->delete(0, 'end');
    my ($txt, $cmd);
    if ($data{'installed'}) {
	# installed items are removable
	$txt = "Remove $name $data{'installed'}";
	if ($data{'area'} && (($data{'area'} eq "perl")
				  || $ppm->area($data{'area'})->readonly)) {
	    # perl area items should not be removed
	    $menu->add_command(-label => $txt, -state => "disabled", -accelerator => '-');
	}
	else {
	    $cmd = sub { queue_action($item, $name, "remove"); };
	    $remove_btn->configure(
	        -state => "normal",
                -command => $cmd,
		-variable => \$ACTION{$item}{'remove'},
            );
	    $menu->add_checkbutton(
                -label => $txt,
                -command => $cmd,
		-variable => \$ACTION{$item}{'remove'},
		-accelerator => '-',
            );
	}
    }
    if ($installable) {
	if ($data{'available'} eq $data{'installed'}) {
	    $txt = "Reinstall $name $data{'available'}";
	}
	else {
	    $txt = "Install $name $data{'available'}";
	}
	if (!$INSTALL_AREA
	    || $INSTALL_AREA eq "perl" || $ppm->area($INSTALL_AREA)->readonly) {
	    $menu->add_command(-label => $txt, -state => "disabled", -accelerator => '+');
	}
	else {
	    $cmd = sub { queue_action($item, $name, "install"); };
	    $install_btn->configure(
	        -state => "normal",
                -command => $cmd,
		-variable => \$ACTION{$item}{'install'},
            );
	    $menu->add_checkbutton(
                -label => $txt,
                -command => $cmd,
		-variable => \$ACTION{$item}{'install'},
		-accelerator => '+',
            );
	}
    }
    if (!$data{'available'} && !$data{'installed'}) {
	# Oddball packages that have no version?
	$menu->add_command(-label => $name, -state => "disabled");
    }
    if ($data{'installed'}) {
	# Add "Verify" action
	$menu->add_separator();
	$menu->add_command(
            -label => "Verify $name $data{'installed'}",
	    -command => [\&verify, $name],
        );
    }
}

sub queue_action {
    my ($item, $name, $action) = @_;
    my $altact = ($action eq "install") ? "remove" : "install";
    if ($ACTION{$item}{'remove'} && $ACTION{$item}{'install'}) {
	# only allow install OR remove, not both.
	$ACTION{$item}{$altact} = 0;
	$pkglist->state($item, "!$altact");
	$NUM{$altact}--;
	status_message("$name unmarked for $altact\n");
    }
    if ($ACTION{$item}{$action}) {
	$pkglist->state($item, $action);
	$NUM{$action}++;
	status_message("$name marked for $action\n");
    }
    else {
	$pkglist->state($item, "!$action");
	$NUM{$action}--;
	status_message("$name unmarked for $action\n");
    }
    if ($ACTION{$item}{'install'}) {
	# Figure out what dependent modules are required
	my $repo_pkg = $ACTION{$item}{'repo_pkg'};
	my @pkgs = map $ACTION{$_}{'repo_pkg'},
	    grep($ACTION{$_}{'install'}, keys %ACTION);

	if (my $err = $ppm->is_downgrade($repo_pkg)) {
	    ppm_log("WARN", $err);
	    status_message("\nWARNING: $err\n\n", "abstract");
	}

	my @tmp = $ppm->packages_missing(
	     have => \@pkgs,
	     want_deps => [$repo_pkg],
             error_handler => sub {
		 status_message("\nWARNING: $_\n\n", "abstract")
		     for @_;
             },
        );
	my @need;
	for my $pkg (@tmp) {
	    status_message("$repo_pkg->{name} depends on $pkg->{name}\n", "abstract");
	    push(@need, $pkg);
	}
	$ACTION{$item}{'deps'} = \@need;
    }
    if ($ACTION{$item}{'remove'}) {
	# Find out if removing this breaks dependencies
	my $name = $ACTION{$item}{'pkg'}->name;
	my @deps = $ppm->packages_depending_on($ACTION{$item}{'pkg'},
					       $ACTION{$item}{'area'});
	$ACTION{$item}{'deps'} = \@deps;
	if (@deps) {
	    my @dep_names = map $_->{name}, @deps;
	    my $s = @dep_names == 1 ? "s" : "";
	    status_message("\nWARNING: " . join_with("and", sort @dep_names) . " depend$s on $name to be available\n\n", "abstract");
	}
    }
    update_actions();
}

sub update_actions {
    if ($NUM{'install'} || $NUM{'remove'}) {
	$go_btn->configure(-state => "normal");
	$file_menu->entryconfigure("Run Marked Actions", -state => "normal");
	$filter_mod->configure(-state => "normal");
	$view_menu->entryconfigure("Packages to Install/Remove",
	    -state => "normal",
        );
    }
    else {
	$go_btn->configure(-state => "disabled");
	$file_menu->entryconfigure("Run Marked Actions", -state => "disabled");
	$filter_mod->configure(-state => "disabled");
	$view_menu->entryconfigure("Packages to Install/Remove",
	    -state => "disabled",
        );
    }
}

sub run_actions {
    my $msg = "Ready to ";
    if ($NUM{'install'}) {
	$msg .= "install $NUM{'install'} package";
	$msg .= "s" if $NUM{'install'} > 1;
    }
    if ($NUM{'remove'}) {
	$msg .= " and " if $NUM{'install'};
	$msg .= "remove $NUM{'remove'} package";
	$msg .= "s" if $NUM{'remove'} > 1;
    }
    $msg .= "?";
    my @items =	grep($ACTION{$_}{'install'} && @{$ACTION{$_}{'deps'}}, keys %ACTION);
    if (@items) {
	my $dep_cnt = 0;
	for my $item (@items) {
	    my $repo_pkg = $ACTION{$item}{'repo_pkg'};
	    for my $pkg (@{$ACTION{$item}{'deps'}}) {
		$dep_cnt++;
	    }
	}
	$msg .= "\nPPM will install $dep_cnt additional prerequisite packages.";
    }
    @items =
	grep($ACTION{$_}{'remove'} && @{$ACTION{$_}{'deps'}}, keys %ACTION);
    if (@items) {
	my $dep_cnt = 0;
	for my $item (@items) {
	    my $name = $ACTION{$item}{'pkg'}->name;
	    for my $pkg (@{$ACTION{$item}{'deps'}}) {
		$dep_cnt++;
	    }
	}
	$msg .= "\n$dep_cnt dependent packages may become unusable.";
    }
    my $res = Tkx::tk___messageBox(
	-title => "Commit Actions?", -type => "okcancel", -parent => $mw,
	-icon => "question", -message => $msg,
    );
    if ($res eq "ok") {
	commit_actions();
    }
}

sub commit_actions {
    my $removed;
    for my $item (sort keys %ACTION) {
	# First remove any area pacakges
	if ($ACTION{$item}{'remove'}) {
	    my $name = $pkglist->data($item, "name");
	    my $area = $ACTION{$item}{'area'};
	    my $area_name = $area->name;
	    my $activity = ppm_status("begin", "Removing $name from $area_name area");
	    eval {
		my $pkg = $area->package($name);
		die "$name not installed in area $area_name" unless $pkg;
		$pkg->run_script("uninstall", $area, undef, {
		    old_version => $pkg->{version},
		    packlist => $area->package_packlist($pkg->{id}),
		}, \&run_cmd);
		$area->uninstall($name);
	    };
	    if ($@) {
		{ local $@; $activity->end("failed"); }
		status_error();
		return;
	    }
	    else {
		$removed++;
	    }
	}
    }

    my @install_pkgs = map($ACTION{$_}{'repo_pkg'},
			   grep($ACTION{$_}{'install'}, keys %ACTION));
    if (@install_pkgs) {
	eval {
	    my @need = $ppm->packages_missing(
	        have => \@install_pkgs,
		want_deps => \@install_pkgs,
		force => 1,  # dependency warnings already displayed
            );
	    push(@install_pkgs, grep !$_->has_script("install"), @need);
	};
	if ($@) {
	    status_error();
	}
	my $what = @install_pkgs > 1 ? (@install_pkgs . " packages") : "package";
	my $activity = ppm_status("begin", "Installing $what");
	eval {
	    $ppm->install(
		area => $INSTALL_AREA,
		packages => \@install_pkgs,
		run_cb => \&run_cmd,
            );
	};
	if ($@) {
	    { local $@; $activity->end("failed"); }
	    status_error();
	    return;
	}
    }
    elsif ($removed) {
	update_html_toc();
    }

    # Don't remain in "upgradable" or "modified" filter state
    $FILTER{'type'} = "installed" unless $FILTER{'type'} eq "all";
    refresh();
}

sub run_cmd {
    my @cmds = @_;
    my $ignore_err = $cmds[0] =~ s/^-//;
    if ($cmds[0] =~ s/^\s*\@(-)?//) {
	$ignore_err = 1 if $1;
    }
    #status_message("\n  @_\n");
    my $null = devnull();
    $cmds[0] =~ s,\\,/,g;
    my $fh = Tkx::open("| @cmds <$null 2>\@1");
    Tkx::fconfigure($fh, -blocking => 0);
    Tkx::fileevent($fh, readable => [\&run_watch, $fh]);

    # wait for the file to be closed before we return
    Tkx::vwait("run_done");
}

sub run_watch {
    my $fh = shift;
    my($buf, $n);
    eval { $n = Tkx::gets($fh, \$buf); };
    if ($@) {
	eval { Tkx::close($fh); };
	Tkx::set(run_done => 1);
	return;
    }
    if ($n == -1) {
	if (Tkx::eof($fh)) {
	    eval {Tkx::close($fh);};
	    Tkx::set(run_done => 1);
	}
	return;
    }

    status_message("  | $buf\n");
}

sub select_repo_item {
    my $item = shift;
    return unless $item;
    my $what = shift || "";
    my %data = Tkx::SplitList($repolist->data($item));
    # We need to figure out how we want details formatted
    if ($what eq "remove") {
	my $msg = "Really remove $data{repo} repository?"
	    . "\nDisabling a repository has the same effect"
	    . "\nwithout losing its name or location.";
	my $res = Tkx::tk___messageBox(
	    -title => "Remove Repository?",
	    -icon => "warning",
	    -type => "yesno",
	    -message => $msg,
	    -parent => $prefs_dialog,
	);
	return unless $res eq "yes";
	$ppm->repo_delete($data{id});
	full_refresh();
	return;
    }
    if ($what eq "enable") {
	my $state = $repolist->enable($data{id});
	$state = $ppm->repo_enable($data{id}, $state);
	# feed back result, in case we aren't allowed to change state
	$repolist->enable($data{id}, $state);
	full_refresh();
	return;
    }
    if ($what eq "setname") {
	my $newname = shift;
	$ppm->repo_set_name($data{id}, $newname);
	return;
    }
    if ($what eq "seturl") {
	my $newurl = shift;
	eval { $ppm->repo_set_packlist_uri($data{id}, $newurl); };
	if ($@) {
	    status_error("modifying repository URI");
	}
	else {
	    full_refresh();
	}
	return;
    }
};

sub select_area_item {
    my $item = shift;
    return unless $item;
    my $what = shift || "";
    my %data = Tkx::SplitList($arealist->data($item));
    if ($what eq "default") {
	if (!$ppm->area($data{name})->initialized) {
	    my $res = Tkx::tk___messageBox(
		-title => "Initialize Area $data{name}?",
		-message => "Should PPM start tracking packages in $data{name}?",
		-type => "okcancel",
		-icon => "question",
	    );
	    if ($res eq "ok") {
		eval { $ppm->area($data{name})->initialize(); };
		if ($@) {
		    status_error("initializing $data{name} area");
		}
		elsif (!$ppm->area($data{name})->readonly) {
		    $arealist->state($data{name}, "!readonly");
		}
	    }
	}
	if ($data{name} eq "perl" || $ppm->area($data{name})->readonly) {
	    Tkx::bell();
	    return;
	}
	eval { $arealist->state($INSTALL_AREA, "!default"); };
	$INSTALL_AREA = $data{name};
	$arealist->state($INSTALL_AREA, "default");
    }
}

sub show_prefs_dialog {
    if ($prefs_dialog && Tkx::winfo_exists($prefs_dialog)) {
	$prefs_dialog->display();
	Tkx::focus(-force => $prefs_dialog);
	return;
    }

    my $top = $prefs_dialog = $mw->new_widget__dialog(
	-title => 'PPM Preferences',
	-padding => 4,
	-parent => $mw,
        -place => 'over',
	-type => 'ok',
        -modal => 'none',
	-synchronous => 0,
        -separator => 0,
    );

    # Preferences tabs
    my $nb = $top->new_ttk__notebook();
    Tkx::ttk__notebook__enableTraversal($nb);
    $top->setwidget($nb);

    my ($f, $sw);
    # Areas tab
    $f = $nb->new_ttk__frame(-padding => 8);
    $nb->add($f, -text => "Areas", -underline => 0);
    # Select this tab as default
    $nb->select($f);

    $sw = $f->new_widget__scrolledwindow();
    $arealist = $sw->new_arealist(
        -width => 450,
        -height => 100,
	-selectcommand => [\&select_area_item],
	-borderwidth => 1,
        -relief => 'sunken',
	-itembackground => ["#F7F7FF", ""],
    );
    $sw->setwidget($arealist);
    my $areal = $f->new_widget__panelframe(-text => "Add Area:");
    my $areaf = $areal->new_ttk__frame(-padding => [6, 2]);
    $areal->setwidget($areaf);

    Tkx::grid(columnconfigure => $areaf, 0, -weight => 1);

    Tkx::grid($sw, -sticky => 'news');
    #Tkx::grid($areal, -sticky => 'news', -pady => [4, 0]);
    Tkx::grid(columnconfigure => $f, 0, -weight => 1);
    Tkx::grid(rowconfigure => $f, 0, -weight => 1);

    # Repositories tab
    $f = $nb->new_ttk__frame(-padding => 8);
    $nb->add($f, -text => "Repositories", -underline => 0);

    $sw = $f->new_widget__scrolledwindow();
    $repolist = $sw->new_repolist(
        -width => 450,
        -height => 100,
	-selectcommand => [\&select_repo_item],
	-borderwidth => 1,
        -relief => 'sunken',
	-itembackground => ["#F7F7FF", ""],
    );
    $sw->setwidget($repolist);
    my $addl = $f->new_widget__panelframe(-text => "Add Repository:");
    my $addf = $addl->new_ttk__frame(-padding => [6, 2]);
    $addl->setwidget($addf);
    my $name_var = "";
    my $uri_var = "";
    my $sug_repo_var = "";
    my ($rnamel, $rnamee, $rlocnl, $rlocne);
    my $val_cmd = sub {
	my ($peek, $w) = @_;
	my $state = $peek ? "!invalid" : "invalid";
	$w->state($state);
	return 1;
    };
    $rnamel = $addf->new_ttk__label(-text => "Name:", -anchor => 'w');
    $rnamee = $addf->new_ttk__entry(
        -textvariable => \$name_var,
	-validate => "all",
	-validatecommand => [$val_cmd, Tkx::Ev('%P'), $rnamel],
    );
    $rlocnl = $addf->new_ttk__label(-text => "Location:", -anchor => 'w');
    $rlocne = $addf->new_ttk__entry(
        -textvariable => \$uri_var,
	-validate => "all",
	-validatecommand => [$val_cmd, Tkx::Ev('%P'), $rlocnl],
    );
    $rnamel->state("invalid");
    $rlocnl->state("invalid");
    my $lastdir = cwd();
    my $dircmd = sub {
	my $dir = Tkx::tk___chooseDirectory(
            -title => "Repository Directory",
	    -initialdir => $lastdir,
	    -parent => $top,
	    -mustexist => 1,
        );
	if ($dir) {
	    $uri_var = $lastdir = $dir;
	    $rlocne->selection_clear();
	    $rlocne->icursor("end");
	    $rlocne->g_focus();
	}
    };
    my $dir_btn = $addf->new_ttk__button(
        -image => [Tkx::ppm__img('dir')],
	-command => $dircmd,
    );
    my $add_sub = sub {
	return unless $name_var && $uri_var;
	# This requires duplication of code from do_repo
	if ($uri_var =~ m,\?urn:/,) {
	    Tkx::tk___messageBox(
	        -title => "Error Adding Repository",
		-message => "PPM3 SOAP repositories are not supported.",
		-type => "ok",
                -icon => "error",
            );
	    return;
	}
	if (-d $uri_var) {
	    require URI::file;
	    $uri_var = URI::file->new_abs($uri_var);
	}
	($uri_var, my $uri_noarch) = split /\0/, $uri_var;
	eval { $ppm->repo_add(name => $name_var, packlist_uri => $uri_var); };
	if ($uri_noarch && !$@) {
	    eval { $ppm->repo_add(name => "$name_var-noarch", packlist_uri => $uri_noarch); };
	}
	if ($@) {
	    ppm_log("ERR", "Adding repository: $@");
	    Tkx::tk___messageBox(
                -title => "Error Adding Repository",
		-message => "Error adding repository:\n" . clean_err($@),
		-type => "ok",
                -icon => "error",
            );
	}
	else {
	    $name_var = "";
	    $uri_var = "";
	    $rnamel->state("invalid");
	    $rlocnl->state("invalid");
	    full_refresh();
	}
    };
    my $save_btn = $addf->new_ttk__button(
        -text => "Add",
	-command => $add_sub,
    );
    my $ret_cmd = sub {
	my $next = shift;
	if ($name_var && $uri_var) {
	    # Do add
	    $save_btn->invoke();
	}
	else {
	    Tkx::focus($next);
	}
    };
    $rnamee->g_bind("<Return>", [$ret_cmd, $rlocne]);
    $rlocne->g_bind("<Return>", [$ret_cmd, $rnamee]);
    my @repos;
    my $rsuglbl = $addf->new_ttk__label(
	-text => "Suggested:",
    );
    my $rsugbox = $addf->new_ttk__combobox(
	-textvariable => \$sug_repo_var,
	-state => "readonly",
    );
    eval {
	require PPM::Repositories;
	require ActivePerl;
    };
    if ($@) {
	$rsugbox->set("Install PPM-Repositories");
	$rsugbox->configure(-values => [], -state => "disabled");
    }
    else {
	if (defined &PPM::Repositories::list) {
	    for my $name (PPM::Repositories::list()) {
		# XXX Why don't we allow re-adding the ActiveState repo here?
		next if $name eq "activestate";
		my %repo = PPM::Repositories::get($name);
		push(@repos, "$name :: $repo{desc}");
	    }
	    $rsugbox->set("Select from list ...");
	    $rsugbox->configure(-values => [@repos], -state => "readonly");
	    my $rsugbox_cmd = sub {
		return unless defined &PPM::Repositories::get;
		return unless $sug_repo_var;
		$name_var = $sug_repo_var;
		$name_var =~ s/ ::.*$//;
		my %repo = PPM::Repositories::get($name_var);
		$uri_var = $repo{packlist} || $repo{packlist_noarch};
		if ($repo{packlist_noarch} && $repo{packlist_noarch} ne $uri_var) {
		    $uri_var .= "\0$repo{packlist_noarch}";
		}
	    };
	    Tkx::bind($rsugbox, "<<ComboboxSelected>>", $rsugbox_cmd);
	}
	else {
	    for my $id (sort keys %PPM::Repositories::Repositories) {
		my $repo = $PPM::Repositories::Repositories{$id};
		next unless $repo->{Active};
		next if $repo->{Type} eq "PPMServer";
		my $o = $repo->{PerlO} || [];
		next if @$o && !grep $_ eq $^O, @$o;
		my $v = $repo->{PerlV} || [];
		my $my_v = ActivePerl::perl_version;
		next if @$v && !grep $my_v =~ /^\Q$_\E\b/, @$v;
		push(@repos, "$id :: $repo->{Notes}");
	    }
	    $rsugbox->set("Select from list ...");
	    $rsugbox->configure(-values => [@repos], -state => "readonly");
	    my $rsugbox_cmd = sub {
		return unless keys(%PPM::Repositories::Repositories);
		return unless $sug_repo_var;
		$name_var = $sug_repo_var;
		$name_var =~ s/ ::.*$//;
		$uri_var = $PPM::Repositories::Repositories{$name_var}->{location};
	    };
	    Tkx::bind($rsugbox, "<<ComboboxSelected>>", $rsugbox_cmd);
	}
    }
    Tkx::grid($rnamel, $rnamee, "-", "-",
	-sticky => 'ew',
	-padx => 1,
	-pady => 1
    );
    Tkx::grid($rlocnl, $rlocne, "-", $dir_btn,
	-sticky => 'ew',
	-padx => 1,
	-pady => 1,
    );
    Tkx::grid($rsuglbl, $rsugbox, $save_btn, "-",
	-sticky => 'ew',
	-padx => 1,
	-pady => 1,
    );
    Tkx::grid(configure => $save_btn, -padx => [20, 1]);
    Tkx::grid(columnconfigure => $addf, 1, -weight => 1, -minsize => 20);

    Tkx::grid($sw, -sticky => 'news');
    Tkx::grid($addl, -sticky => 'news', -pady => [4, 0]);
    Tkx::grid(columnconfigure => $f, 0, -weight => 1);
    Tkx::grid(rowconfigure => $f, 0, -weight => 1);

    repo_sync(0); # refresh without sync/clear
    area_sync();

    $prefs_dialog->display();
    Tkx::focus(-force => $prefs_dialog);
}

sub on_load {
    # Restore state from saved information
    # We would need to make sure these are reflected in UI elements
    #$FILTER{'filter'} = $ppm->config_get("gui.filter") || "";
    $FILTER{'fields'} = $ppm->config_get("gui.filter.fields")
	|| "name abstract";
    $FILTER{'type'} = $ppm->config_get("gui.filter.type") || "installed";
    $INSTALL_AREA = $ppm->config_get("gui.install_area")
	|| $ppm->default_install_area;

    my @view_keys = keys %VIEW;
    my @view_vals = $ppm->config_get(map "gui.view.$_", @view_keys);
    while (@view_keys) {
	my $k = shift @view_keys;
	my $v = shift @view_vals;
	$VIEW{$k} = $v if defined $v;
    }
}

sub on_exit {
    # We should save dialog and other state information

    ## Window location and size
    $ppm->config_save("gui.geometry", $mw->g_wm_geometry);

    ## Current filter
    $ppm->config_save(
        "gui.filter" => $FILTER{'lastfilter'},
        "gui.filter.fields" => $FILTER{'lastfields'},
        "gui.filter.type" => ($FILTER{'lasttype'} eq 'modified' ? "all" : $FILTER{'lasttype'}),
    );

    ## Current selected package?

    ## Current install area
    $ppm->config_save("gui.install_area", $INSTALL_AREA || "");

    ## Tree column order, widths, visibility, sort

    # this gets columns in current order (visible and not)
    my @cols = $pkglist->column('list');
    for my $col (@cols) {
	#my $width = $pkglist->column('width', $col);
    }

    $ppm->config_save(map { ("gui.view.$_" => $VIEW{$_}) } keys %VIEW);

    $mw->g_destroy;
}

sub about {
    require ActivePerl::PPM;
    require ActivePerl;
    my $activeperl = ActivePerl::PRODUCT();
    my $perl_version = ActivePerl::perl_version;

    my $msg = "PPM version $ActivePerl::PPM::VERSION
$activeperl version $perl_version
\xA9 2013 ActiveState Software Inc.";

    if (my $be_serial = ActivePerl::PPM::Web::web_ua()->be_serial) {
	$msg .= "\n\nBusiness Edition $be_serial";
	my $state = $ppm->be_state;
	$msg .= " ($state)" if $state ne "valid";
    };

    Tkx::tk___messageBox(
        -title => "About Perl Package Manager",
	-icon => "info",
        -type => "ok",
	-message => $msg,
    );
}

sub status_error {
    my $context = shift;
    my $err;
    if (@_) {
	$err = shift;
	ppm_log("ERR", $context ? "$context: $err" : $err);
    }
    elsif ($@) {
        $err = $@;
	ppm_log("ERR", $context ? "$context: $err" : $err);
	$err = clean_err($err);
    }
    if ($context) {
	status_message("\nERROR $context:\n\t$err\n", "abstract");
    }
    else {
	status_message("\nERROR: $err\n", "abstract");
    }
}

sub status_message {
    if ($pw_nb && $status_box) {
	$pw_nb->select($status_sw);
	$status_box->configure(-state => "normal");
	$status_box->insert("end", @_);
	$status_box->configure(-state => "disabled");
	$status_box->see("end");
	Tkx::update('idletasks');
    }
    else {
	push(@pending_status_message, [@_]);
    }
    if ($ENV{'ACTIVEPERL_PPM_DEBUG'}) {
	print $_[0];
    }
}

BEGIN {
    package ActivePerl::PPM::GUI::Status;

    require ActivePerl::PPM::Status;
    our @ISA = qw(ActivePerl::PPM::Status);

    my $prefixed;

    sub begin {
	my $self = shift;
	my $what = shift;
	my $depth = $self->depth;
	ActivePerl::PPM::GUI::status_message("\n") if $prefixed;
	my $indent = "  " x $depth;
	ActivePerl::PPM::GUI::status_message("$indent$what ... ", ($depth == 0 ? "h2" : ""));
	$prefixed = 1;
	$self->SUPER::begin($what, @_);
    }

    sub tick {
	my $self = shift;
        if (@_) {
	    # update the progressbar
	    $progress = shift;
	    if (defined($progressbar) && !Tkx::winfo_ismapped($progressbar)) {
		# This works as progressbar should be last element
		$statusbar->add($progressbar, -weight => 1);
	    }
        }

	Tkx::update('idletasks');
	if ($ENV{'ACTIVEPERL_PPM_DEBUG'}) {
	    print "#";
	}
    }

    sub end {
	my $self = shift;
	my $outcome = shift || "done";
	my $what = $self->SUPER::end;
	my $depth = $self->depth;
	unless ($prefixed) {
	    my $indent = "  " x $depth;
	    $outcome = "$indent$what $outcome" unless $prefixed;
	}
	ActivePerl::PPM::GUI::status_message("$outcome\n", ($depth == 0 ? "h2" : ""));
	$prefixed = 0;
	$progress = -1;
	if (defined($progressbar) && Tkx::winfo_ismapped($progressbar)) {
	    $statusbar->remove($progressbar);
	}
    }
}
