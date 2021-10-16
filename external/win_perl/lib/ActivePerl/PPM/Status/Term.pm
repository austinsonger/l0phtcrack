package ActivePerl::PPM::Status::Term;

use strict;
require ActivePerl::PPM::Status;
our @ISA = qw(ActivePerl::PPM::Status);

use Time::HiRes qw(time);

my @ANIMATION = ("/", "-", "\\", "|");
my $INDENT = "  ";

my $animation_index = 0;
my $last_p;
my $last_t;
my $prefixed;

sub begin {
    my $self = shift;

    if (my $depth = $self->depth) {
	print "\n" if $prefixed;
	print $INDENT x $depth;
    }

    my $what = shift;
    print "$what...";

    $animation_index = 0;
    $last_p = "";
    $last_t = time;
    $prefixed = 1;

    return $self->SUPER::begin($what, @_);
}

sub tick {
    my $self = shift;
    $self->SUPER::tick(@_);
    return unless $prefixed;
    if (@_) {
	my $p = shift;
	$p = 1 if $p > 1;
	$p = sprintf "%3.0f%%", $p * 100;
	if ($last_p ne $p) {
	    my $t = time;
	    my $d = $t - $last_t;
	    if ($p eq "100%" || $d > 0.5) {
		print $p . ("\b" x length($p));
		$last_p = $p;
		$last_t = $t;
	    }
	}
    }
    else {
	my $t = time;
	my $d = $t - $last_t;
	if ($d > 0.1) {
	    my $c = $ANIMATION[$animation_index];
	    $animation_index = ($animation_index + 1) % @ANIMATION;
	    print $c . ("\b" x length($c));
	    $last_t = $t;
	}
    }
}

sub end {
    my $self = shift;
    my $outcome = shift || "done";
    my $what = $self->SUPER::end();
    print "     \b\b\b\b\b" if $prefixed && $last_t;
    if ($prefixed) {
	print "$outcome\n";
	$prefixed = 0;
    }
    else {
	print $INDENT x $self->depth;
	print "$what $outcome\n";
    }
}

1;
