package ActiveState::Tkx;

use strict;
use base 'Exporter';
our @EXPORT_OK = qw($mw);

use ActiveState::OSType qw(IS_WIN32);

eval {
   require Tkx;
};
if ($@) {
    # clean up of message
    $@ =~ s/ at .* line \d+//g;
    $@ =~ s/^Tcl error '// && $@ =~ s/' while invoking (scalar|array) result.*//s;
    $@ =~ s/this isn't a Tk application//;
    $@ =~ s/Compilation failed in require\.\n\z//;
    my $msg = "Panic: Not able to initialize graphical interface.\n$@";

    if (IS_WIN32) {
	require Win32;
	Win32::MsgBox($msg, Win32::MB_ICONSTOP());
    }
    print STDERR "$msg\n";
    exit 1;
}

require ActiveState::Tkx::Widget;
our $mw = ActiveState::Tkx::Widget->new(".");
$mw->g_wm_withdraw;

1;
