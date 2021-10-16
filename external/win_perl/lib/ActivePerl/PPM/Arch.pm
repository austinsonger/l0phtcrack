package ActivePerl::PPM::Arch;

use strict;
use base 'Exporter';

our @EXPORT_OK = qw(arch short_arch full_arch pretty_arch versioned_arch osname @archs);

use Config qw(%Config);

sub versioned_arch {
    my($arch,$version) = @_;
    return undef unless $arch;
    if ($version >= 5.008) {
	$arch .= sprintf "-%d.%d", int($version), int(($version-int($version))*1000 + 0.5);
    }
    return $arch;
}

sub arch {
    return versioned_arch($Config{archname}, $]);
}

sub short_arch {
    my $arch = shift || arch();
    1 while $arch =~ s/-(thread|multi|2level)//;
    return $arch;
}

sub full_arch {
    my $arch = shift;
    return $arch if $arch =~ /-thread\b/;
    my $perl = "";
    $perl = $1 if $arch =~ s/(-5\.\d\d?)$//;
    return "$arch-thread-multi-2level$perl" if $arch =~ /^darwin/;
    return "$arch-multi-thread$perl"        if $arch =~ /^MSWin/;
    return "$arch-thread-multi$perl";
}

sub osname {
    my $arch = shift || arch();
    for (qw(MSWin32 darwin aix linux solaris)) {
        return $_ if index($arch, $_) >= 0;
    }
    return "" if $arch eq "noarch";
    return "hpux";  # the odd one
}

sub pretty_arch {
    my $arch = shift || arch();
    1 while $arch =~ s/-(thread|multi|2level)//;
    my $perl = "5.6";
    $perl = $1 if $arch =~ s/-(5\.\d\d?)$//;
    if ($arch eq "darwin") {
        $arch = "Mac OS X";
    }
    elsif ($arch eq "aix") {
        $arch = "AIX";
    }
    elsif ($arch =~ /^MSWin32-x(86|64)$/) {
        $arch = "Windows";
        $arch .= " 64" if $1 eq "64";
    }
    elsif ($arch =~ /^(i686|x86_64|ia64)-linux$/) {
        my $cpu = $1;
        $cpu = "x86" if $cpu eq "i686";
        $cpu = "IA64" if $cpu eq "ia64";
	$arch = "Linux ($cpu)";
    }
    elsif ($arch =~ /^(x86|sun4)-solaris(-64)?$/) {
        $arch = "Solaris";
        $arch .= " 64" if $2;
	my $cpu = $1;
	$cpu = "SPARC" if $cpu eq "sun4";
        $arch .= " ($cpu)";
    }
    elsif ($arch =~ /^(IA64\.ARCHREV_0|PA-RISC\d+\.\d+)(-LP64)?$/) {
        $arch = "HP-UX";
        $arch .= " 64" if $2;
        my $cpu = $1;
        $cpu = "IA64" if $cpu =~ /^IA64/;
        $cpu =~ s/(?<=^PA-RISC)/ /;
        $arch .= " ($cpu)";
    }
    elsif ($arch eq "noarch") {
	return "ActivePerl" if $perl eq "5.6";
	return "ActivePerl $perl (and later)";
    }
    else {
        $arch = ucfirst($arch);  # lame
    }
    return "ActivePerl $perl on $arch";
}

our @archs = qw(
    MSWin32-x86
    MSWin32-x64
    darwin
    i686-linux
    x86_64-linux
    ia64-linux
    sun4-solaris
    sun4-solaris-64
    x86-solaris
    x86-solaris-64
    PA-RISC1.1
    PA-RISC2.0-LP64
    IA64.ARCHREV_0
    IA64.ARCHREV_0-LP64
    aix
);

1;

__END__

=head1 NAME

ActivePerl::PPM::Arch - Get current architecture identification

=head1 DESCRIPTION

The following functions are provided:

=over

=item arch()

Returns the string that PPM use to identify the architecture of the
current perl.  This is what goes into the NAME attribute of the
ARCHITECTURE element of the PPD files; see L<ActivePerl::PPM::PPD>.

This is L<$Config{archname}> with the perl major version number
appended.

=item short_arch()

=item short_arch( $arch )

This is the shorteded architecture string; dropping the segments for
features that will always be enabled for ActivePerl ("thread",
"multi", "2level").

Used to form the URL for the PPM CPAN repositories provided by
ActiveState.

=item full_arch( $short_arch )

Convert back from a short arch string to a full one.  If the passed arch
string is already full it's returned unchanged.

=item pretty_arch()

=item pretty_arch( $arch )

Returns a more human readable form of arch().  Will be a string on the form:

   "ActivePerl 5.10 for Windows 64"

=item versioned_arch( $arch, $version )

Returns $arch, potentially suffixed with a version if $version is at least 5.010.
Version 5.010 would be suffixed as "-5.10".  Returns undef if $arch is not defined.

=back

=head1 SEE ALSO

L<ppm>, L<ActivePerl::PPM::PPD>, L<Config>
