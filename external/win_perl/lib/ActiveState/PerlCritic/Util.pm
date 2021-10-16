package ActiveState::PerlCritic::Util;

use strict;
use base qw(Exporter);
our @EXPORT_OK = qw(get_image appdata_location);

use Carp qw(croak);
use File::Basename qw(dirname);
use ActiveState::OSType qw(IS_WIN32 IS_DARWIN);

my $CRITIC_DIR = dirname(__FILE__);

my @formats = (
   [xbm => "bitmap"],
   [xpm => "photo", "img::xpm"],
   [ico => "photo", "img::ico"],
   [gif => "photo",],
   [png => "photo", "img::png"],
   [ppm => "photo",],
   [pgm => "photo",],
);

my %img;

sub get_image {
    my($name, @image_args) = @_;
    return $img{$name} ||= do {
        my @img_spec;
        for (@formats) {
            my($ext, $type, $pkg) = @$_;
	    my $file = "$CRITIC_DIR/img/$name.$ext";
	    @img_spec = (-file => $file) if -f $file;
            if (@img_spec) {
                Tkx::package_require($pkg) if $pkg;
                if ($type ne 'bitmap') {
                    unshift(@img_spec, -format => $ext);
                }
                unshift(@img_spec, $type);
                last;
            }
        }

        croak("Can't locate image '$name'") unless @img_spec;
        Tkx::image_create(@img_spec, @image_args);
    }
}

sub appdata_location {
    if (IS_WIN32) {
	require Win32;
	my $appdata = Win32::GetFolderPath(Win32::CSIDL_APPDATA()) ||
	    $ENV{APPDATA} || $ENV{HOME};
	die "No valid setting for APPDATA\n" unless $appdata;
	$appdata = Win32::GetShortPathName($appdata) || $appdata;
	return "$appdata/ActiveState/PerlCritic";
    }
    elsif (IS_DARWIN) {
	return "$ENV{HOME}/Library/Application Support/ActiveState/PerlCritic";
    }
    else {
	return "$ENV{HOME}/.ActivePerl/PerlCritic";
    }
}

1;
