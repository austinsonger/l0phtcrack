package ActiveState::DiskUsage;

use strict;
use base 'Exporter';
our @EXPORT_OK = qw(du);

our %seen;
our $blocksize = 512;
$blocksize = 1024 if $^O eq "hpux";

sub du {
    local %seen;
    return _du(@_);
}

sub _du
{
    my $f = shift;
    my $total = 0;

    my @s = lstat($f);
    return 0 unless @s;
    return 0 if $^O ne "MSWin32" && -l _;

    if (-d _) {
	$total += $^O eq "MSWin32" ? 0 : $s[12] * $blocksize;
	if (opendir(my $d, $f)) {
	    my @files = readdir($d);
	    closedir($d);
	    for (@files) {
		next if $_ eq "." || $_ eq "..";
		$total += _du("$f/$_");
	    }
	}
	return $total;
    }

    # plain file, don't count hard linked files multiple times
    return 0 if $s[1] && $seen{"$s[0]:$s[1]"}++;
    return $^O eq "MSWin32" ? $s[7] : $s[12] * $blocksize;
}

1;

=head1 NAME

ActiveState::DiskUsage - Calculate how much diskspace is used

=head1 SYNOPSIS

 use ActiveState::DiskUsage qw(du);
 print du(".") . " bytes allocated in current directory\n";

=head1 DESCRIPTION

This module provide a single function called du() that calculate the
diskusage for the given file or directory.  If the argument is a
directory then the diskspace used by all its files and subdirectories
are recursively counted as well. Symbolic links are not dereferenced.

The return value of the du() function is the number of bytes
allocated.

The du() function is not exported by default.

=head1 BUGS

On Windows, this function only sum up the size of the files found at
the given location.  This number will be lower than the actual
allocation as it does not take block allocation and space consumed by
the directories themselves into account.  The L<Win32::DirSize> module
might give more accurate results on this platform.

=head1 COPYRIGHT

Copyright (C) 2003 ActiveState Corp.  All rights reserved.

=head1 SEE ALSO

L<ActiveState::Bytes>

