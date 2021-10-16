package ActivePerl;

sub perl_version {
    return sprintf("%vd.%s", $^V, BUILD());
}

1;

__END__

=head1 NAME

ActivePerl - ActiveState's quality-assured binary build of Perl

=head1 SYNOPSIS

  use ActivePerl;
  print "$ActivePerl::VERSION\n";

=head1 DESCRIPTION

ActivePerl is ActiveState's quality-assured binary build of Perl,
available for AIX, HP-UX, Linux, Mac OS X, Solaris and
Windows. ActivePerl includes:

=over

=item *

The binary core distribution of perl.  See L<perl> for details.

=item *

A set of useful additional CPAN modules "out of
the box".

=item *

The Perl Package Manager, for quick and easy install of additional
Perl extension modules.  See L<ppm> for details.

=item *

Complete documentation in HTML format.  This is found in the F<html>
sub directory where ActivePerl was installed.

=back

The release notes for this version of ActivePerl is available in
L<activeperl-release>.  A list of changes beween builds of ActivePerl
is found in L<activeperl-changes>.

=head2 The ActivePerl:: module

The C<ActivePerl> module provide version information about ActivePerl.
The module is built in so most of its variables and functions are
available without having to C<require> the module. The module was
introduced in ActivePerl 813.1.

The following functions are available:

=over

=item ActivePerl->VERSION

=item ActivePerl->VERSION( $VERSION )

This method returns the current version of ActivePerl.  It is a number
like C<1202.01> or C<1202>.  With argument croak if the current version
is less than the given number.

This method is inherited from Perl's C<UNIVERSAL> class, and will
usually be invoked indirectly by C<use>, for instance:

    use ActivePerl 1200;

This statement ensures that the scripts runs under ActivePerl 1200 or
better.  If this perl is not ActivePerl or is older than 1200 then this
statement would croak.

The version number returned is picked up from the $ActivePerl::VERSION
variable.  For conditional code it is usually better to test against
this variable directly:

   if (($ActivePerl::VERSION || 0) >= 1200) {
       ...
   }

The C<|| 0> ensures that this code does not produce a warning if
running on ActivePerl 813 or older, or running on standard perl.

=item ActivePerl::PRODUCT

This constant function returns "ActivePerl" for the free version and
"ActivePerl Enterprise" for the Enterprise product. Earlier ActivePerl
Enterprise versions up to builds 638.8 and 817.4 returned "ActivePerlEE".

=item ActivePerl::BUILD

The value returned is the same as found in $ActivePerl::VERSION, but
the subversion number will not be padded with 0.  It means that this
value is suitable for printing, but unsafe for numeric comparisons.

This returns the same value as Win32::BuildNumber(), but this function
is only avalable on the Windows builds of ActivePerl.

=item ActivePerl::perl_version

Returns a full version number that also include the version of perl
this ActivePerl release is based on.  It returns a string like
"5.12.0.1202" or "5.12.0.1202.1".

This function is not directly built in and will only be avalable after
'require ActivePerl' has executed.

=back

=head1 SEE ALSO

L<activeperl-release>, L<activeperl-changes>, L<perl>

=cut
