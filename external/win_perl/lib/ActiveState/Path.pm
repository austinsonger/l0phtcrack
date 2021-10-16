package ActiveState::Path;

use strict;

our $VERSION = '1.01';

use Exporter ();
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(path_list find_prog is_abs_path abs_path join_path rel_path unsymlinked realpath);

use constant IS_WIN32 => $^O eq "MSWin32";
use File::Basename qw(dirname basename);
use Cwd ();
use Carp ();

my $ABS_PATH_RE = IS_WIN32 ? qr,^(?:[a-zA-Z]:)?[\\/], : qr,^/,;
my $PATH_SEP_RE = IS_WIN32 ? qr,[\\/], : qr,/,;
my $PATH_SEP    = IS_WIN32 ? "\\" : "/";

sub path_list {
    require Config;
    my @list = split /\Q$Config::Config{path_sep}/o, $ENV{PATH}, -1;
    if (IS_WIN32) {
        s/"//g for @list;
        @list = grep length, @list;
        unshift(@list, ".");
    }
    else {
        for (@list) {
            $_ = "." unless length;
        }
    }
    return @list;
}

sub find_prog {
    my $name = shift;
    return _find_prog($name) if $name =~ $PATH_SEP_RE;

    # try to locate it in the PATH
    for my $dir (path_list()) {
        if (defined(my $file = _find_prog(_join_path($dir, $name)))) {
	    return $file;
	}
    }
    return undef;
}

sub _find_prog {
    my $file = shift;
    return $file if -x $file && -f _;
    if (IS_WIN32) {
	for my $ext (qw(bat exe com cmd)) {
	    return "$file.$ext" if -f "$file.$ext";
	}
    }
    return undef;
}

sub is_abs_path {
    my $path = shift;
    return $path =~ $ABS_PATH_RE;
}

sub abs_path {
    my $path = shift;
    return ($path =~ $ABS_PATH_RE) ? $path : _join_path(_cwd(), $path)
}

sub _cwd {
    if (IS_WIN32) {
        my $cwd = Cwd::cwd();
	$cwd =~ s,/,\\,g;
	return $cwd;
    }
    else {
	return Cwd::cwd();
    }
}

sub join_path {
    my($base, $path) = @_;
    return ($path =~ $ABS_PATH_RE) ? $path : _join_path($base, $path);
}

sub _join_path {
    my($base, $path) = @_; # $path assumed to be relative
    while ($path =~ s,^(\.\.?)(:?$PATH_SEP_RE|\z),,o) {
	$base = dirname(unsymlinked($base)) if $1 eq "..";
    }
    if (length($path)) {
	return $path if $base eq ".";
	$base .= $PATH_SEP if $base !~ m,$PATH_SEP_RE\z,o;
    }
    $base .= $path;
    return $base;
}

sub rel_path {
    my($base, $path, $depth) = @_;

    # try the short way out
    $base .= $PATH_SEP if $base !~ m,$PATH_SEP_RE\z,o;
    if (substr($path, 0, length($base)) eq $base) {
	$path = substr($path, length($base));
	$path = "." unless length($path);
	return $path;
    }

    # the hard way
    $_ = abs_path($_) for $base, $path;

    my @base = split($PATH_SEP_RE, $base);
    my @path = split($PATH_SEP_RE, $path, -1);

    while (@base && @path && $base[0] eq $path[0]) {
        shift(@base);
        shift(@path);
    }

    my $up = @base;

    if (!IS_WIN32) {
	$base =~ s,$PATH_SEP_RE\z,,o;  # otherwise the -l test might fail
	my @base_rest;
	while (@base) {
	    if (-l $base) {
		my $rel_path = eval {
		    $base = _unsymlinked($base);
		    $depth ||= 0;
		    Carp::croak("rel_path depth limit exceeded") if $depth > 10;
		    return rel_path(_join_path($base, join($PATH_SEP, @base_rest)), $path, $depth + 1);
		};
		return $@ ? $path : $rel_path;
	    }
	    unshift(@base_rest, pop(@base));
	    $base = dirname($base);
	}
    }

    unshift(@path, ".") if !$up && (!@path || (@path == 1 && $path[0] eq ""));
    $path = join($PATH_SEP, ("..") x $up, @path);
    return $path;
}

