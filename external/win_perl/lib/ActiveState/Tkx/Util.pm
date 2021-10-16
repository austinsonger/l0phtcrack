package ActiveState::Tkx::Util;

use strict;
use base 'Exporter';
our @EXPORT_OK = qw(focus_event selection_event mydie set_application_icon add_help accel_bind);

use ActiveState::OSType qw(IS_WIN32 IS_DARWIN);
use File::Spec::Functions qw(catfile);

sub focus_event {
    my $event = shift;
    my $focus_w = Tkx::focus(-displayof => ".");
    return unless $focus_w;
    Tkx::event_generate($focus_w, $event);
}


sub selection_event {
    my $event = shift;
    my $sel_w = Tkx::selection_own(-displayof => ".");
    return unless $sel_w;
    Tkx::event_generate($sel_w, $event);
}


sub mydie {
    my $msg = shift;
    chomp($msg);

    our $mw;
    require ActiveState::Tkx;
    ActiveState::Tkx->import('$mw');

    my $progname = $0;
    $progname =~ s,.*[\\/],,;
    $progname =~ s/\.pl$//;

    if (IS_DARWIN) {
	# We need to force visibility on OS X - this is a bug in Tk/Aqua,
	# but this is a fine work-around
	Tkx::wm_deiconify($mw);
	Tkx::update();
    }
    $mw->messageBox(-title => "$progname aborting",
		    -icon => "error",
		    -message => $msg);
    $mw->g_destroy;

    die "$progname: $msg\n";
}

sub accel_bind {
    # Accelerator / Binding mixed with cross-platform handling
    my ($w_bind, $evt, $cmd) = @_;
    my $acc;

    if ($evt =~ /^<<.*>>$/) {
	# virtual event
	my @binding = Tkx::SplitList(Tkx::event_info($evt));
	# Bindings for F-keys not found on the standard PC keyboard
	# are of no use to us.
	@binding = grep !/Key-F(\d+)/ || $1 <= 12, @binding;
	$acc = $binding[0];
    }

    $acc = $evt;
    $acc =~ s/^<// && $acc =~ s/>$//;
    $acc =~ s/^Key-//;
    $acc =~ s/Shift-/Shift+/;
    if (IS_DARWIN) { # should check [tk windowingsystem] eq "aqua"
	$acc =~ s/Mod1-/Command-/;
	$acc =~ s/Control-/Command-/;
    } else {
	$acc =~ s/Control-/Ctrl+/;
    }
    $acc =~ s/Meta-/Alt+/;
    $acc =~ s/([-\+][a-z])$/\U$1/;

    if (IS_DARWIN) { # should check [tk windowingsystem] eq "aqua"
	$evt =~ s/Control-/Command-/;
    }
    $w_bind->g_bind($evt, $cmd);
    return (-accelerator => $acc, -command => $cmd);
}

sub set_application_icon {
    my %opt = @_;
    my $mw = $opt{mw} || ".";

    if (IS_WIN32) {
	my $exe = $opt{exe} || $^X;
	# Due to a bug in Tk, we need to call the iconbitmap setting on
	# the main window an extra time (some Tk dll init issue).
	Tkx::wm_iconbitmap($mw, Tkx::wm_iconbitmap($mw));
	Tkx::wm_iconbitmap($mw, "-default", $exe);
	return;
    }

    if (IS_DARWIN) {
	return;
    }

    # X11
    if ((my $base = $opt{basename}) && $PerlApp::VERSION) {
	my @xbm = glob(catfile($PerlApp::RUNLIB, "icons/*/apps/$base.xbm"));
	if (@xbm) {
	    Tkx::wm_iconbitmap($mw, '@' . $xbm[-1]);
	}
	my @png = glob(catfile($PerlApp::RUNLIB, "icons/*/apps/$base.png"));
	if (@png) {
	    splice(@png, 1, @png - 2) if @png > 2;  # more than 2 icons confuse the KDE wm
	    @png = reverse @png;                    # and it needs higest resolution first
	    Tkx::package_require("img::png");
	    my @photos = map Tkx::image_create_photo(-file => $_, -format => "png"), @png;
	    Tkx::wm_iconphoto($mw, "-default", @photos);
	    Tkx::image_delete(@photos);
	}
    }
}

sub add_help {
    my($menu, %opt) = @_;

    my $contents = delete $opt{contents};
    # XXX should get the toplevel over the menu by default, but assume that
    # XXX the caller will pass in the right parent.  If not, you may end up
    # XXX hanging if '.' is not visible on the screen.
    my $parent = delete $opt{-parent} || ".";
    my $program_title = $opt{program_title} || die;

    my $help = $menu->new_menu(-tearoff => 0);
    $menu->add_cascade(
        -label => "Help",
	-underline => 0,
	-menu => $help,
    );

    my $top = $menu->_nclass->new($parent);

    my $pending_sep;
    require ActiveState::Browser;
    if ($contents && ActiveState::Browser::can_open($contents)) {
	$help->add_command(
	    -label => "Contents",
	    -accelerator => "F1",
	    -command => [\&ActiveState::Browser::open, $contents],
	);
	$top->g_bind("<Key-F1>", [\&ActiveState::Browser::open, $contents]);
	$pending_sep++;
    }

    if (ActiveState::Browser::can_open("index.html")) {
	$help->add_command(
	    -label => "ActivePerl User Guide",
	    -command => [\&ActiveState::Browser::open, "index.html"],
	);
	$pending_sep++;
    }

    if (ActiveState::Browser::can_open("http://www.activestate.com")) {
	$help->add_separator if $pending_sep;
	$pending_sep++;

	my $web = $help->new_menu(-tearoff => 0);
	$help->add_cascade(
	    -label => "Web Resources",
	    -menu => $web,
        );

        $web->add_command(
            -label => "Report Bug",
            -command => [\&ActiveState::Browser::open,
			 "http://bugs.activestate.com/enter_bug.cgi?product=ActivePerl"],
        );
        $web->add_command(
            -label => "ActivePerl Home",
            -command => [\&ActiveState::Browser::open,
			 "http://www.activestate.com/activeperl"],
        );
        $web->add_command(
            -label => "ActiveState Home",
            -command => [\&ActiveState::Browser::open, "http://www.activestate.com"],
        );
    }

    my $about_menu = $help;
    if (Tkx::tk("windowingsystem") eq "aqua") {
	# Ensure this is the first menu for OS X Cocoa to do the special magic
	$about_menu = $menu->new_menu(-tearoff => 0, -name => "apple");
	$menu->insert(0, 'cascade',
	    -underline => 0,
	    -menu => $about_menu,
	);
	$menu->add_separator;
    }
    else {
	$help->add_separator if $pending_sep;
    }

    $about_menu->add_command(
        -label => "About $program_title",
	-command => sub {
	    Tkx::tk___messageBox(
		-title => "About $program_title",
		-icon => "info",
		-type => "ok",
		-message => "$program_title $::VERSION\n\xA9 2011 ActiveState Software Inc.",
	    );
	 },
    );
}


1;
