package ActiveState::Version;

use strict;

our $VERSION = '1.3';

use base 'Exporter';
our @EXPORT_OK = qw(vcmp vge vgt vle vlt veq vnorm vnumify);

=head1 NAME

ActiveState::Version - Utility functions for version comparison

=head1 SYNOPSIS

 use ActiveState::Version qw(vgt veq vcmp);

 my $x = "0.9.9_beta";
 my $y = "0.10";
 my $z = "0.1";

 print "$x is ", (vgt($x, y) ? "greater" : "less or equal"), "than $y.\n";
 print "$y and $z are ", (veq($y, $z) ? "" : " not "), "equal.\n";

 my @sorted = sort { vcmp($a, $b) } ($x, $y, $z);

 print "The newest version is $sorted[-1].\n";

=head1 DESCRIPTION

Handy utilities for uniform version comparison across various
ActiveState applications.

Provides C<vcmp>, C<vge>, C<vgt>, C<vle>, C<vlt>, C<veq>, all
of which perform comparisons equivalent to the similarly named
perl operators.

Also provides the C<vnumify> function which turns any version string
to a floating point number.  For version strings that are gibberish it
returns 0.

=cut

sub vge ($$) { return (vcmp(shift, shift) >= 0); }
sub vgt ($$) { return (vcmp(shift, shift) >  0); }
sub vle ($$) { return (vcmp(shift, shift) <= 0); }
sub vlt ($$) { return (vcmp(shift, shift) <  0); }
sub veq ($$) { return (vcmp(shift, shift) == 0); }

sub vcmp ($$) {
    my($v1, $v2) = @_;

    return undef unless defined($v1) && defined($v2);

    # can we compare the version numbers as floats
    # return $v1 <=> $v2 if $v1 =~ /^\d+\.\d+$/ && $v2 =~ /^\d+\.\d+$/;

    my @a = _vtuple($v1);
    my @b = _vtuple($v2);

    # { local $" = '.'; print "$v1=@a $v2=@b\n"; }
    while (@a || @b) {
        my $a = @a ? shift(@a) : 0;
        my $b = @b ? shift(@b) : 0;
        unless ($a =~ /^-?\d+$/ && $b =~ /^-?\d+$/) {
            next if $a eq $b;
            return undef;

        }
        if (my $cmp = $a <=> $b) {
            return $cmp;
        }
    }
    return 0;
}

sub _vtuple {
    my $v = shift;
    $v = "0" if !defined($v) || $v eq "";

    unless ($v =~ s/^v//) {
        if ($v =~ /^(\d+)\.(\d+)(_\d+)?\z/) {
            my $g;
            if (length($2) == 4) {
                # Turn 5.0102 into 5.1.2
                $g = 2;
            }
            elsif (length($2) % 3 == 0) {
                # Turn 5.010001 into 5.10.1
                $g = 3;
            }
            if ($g) {
                $v = join(".", $1, map substr($2, $_*$g, $g), 0 .. (length($2) / $g - 1));
                $v .= $3 if $3;
            }
        }
    }

    my @v = split(/[-_.]/, $v);

    # The /-r\d+/ suffix if used by PPM to denote local changes
    # and should always go into the 4th part of the version tuple.
    # As an extension, we will just strip the 'r' if the version
    # already has 4 or more parts.
    if ($v[-1] =~ /^r(\d+)$/) {
        pop @v;
        push @v, 0 while @v < 3;
        push @v, $1;
    }

    my $num;
    if ($v[-1] =~ s/([a-z])$//) {
        my $a = $1;
        if ($v[-1] eq "" || $v[-1] =~ /^\d+$/) {
            $num = ord($a) - ord('a') + 1;
        }
        else {
            $v[-1] .= $a;
        }
    }

    if (!defined($num) && $v[-1] =~ s/(a|alpha|b|beta|gamma|delta|p|patch|m|maint|pre|rc|RC|trial|TRIAL|mt)(\d*)$//) {
        my $kind;
        ($kind, $num) = (lc $1, $2);
        $num ||= 0;
        my $offset = {
            a => 400,
            alpha => 400,
            b => 300,
            beta => 300,
            gamma => 290,
            delta => 280,
	    p => 0,
	    patch => 0,
            m => 0,
            maint => 0,
            pre => 200,
            rc => 100,
            t => 100,
            trial => 100,
            mt => 100,
        };
	die unless defined $offset->{$kind};
	$num -= $offset->{$kind};
    }

    if (defined $num) {
        if (length($v[-1])) {
            push(@v, $num);
        }
        else {
            $v[-1] = $num;
        }
    }
    return @v;
}

sub _vnorm {
    my @v = @_;
    my $i = @v - 1;
    while ($i >= 0) {
        $v[$i] =~ s/^0+(\d)/$1/;
        $v[$i] = 0 unless $v[$i] =~ /^-?\d+\z/;
        if ($i && $v[$i] < 0) {
            $v[$i] = 1000 + $v[$i];
            $v[$i - 1]--;
        }
        $i--;
    }
    return @v;
}

sub vnorm {
    return "v" . join(".", _vnorm(_vtuple(@_)));
}

sub vnumify {
    my $v = shift;
    return 0 unless $v;
    if (UNIVERSAL::isa($v, "version")) {
        $v = $v->numify;
        $v =~ s/_//;  # why do they do this?
        #$v =~ s/(\.\d+?)0+\z/$1/;  # trim trailing zeros
        return $v;
    }
    return $v if $v =~ /^\d+(?:\.\d+)?\z/;
    return "$1$2" if $v =~ /^(\d+\.\d+)_(\d+)\z/;
    my @v = _vnorm(_vtuple($v));
    my $first = shift(@v);
    return $first unless @v;
    for (@v) {
        $_ = 999 if $_ > 999;
    }
    return join("", "$first.", map { sprintf "%03d", $_ } @v);
}

1;
