package ActiveState::OSType;

our $VERSION = "1.00";

use base 'Exporter';
our @EXPORT = qw(IS_WIN32 IS_DARWIN IS_UNIX);

if ($^O eq "MSWin32") {
    eval <<EOT; die $@ if $@;
sub IS_WIN32  () { 1 }
sub IS_DARWIN () { 0 }
sub IS_UNIX   () { 0 }
EOT
}
elsif ($^O eq "darwin") {
    eval <<EOT; die $@ if $@;
sub IS_WIN32  () { 0 }
sub IS_DARWIN () { 1 }
sub IS_UNIX   () { 1 }
EOT
}
else {
    eval <<EOT; die $@ if $@;
sub IS_WIN32  () { 0 }
sub IS_DARWIN () { 0 }
sub IS_UNIX   () { 1 }
EOT
}

1;

__END__

=head1 NAME

ActiveState::OSType - Constants for platform specific conditional code

=head1 SYNOPSIS

  use ActiveState::OSType qw(IS_WIN32);

  if (IS_WIN32) {
     # do windows specific code
  }
  else {
     # do other code
  }

=head1 DESCRIPTION

This module exports the constants C<IS_WIN32>, C<IS_DARWIN> and
C<IS_UNIX>.  These constants can be used to wrap conditional code for
specific platforms.  Using these constants instead of testing $^O
directly allow perl to eliminate unused branches of the code at
compile time.  Thus

  if (IS_WIN32) {
      ...
  }

is exactly the same as

  if ($^O eq "MSWin32") {
      ...
  }

but the first form looks better and the whole block is eliminated at
compile time if not running on a windows machine.  The test:

  if (IS_UNIX) { ... }

is currently the same as

  if ($^O ne "MSWin32") { ... }

but this might change in the future if this module is made to support
systems other than Windows and Unix.  There is no simple $^O test that
can determine if the OS is Unix.

