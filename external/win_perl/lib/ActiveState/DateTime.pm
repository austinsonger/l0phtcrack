package ActiveState::DateTime;

use strict;
use base 'Exporter';
use Time::Local;
our @EXPORT_OK = qw(is_leap_year days_in_month check_date month_name_short month_name_long gmt_offset);
    
my %months_short = (
              1 => 'Jan',
              2 => 'Feb',
              3 => 'Mar',
              4 => 'Apr',
              5 => 'May',
              6 => 'Jun',
              7 => 'Jul',
              8 => 'Aug',
              9 => 'Sep',
              10 => 'Oct',
              11 => 'Nov',
              12 => 'Dec',
              );

my %months_long = (
              1 => 'January',
              2 => 'February',
              3 => 'March',
              4 => 'April',
              5 => 'May',
              6 => 'June',
              7 => 'July',
              8 => 'August',
              9 => 'September',
              10 => 'October',
              11 => 'November',
              12 => 'December',
              );

sub is_leap_year {
    my $year = shift;

    my $leap = $year >= 1582
             ? ((!($year % 4) && ($year % 100)) || !($year % 400))
             : !($year % 4);
}

sub days_in_month {
    my $year = shift;
    my $month = shift;

    my @days_in_month = (0, 31, 28, 31, 30, 31,
                         30, 31, 31, 30, 31, 30, 31);
    return undef if $year < -4713 or $month < 1 or $month > 12;
    my $days = $days_in_month[$month];
    ++$days if $month == 2 and is_leap_year($year);
    return $days;
}

sub check_date {
    my $year = shift;
    my $month = shift;
    my $day = shift;

    return undef if $year < -4713 or $month < 1 or $month > 12;
    return $day <= days_in_month($year, $month) ? 1 : 0;
}

sub month_name_short {
    my $month = shift;

    return undef if $month < 1 or $month > 12;
    return $months_short{$month};
}

sub month_name_long {
    my $month = shift;

    return undef if $month < 1 or $month > 12;
    return $months_long{$month};
}

sub gmt_offset {
    my ($sec, $min, $hour, $mday, $mon, $year) = localtime();
    my $localtime = timelocal($sec, $min, $hour, $mday, $mon, $year);
    my $gmtime = timegm($sec, $min, $hour, $mday, $mon, $year);
    my $offset = $gmtime - $localtime;
    my $hours = int($offset / 3600);
    my $remainder = $offset % 3600;
    my $minutes = int($remainder / 60);
    my $gmt_offset;
    # Take into account the zero offset from GMT
    if (($hours == 0) && ($minutes == 0)) {
       	$gmt_offset = '+0000';
    }
    else {
        $gmt_offset = sprintf("%.2d%.2d", $hours, $minutes);
    }
    return $gmt_offset;
}

1;

=head1 NAME

ActiveState::DateTime - Date and Time utilities

=head1 SYNOPSIS

 use ActiveState::DateTime qw(is_leap_year days_in_month check_date month_name_short month_name_long gmt_offset);

 if (is_leap_year($year)) {
    ... do stuff ...
 }

 my $max_days = days_in_month($year, $month);

 if (check_date($year, $month, $day)) {
    ... do stuff ...
 }

 my $short_month_name = month_name_short($month);

 my $long_month_name = month_name_long($month);

 my $offset = gmt_offset();

=head1 DESCRIPTION

The ActiveState::DateTime module provide functions that can be used
for convenient data and time checks.

=over

=item is_leap_year($year)

This function takes a numerical argument of a year.  It returns true if
the year is a leap year, otherwise it returns false.

=item $days = days_in_month($year, $month)

This function returns the number of days in a month and takes into
account leap years.  The year is a numerical argument.  The month is
a numerical argument from 1 to 12.

=item check_date($year, $month, $day)

This function takes numerical arguments for year, month and day and
checks to make sure the date is valid.  For example:

Is February 29, 2005 a valid date?

Is February 29, 2008 a valid date?

The month is a numerical argument that ranges from 1 to 12.

=item $month_name = month_name_short($month);

This function returns the English three letter abbreviation of the month
name.

=item $month_name = month_name_long($month);

This function returns the full English month name.

=item $gmt_offset = gmt_offset();

This function returns the GMT offset.

=back

No functions are exported by default.

=head1 COPYRIGHT

Copyright (C) 2003 ActiveState Corp.  All rights reserved.

=cut
