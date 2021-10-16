package ActiveState::Browser;

our $VERSION = "1.03";

use strict;
use ActiveState::Handy qw(shell_quote);
use ActiveState::Path qw(find_prog abs_path join_path);
use ActiveState::Run qw(run);

use constant IS_WIN32 => ($^O eq "MSWin32");
use constant IS_DARWIN => ($^O eq "darwin");

our $BROWSER ||= $ENV{AS_BROWSER};
unless ($BROWSER) {
    if (IS_WIN32) {
	$BROWSER = "start %s";
    }
    elsif (IS_DARWIN) {
	$BROWSER = "/usr/bin/open %s";
    }
    else {
	my @try = qw(xdg-open);
	if ($ENV{BROWSER}) {
	    push(@try, split(/:/, $ENV{BROWSER}));
	}
	else {
	    push(@try, qw(firefox galeon mozilla opera netscape));
	}
	unshift(@try, "kfmclient") if $ENV{KDE_FULL_SESSION};
	unshift(@try, "gnome-open") if $ENV{GNOME_DESKTOP_SESSION_ID};
	for (@try) {
	    if (my $p = find_prog($_)) {
		$BROWSER = $p;
		if ($_ eq "kfmclient") {
		    $BROWSER = [$BROWSER, "openURL"];
		}
		elsif ($_ eq "gnome-open" || $_ eq "opera") {
		    # fine as it is
		}
		else {
		    $BROWSER = shell_quote($BROWSER) . " %s &";
		}
		last;
	    }
	}
    }
}

our $HTML_DIR;
$HTML_DIR ||= abs_path(".");

sub can_open {
    my $url = shift;
    return 0 unless $BROWSER;
    return 1 if $url =~ /^(\w+):/;
    return !!eval { _resolve_file_url($url) };
}

sub _resolve_file_url {
    my $url = shift;
    my $frag;
    $frag = $1 if $url =~ s/#(.*)//;
    $url = join_path($HTML_DIR, $url);
    die "Help file $url not found\n" unless -f $url;
    $url = Win32::GetShortPathName($url) if IS_WIN32;
    $url = (IS_WIN32 ? "file:///" : "file://") . $url;
    $url .= "#$frag" if defined $frag;
    return $url;
}

sub _browser_cmd {
    my($url, $browser) = @_;
    $browser ||= $BROWSER || die "No browser specified";
    my $cmd;
    if (ref($browser)) {
	$cmd = shell_quote(@$browser, $url);
    }
    elsif ($browser =~ /%/) {
	$cmd = $browser;
	$url = shell_quote($url);
	# substitute %s with url, and %% to %.
	$cmd =~ s/%([%s])/$1 eq '%' ? '%' : $url/eg;
    }
    else {
	$cmd = shell_quote($browser, $url);
    }
    #$cmd .= " 2>/dev/null 1>&2 " unless IS_WIN32;
    return $cmd;
}

sub open {
    my $url = shift;
    if (IS_WIN32 && eval { require ActiveState::Win32::Shell }) {
	my($document,$fragment) = $url =~ m,^(?:file:///?)?([^#]+)(?:#(.*))?$,;
	unless ($document =~ /^\w{2,}:/) {
	    $document = join_path($HTML_DIR, $document);
	    return if ActiveState::Win32::Shell::BrowseDocument($document, $fragment);
	}
	ActiveState::Win32::Shell::BrowseUrl($url);
	return;
    }

    $url = _resolve_file_url($url) unless $url =~ /^\w{2,}:/;
    die "Can't find any browser to use.  Try to set the AS_BROWSER environment variable.\n"
	unless $BROWSER;

    run(_browser_cmd($url, $BROWSER));
}

1;

__END__

=head1 NAME

ActiveState::Browser - Interface to invoke the web-browser

=head1 SYNOPSIS

  use ActiveState::Browser;
  ActiveState::Browser::open("http://www.activestate.com");

=head1 DESCRIPTION

The ActiveState::Browser module provides an interface to make a web browser
pop up showing some URL or file.  The following functions are
provided:

=over

=item open( $url )

This will try to open up a web browser displaying the given URL.  The
function will croak if the $url can't be resolved or if no suitable
browser could be found.  The can_open() test can be used to protect
against such failure, but note that such a test is not race-proof.

If the $url is absolute it is passed directly to the browser.

If the $url is relative it is looked up relative to the directory
C<$ActiveState::Browser::HTML_DIR>, which defaults to the current
directory.

=item can_open( $url )

Will return TRUE if we can invoke a browser for the given URL.  If the
URL is not to a local file, then this always returns TRUE, given that
a browser program was found.

=back

=head1 ENVIRONMENT

The AS_BROWSER environment variable can be set to override what
browser to use.  The string C<%s> will be replaced with the URL to
open.  If no C<%s> is present the string is taken as a command to invoke
with the URL as the only argument.

The C<%s> template was inspired by the BROWSER environment variable
suggestion that appear quite dead; see
L<http://www.catb.org/~esr/BROWSER/>.  Note that the AS_BROWSER is
B<not> a colon separated list.

=head1 BUGS

none.

=cut
