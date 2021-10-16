package ActiveState::Color;

use strict;
use base 'Exporter';
our @EXPORT_OK = qw(rgb_from_name name_from_rgb hex_from_rgb hsv_from_rgb rgb_from_hsv);

use Carp;

my %hex_from_name = (
    black    => '#000000',
    blue     => '#0000ff',
    cyan     => '#00ffff',
    green    => '#00ff00',
    magenta  => '#ff00ff',
    red      => '#ff0000',
    yellow   => '#ffff00',
    white    => '#ffffff',
);

my %name_from_hex = reverse %hex_from_name;


sub rgb_from_name {
    my $c = lc(shift);
    $c = $hex_from_name{$c} if exists $hex_from_name{$c};
    croak("Bad rgb value $c")
	unless $c =~ /^\#?([0-9a-f]+)/ && (length($1) % 3) == 0;
    croak("Need to be called in array context")
	 unless wantarray;
    my $len = length($1) / 3;
    my @t = map hex(substr($1, $_*$len, $len)) / ((1 << 4*$len) - 1), 0 .. 2;
    return @t;
}

sub hex_from_rgb {
    die unless @_ == 3;
    my @rgb = @_;
    for (@rgb) {
	$_ *= 256;
	$_ = 0 if $_ < 0;
	$_ = 255 if $_ > 255;
    }
    return sprintf "#%02x%02x%02x", @rgb;
}

sub name_from_rgb {
    my $hex = hex_from_rgb(@_);
    return $name_from_hex{$hex} || $hex;
}

sub hsv_from_rgb {
    croak("Must be called with 3 argument and in array context")
	unless @_ == 3 && wantarray;
    my ($r, $g, $b)= @_;

    my $min = _min($r, $g, $b);
    my $max = _max($r, $g, $b);

    my $v = $max;
    my $delta = $max - $min;

    my $s;
    if ($delta) {
	$s = $delta / $max;
    }
    else {
	return 0, 0, $v;
    }

    my $h;
    if ($r == $max) {
	$h = ($g - $b) / $delta;
    }
    elsif ($g == $max) {
	$h = 2 + ($b - $r) / $delta;
    }
    else { # $b == $max
	$h = 4 + ($r - $g) / $delta;
    }

    $h *= 60;
    $h += 360 if $h < 0;

    return $h, $s, $v;
}

sub rgb_from_hsv {
    croak("Must be called with 3 argument and in array context")
	unless @_ == 3 && wantarray;
    my($h, $s, $v)= @_;

    return $v, $v, $v if $s == 0;

    $h /= 60;
    my $i = int($h);
    my $f = $h - $i;
    my $p = $v * ( 1 - $s );
    my $q = $v * ( 1 - $s * $f );
    my $t = $v * ( 1 - $s * ( 1 - $f ) );

    if ($i == 0) {
	return $v, $t, $p;
    }
    elsif ($i == 1) {
	return $q, $v, $p;
    }
    elsif ($i == 2) {
	return $p, $v, $t;
    }
    elsif ($i == 3) {
	return $p, $q, $v;
    }
    elsif ($i == 4) {
	return $t, $p, $v;
    }
    else { # $i == 5
	return $v, $p, $q;
    }
}

sub _min {
    my $min = shift;
    while (@_) {
	my $n = shift;
	$min = $n if $n < $min;
    }
    return $min;
}

sub _max {
    my $max = shift;
    while (@_) {
	my $n = shift;
	$max = $n if $n > $max;
    }
    return $max;
}

1;

=head1 NAME

ActiveState::Color - Collection of color conversion functions

=head1 SYNOPSIS

 use ActiveState::Color qw(name_from_rgb rgb_from_name
                           rgb_from_hsv hsv_from_rgb
                          );
 my($h, $s, $v) = hsv_from_rgb(rgb_from_name(shift));
 # make the color fully saturated and a bit lighter
 $s = 1; $v *= 1.2;
 print name_from_rgb(rgb_from_hsv($h, $s, $v)), "\n";

=head1 DESCRIPTION

The following functions are provided:

=over 4

=item ($r, $g, $b) = rgb_from_name( $name )

This will convert a color name or a hex RGB-tripplet to a decimal RGB
value with $r, $g, $b in the range 0.0 to 1.0.  The hex tripplet can
have any precision and can optionally be prefixed with "#".  If the
name is not recognized this function will croak.  Examples of valid
names are:

    #F0F
    #FF00FF
    #FFF000FFF
    ff00ff
    black
    BLACK
    yellow

=item $hexname = hex_from_rgb( $r, $g, $b )

This converts a decimal RGB value with $r, $g, $b in the range 0.0 to
1.0 to an 8-bit hex RGB-tripplet.  The output will be on the form:

    #ff00ff

=item $name = name_from_rgb( $r, $g, $b )

This will convert a decimal RGB value to a color name.  If the color
is one of the 8 primary RGB colors then the name will be returned,
otherwise a hex RGB-tripplet is returned.  The 8 primary color names
are:

    black
    blue
    cyan
    green
    magenta
    red
    yellow
    white

=item ($r, $g, $b) = rgb_from_hsv( $h, $s, $v )

=item ($h, $s, $v) = rgb_from_hsv( $r, $g, $b )

These functions convert between the RGB and HSV color space.  The
range of $s, $v, $r, $g, and $b is 0.0 to 1.0.  The range of $h is 0.0
to 360.0.

=back

=head1 BUGS

none.

=cut
