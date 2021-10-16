package ActiveState::Duration;

our $VERSION = "1.00";

use strict;
use base 'Exporter';

our @EXPORT_OK = qw(dur_format_sm dur_format_iso dur_format_eng ago_eng dur_format_clock dur_parse);

my @unit = qw(second minute hour day week);
my @unit_f = (60,    60,    24,  7);

my @factor;
push(@factor, 1);
for (@unit_f) {
    push(@factor, $factor[-1] * $_);
}

sub _dur_breakup {
    my($dur, $prec, $frac_part) = @_;
    $prec = 0.05 unless defined($prec);
    $frac_part = "second" unless defined($frac_part);

    my $neg = $dur < 0;
    $dur = abs($dur);

    # special case
    return [[0, $unit[0]]], $neg unless $dur;

    my $d = $dur;
    my $res = 0;
    my @res;

 UNIT:
    for my $i (reverse 0 .. @factor - 1) {
	my $u = $d / $factor[$i];

        my @div = (1);
        unshift(@div, 30, 10, 5) if ($unit_f[$i] || 0) == 60;
        for my $div (@div) {
            my $ui = int($u / $div + 0.5) * $div;
            if (abs($res + $ui * $factor[$i] - $dur) / $dur <= $prec) {
                # close enough
                push(@res, [$ui => $unit[$i]]);
                last UNIT;
            }
        }

	my $ui = int $u;
	if ($frac_part eq $unit[$i] || ($ui && $frac_part eq "first")) {
	    my $uf;
	    for my $dec (1 .. 6) {
		$uf = sprintf "%.*f", $dec, $u;
		last if abs($res + $uf * $factor[$i] - $dur) / $dur <= $prec;
	    }
	    push(@res, [$uf => $unit[$i]]);
	    last UNIT;
	}
	elsif ($ui) {
	    push(@res, [$ui => $unit[$i]]);
	    $res += $ui * $factor[$i];
	    $d -= $ui * $factor[$i];
	}
    }

    return \@res, $neg
}

sub dur_format_sm {
    my($res, $neg) = _dur_breakup(@_);
    $res = join("", map { $_->[0] . substr($_->[1], 0, 1) } @$res);
    $res = "-$res" if $neg;
    $res;
}

sub dur_format_iso {
    my($res, $neg) = _dur_breakup(@_);
    $res = "P" . join("", map { $_->[0] . uc(substr($_->[1], 0, 1)) } @$res);
    $res =~ s/(\d+[HMS])/T$1/;
    $res = "-$res" if $neg; # XXX not really any standard negative notation
    $res;
}

sub dur_format_eng {
    my($res, $neg) = _dur_breakup(@_);
    my @a;
    for (@$res) {
	my $t = $_->[0] . " " . $_->[1];
	$t .= "s" if $_->[0] ne "1";
	push(@a, $t);
    }

    $res = pop(@a);
    $res = join(" and ", join(", ", @a), $res) if @a;
    $res = "negative $res" if $neg;
    $res;
}

sub ago_eng {
    my $res = dur_format_eng(@_);
    if ($res =~ s/^negative //) {
	$res .= " from now"
    }
    else {
	$res .= " ago";
	$res = "just now" if $res eq "0 seconds ago";
    }
    $res;
}

sub dur_format_clock {
    my $d = shift;

    my $neg = $d < 0;
    $d = abs($d);

    my $s = $d % 60;
    my $m = int($d / 60) % 60;
    my $h = int($d / 3600);
    sprintf "%s%d:%02d:%02d", ($neg ? "-" : ""), $h, $m, $s;
}