sub unsymlinked {
    my $path = shift;
    $path = _unsymlinked($path) if !IS_WIN32 && -l $path;
    return $path;
}

sub realpath {
    my $path = shift;
    if (IS_WIN32) {
        Carp::croak("The path '$path' is not valid") unless -e $path;
        return scalar(Win32::GetFullPathName($path));
    }

    lstat($path);  # prime tests on '_'

    Carp::croak("The path '$path' is not valid") unless -e _;
    return Cwd::realpath($path) if -d _;

    $path = _unsymlinked($path) if -l _;
    return _join_path(Cwd::realpath(dirname($path)), basename($path));
}

sub _unsymlinked {
    my $path = shift;  # assumed to be a link
    my $orig_path = $path;
    my %seen;
    my $count;
    while (1) {
	Carp::croak("symlink cycle for $orig_path") if $seen{$path}++;
	Carp::croak("symlink resolve limit exceeded") if ++$count > 10;
	my $link = readlink($path);
	die "readlink failed: $!" unless defined $link;
	$path = join_path(dirname($path), $link);
	last unless -l $path;
    }
    Carp::croak("Dangling symlink for $orig_path") unless -e _;
    return $path;
}

1;

__END__

=head1 NAME

ActiveState::Path - Collection of small utility functions

=head1 SYNOPSIS

  use ActiveState::Path qw(find_prog);
  my $ls = find_prog("ls");

=head1 DESCRIPTION

This module provides a collection of small utility functions dealing
with file paths.

The following functions are provided:

=over 4

=item abs_path( $path )

Returns an absolute pathname denoting the same file as $path.  If
$path is already absolute it is returned unchanged.  The $path does
not have to reference an existing file or directory.

This functions differs from realpath() by only removing "." or ".."
segments at the beginning of $path and by only resolving the
symlinks needed to process the ".." segments correctly.  The
realpath() function also requires the file at $path to exist.

=item find_prog( $name )

=item find_prog( $path )

This function returns the full path to the given program if it can be
located on the system.  Otherwise C<undef> is returned.

The returned path might be relative.

=item is_abs_path( $path )

Returns TRUE if $path is an absolute file name.  This function works
the same as File::Spec method file_name_is_absolute().

=item join_path( $base, $path )

Returns a path that will reference the same file as $path does when
the current directory is $base.  If $path is absolute then it is
returned unchanged.

The $base should reference an existing directory.  The $path does not
have to refence an existing file or directory.

Any leading "." and ".." segments are removed from the $path before
the paths are joined.  In order to process ".." segments join_path()
will need to resolve symlinks in $base, and the function might croak
if this involves a symlink cycle.

=item path_list()

Returns the list of directories that will be searched to find
programs.  The path_list() is derived from the PATH environment
variable.  In scalar context this returns the number of paths to be
searched.

On Unix when the PATH environment variable is not present then this
function returns an empty list, but most shells still default to a
path list like (F</usr/bin>, F</bin>).

=item realpath( $path )

Returns the canonicalized absolute pathname of the path passed in.
All symbolic links are expanded and references to F</./>, F</../> and
extra F</> characters are resolved.  The $path passed in must be an
existing file.  The function will croak if not, or if the symbolic
links can't be expanded.

This differs from the Cwd::realpath() function in that $path does
not have to be a directory.

=item rel_path( $base, $path )

Return a relative pathname that denotes the same file as $path when
$base is the current directory.

The $base should reference an existing directory.  The $path does not
have to refence an existing file or directory.

This function differs from the File::Spec method abs2rel() in that it
make sure that any ".." segments in the returned value is correct when
the corresponding $base segments are symlinks.  If the $base path
contains symlink cycles there might not be any relative path that can
be produced, and in that case rel_path() falls back to returning
abs_path($path).

=item unsymlinked( $path )

If $path denotes a symlink expand it, otherwise return $path
unchanged.  The $path must reference an existing file.  This function
differs from realpath() by only expanding the symlink at the last
segment of $path.

On systems that don't implement symlinks this function is a noop,
always returning $path unchanged.

This function will croak if it's not possible to expand the symlink
because of cycles.

=back

=head1 BUGS

none.

=cut
