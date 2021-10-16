package ActivePerl::PPM::SudoPath;

# When running under sudo you might want to make sure that created files belong
# to the user that invoked sudo instead of root.  If you register paths you are
# about to create by creating objects of this class you can chown them back to
# the original user.  Only files that did not exist originally are affected.
#
# Race conditions are not handled.  The paths sampled when objects are created
# might not be the same when chown is invoked.
#
# By default chown will not do anything unless we are running under sudo.

use ActiveState::Path qw(abs_path);
use File::Basename qw(basename dirname);

sub new {
    my $class = shift;
    my $self = bless {
	new => [],
    }, $class;
    my @paths = @_;

    for my $path (@paths) {
	$path = abs_path($path);
	next if -e $path;

	while (1) {
	    # This loop will terminate when $dir becomes the root
	    my $dir = dirname($path);
	    if (-d $dir) {
		push(@{$self->{new}}, $path);
		last;
	    }
	    $path = $dir;
	}
    }

    return $self;
}

sub chown {
    my($self, $uid, $gid) = @_;
    $uid ||= $ENV{SUDO_UID} || return;
    $gid ||= $ENV{SUDO_GID} || -1;

    for my $path (@{$self->{new}}) {
	_chown($uid, $gid, $path);
    }
}

sub _chown {
    my($uid, $gid, $path) = @_;
    return unless -e $path;
    warn "chown $uid $gid $path\n";
    CORE::chown($uid, $gid, $path);
    if (-d _) {
	if (opendir(my $dh, $path)) {
	    my @files = sort(grep !/^\.\.?$/, readdir($dh));
	    closedir($dh);
	    _chown($uid, $gid, "$path/$_") for @files;
	}
    }
}

1;