sub dur_parse {
    my $str = shift;
    return undef unless defined $str;

    $str = lc($str);
    $str =~ s/\s+//g;
    return 0 if $str eq "justnow" || $str eq "0";

    my $sign = 1;
    $sign = -1 if $str =~ s/^-// || $str =~ s/^negative// || $str =~ s/fromnow$//;

    if ($str =~ /^(\d+):(\d\d):(\d\d)$/) {
	# clock format
	return $sign * ($1 * 60*60 + $2 * 60 + $3);
    }

    # cleanup
    $str =~ s/^p// && $str =~ s/t//;  # ISO stuff
    $str =~ s/ago$//;

    my @v = split(/([a-z]+)/, $str);
    return undef unless @v;

    my $dur = 0;
    while (@v) {
	my($n, $unit) = splice(@v, 0, 2);
	return undef unless $n =~ /^\d+(\.\d+)?$/;
	return undef unless defined $unit;

	$unit =~ s/and$//;
	$unit =~ s/s$// if length $unit > 1;
	return undef unless length $unit;

	my $factor;
	for my $i (0 .. @unit - 1) {
	    next unless substr($unit[$i], 0, length($unit)) eq $unit;
	    $factor = $factor[$i];
	    last;
	}

	return undef unless $factor;
	$dur += $n * $factor;
    }

    return $sign * $dur;
}

1;

__END__

=head1 NAME

ActiveState::Duration - Format and parse time duration values

=head1 SYNOPSIS

 use ActiveState::Duration qw(ago_eng);
 print "It is now ", ago_eng(time), " since time begun.\n";

=head1 DESCRIPTION

The C<ActiveState::Duration> module provides functions to format and
parse time duration values.  Time is expressed as a number of weeks,
days, hours, minutes and seconds.

=over

=item $str = dur_format_XXX( $duration )

=item $str = dur_format_XXX( $duration, $precision )

=item $str = dur_format_XXX( $duration, $precision, $frac_part )

The dur_format_XXX() functions take a time $duration value (in
seconds) and format it as a readable string.  This section describes
the common arguments supported by most of dur_format_XXX() functions.
The C<XXX> in the function name selects which format it returns and is
described below.

The $precision argument specify how close the string much match the
duration value.  The default is 0.05 which means that up to 5% off is
acceptable.  Pass 0 as the $precision to request an exact result.

The $frac_part specify at which time unit formatting will start using
fractions to achieve the desired precision.  It should be one of
"week", "day", "hour", "minute", "second" or "first".  The default is
"second".

A $frac_part value of "first" is special and needs some more
explanation.  The time units will be considered in order from "week"
to "second", and the first one longer than the $duration is used.

=item $str = dur_format_sm( @ARGS )

Use the Sendmail format which looks like "1h30m".  If the $duration
value is negative it is formatted like "-1h30m".

=item $str = dur_format_iso( @ARGS )

Use the ISO 8601 format which looks like "PT1H30M".  If the $duration
value is negative it is formatter with a leading "-", even though that
is not standards conforming.

=item $str = dur_format_eng( @ARGS )

Use plain English; "1 hour and 30 minutes".  If the $duration value is
negative then the word "negative" is prepended to the string.

=item $str = ago_eng( @ARGS )

Same as dur_format_eng() but adds the word "ago" as long as $duration
is positive and the words "from now" if $duration is negative. A
$duration of 0 is special cased as "just now".

=item $str = dur_format_clock( $duration )

Use stopwatch format; "1:30:00", i.e. "hhh:mm:ss".  This function does
not take the $precision or $frac_part arguments yet and the $duration
is truncated to whole seconds.  A negative $duration value will be
formated with a leading "-".  A value of C<0> is formatted as "0:00:00".

=item $duration = dur_parse( $str )

This function takes a string representing a duration value and return
it as number of seconds.  All strings produced by any of the
dur_format_XXX() or the ago_eng() functions can be parsed back to an
$duration value by dur_parse().  It returns C<undef> if it cannot
parse the $str for some reason.

=back

=head1 COPYRIGHT

Copyright (C) 2003 ActiveState Corp.  All rights reserved.

=head1 SEE ALSO

L<Time::Duration>, L<ActiveState::StopWatch>, L<ActiveState::Bytes>

=cut
