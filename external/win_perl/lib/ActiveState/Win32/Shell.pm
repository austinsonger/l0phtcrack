package ActiveState::Win32::Shell;

use 5.006;
use strict;
use warnings;

require DynaLoader;
require Exporter;

our @ISA = qw(DynaLoader Exporter);
our @EXPORT_OK = qw(FindExecutable BrowseDocument BrowseUrl);

our $VERSION = '0.01';

bootstrap ActiveState::Win32::Shell $VERSION;

sub BrowseDocument {
    my($document,$fragment) = @_;
    my $executable = FindExecutable($document);
    return unless $executable;
    $document = "file:///$document";
    $document .= "#$fragment" if $fragment;
    $document =~ s/ /%20/g;
    $document =~ s,\\,/,g;
    return _ShellExecute($executable, qq("$document")) == 0;
}

sub BrowseUrl {
    my $url = shift;
    return _ShellExecute($url, undef) == 0;
}

1;
__END__

=head1 NAME

ActiveState::Win32::Shell - Windows Shell Functions

=head1 SYNOPSIS

  use ActiveState::Win32::Shell qw(BrowseDocument);

  my $document = 'C:\Program Files\ActiveState\Document.html';
  my $fragment = 'anchor_name';

  BrowseDocument($document, $fragment)
    or die "Can't browse document";

=head1 DESCRIPTION

The ActiveState::Win32::Shell module provides access to the
windows shell (Explorer).

The following methods are provided:

=over

=item BrowseDocument( $document, $fragment )

Opens $document in default browser and navigates to $fragment.
Returns true if the operation was successful.

=item BrowseUrl( $url )

Opens $url in default browser.  Returns true if the operation was
successful.

=item FindExecutable( $document )

Returns the executable registered to open the $document or undef.

=back

=head1 COPYRIGHT

Copyright 2006 ActiveState Software Inc.

=cut
