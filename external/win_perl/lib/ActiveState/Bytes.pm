package ActiveState::Bytes;

use strict;
use base 'Exporter';

our @EXPORT_OK = qw(bytes_format bytes_parse);

sub bytes_format {
    my $n = shift;

    return sprintf "%.3g TB", $n / (1024 * 1024 * 1024 * 1024)
	if $n >= 1024 * 1024 * 1024 * 1024;

    return sprintf "%.3g GB", $n / (1024 * 1024 * 1024)
	if $n >= 1024 * 1024 * 1024;

    return sprintf "%.3g MB", $n / (1024 * 1024)
	if $n >= 1024 * 1024;
    
    return sprintf "%.3g KB", $n / 1024
	if $n >= 1024;

    return "$n bytes";
}

my %units = (
    K => 2**10,
    M => 2**20,
    G => 2**30,
    T => 2**40,
    P => 2**50,
    E => 2**60,
    Z => 2**70,
    Y => 2**80,
    k => 10**3,
    m => 10**6,
    g => 10**9,
    t => 10**12,
    p => 10**15,
    e => 10**18,
    z => 10**21,
    y => 10**24,
);

sub bytes_parse {
    my ($neg, $base, $unit) = ($_[0] =~ /([-+]?)([\d_,.]+)\s*([KMGTPEZY]?)/i);
    $base =~ s/[_,]//g;
    $base *= -1 if $neg eq '-';
    $base *= $units{$unit} if $unit;
    return $base;
}

1;

__END__

=head1 NAME

ActiveState::Bytes - Format byte quantities

=head1 SYNOPSIS

 use ActiveState::Bytes qw(bytes_format bytes_parse);
 print "The file is ", bytes_format(-s $file), " long.\n";

 print "1 megabyte is ", bytes_parse('1m'), " bytes.\n";

=head1 DESCRIPTION

The C<ActiveState::Bytes> module provides functions for dealing with human
readable byte strings.

=over

=item $str = bytes_format( $n )

This formats the number of bytes given as argument as a string using
suffixes like "KB", "GB", "TB" to make it concise.  The return value
is a string like one of these:

   128 bytes
   1.5 KB
   130 MB

Precision might be lost and there is currently no way to influence how
precise the result should be.  The current implementation gives no
more than 3 digits of precision.

=item $bytes = bytes_parse( $str )

Converts a bytes string ( "1m" ) into actually bytes. Lowercase units are in
powers of 10, uppercase are in powers of 2. For example, 1K is 1024 bytes, 1k
is 1000 bytes. Supports units are k, m, g, t, p, e, z, and y in ascending
order.

=back

=head1 COPYRIGHT

Copyright (C) 2003 ActiveState Corp.  All rights reserved.

=head1 SEE ALSO

L<ActiveState::Duration>

=cut
