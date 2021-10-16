package ActiveState::StopWatch;

use strict;

BEGIN {
    eval {
	require Time::HiRes;
	Time::HiRes->import('time');
    };
}

use base 'Exporter';
our @EXPORT = qw(start_watch stop_watch);
our @EXPORT_OK = qw(real_time read_watch);

my @LABELS = qw(r u s cu cs);

sub start_watch {
    return [time, times] unless @_;

    my $w = shift;
    return unless @$w > @LABELS;  # not stopped
    my @t = (time, times);
    my @p = splice(@$w, @LABELS);

    for (@$w) {
	$_ += shift(@t) - shift(@p);
    }
}

sub read_watch {
    my @w = @{shift(@_)};

    my @t = @w > @LABELS ? @w[@LABELS .. $#w] : (time, times);
    for (@t) {
	$_ -= shift @w;
    }

    # drop child times if they are 0
    pop(@t) unless $t[-1];
    pop(@t) unless $t[-1];

    my @labels = @LABELS;
    for (@t) {
	if ($_ > 60 || ($labels[0] eq "r" && !$Time::HiRes::VERSION)) {
	    $_ = sprintf "%s=%.0fs", shift(@labels), $_;
	}
	elsif ($_ > 15) {
	    $_ = sprintf "%s=%.1fs", shift(@labels), $_;
	}
	elsif ($_ == 0) {
	    $_ = sprintf "%s=0", shift(@labels);
	}
	else {
	    $_ = sprintf "%s=%.2fs", shift(@labels), $_;
	}
    }

    return "@t";
}

sub stop_watch {
    my $w = shift;
    push(@$w, time, times) unless @$w > @LABELS;  # already stopped
    return read_watch($w) if defined wantarray;
}

sub real_time {
    my $w = shift;
    return time - $w->[0];
}

1;

=head1 NAME

ActiveState::StopWatch - Time code

=head1 SYNOPSIS

 use ActiveState::StopWatch qw(start_watch stop_watch read_watch real_time);

 my $w = start_watch();
 # ... do stuff ...
 print read_watch($w);
 # ... do stuff ...
 print stop_watch($w);

 # restart it
 start_watch($w);
 print stop_watch($w);

=head1 DESCRIPTION

The ActiveState::StopWatch module provide functions that can be used
to measure the time spent by sections of code.  The following
functions are provided:

=over

=item $w = start_watch()

=item start_watch( $w )

Without argument this function creates a new watch object.  With
argument it can restart a watch that has been stopped after invoking
the stop_watch() function on it.  If the watch provided is not stopped
then its state is not affected.

=item $str = read_watch( $w )

This function returns a string that tells how much real, user and
system time has ticked during the time the watch has been running.
The string will look like this:

  r=12.5s u=4.90s s=0.55s cu=1.22s cs=0.22s

The 'cu' and 'cs' field gives time spent in children of the current
process during the time the watch has been running.  The child fields
are not shown if they are 0.

=item $str = stop_watch( $w )

This function return a string of the same format as for read_watch(),
but it will also stop the clock from running.  This means that
read_watch() will continue to return the same result.  The watch can
be restarted by calling start_watch() with the watch object as
argument.

=item $r = real_time( $w )

This function returns the number of seconds of real time passed during
the time the watch has been running.

=back

Only the start_watch() and stop_watch() functions are exported by default.

=head1 COPYRIGHT

Copyright (C) 2003 ActiveState Corp.  All rights reserved.

=cut
