package ActivePerl::PPM::Util;

use strict;
use base 'Exporter';

our @EXPORT_OK = qw(is_cpan_package clean_err join_with update_html_toc gunzip);

sub is_cpan_package {
    my $pkg_name = shift;
    return "" if $pkg_name =~ /^Active(State|Perl)-/;
    return "libwww-perl" if $pkg_name eq "LWP";
    return "TermReadKey" if $pkg_name eq "Term-ReadKey";
    return $pkg_name;  # assume everything else is
}

sub clean_err {
    my $err = shift;
    $err =~ s/ at .*//s unless $ENV{ACTIVEPERL_PPM_DEBUG};
    $err =~ s/ _at / at /g; # escape for when you really want "at" in the message
    $err =~ s/\n*\z//;
    return $err;
}

sub join_with {
    my $conjunc = shift;
    my $text = pop(@_);
    if (@_) {
	my $serial_comma = (@_ > 1) ? "," : "";
	$serial_comma = "" if @_ <= 5 && !grep /\s/, $text, @_;
	$text = join(", ", @_) . "$serial_comma $conjunc $text";
    }
    return $text;
}

sub update_html_toc {
    if (eval { require ActivePerl::DocTools; }) {
	eval { ActivePerl::DocTools::WriteTOC() };
	if ($@) {
	    require ActivePerl::PPM::Logger;
	    ActivePerl::PPM::Logger::ppm_log("ERR", $@);
	}
    }
}

sub gunzip {
    my $gzfile = shift;
    my $out = shift;
    unless ($out) {
	($out = $gzfile) =~ s/\.gz$// || die "gunzip need a .gz file to uncompress";
    }

    require Compress::Zlib;
    my $gz = Compress::Zlib::gzopen($gzfile, 'rb') || die "Can't open $gzfile: $Compress::Zlib::gzerrno";

    open(my $fh, ">", $out) || die "Can't create $out: $!";
    binmode($fh);

    my $buf;
    my $n;
    while (($n = $gz->gzread($buf)) > 0) {
	print $fh $buf;
    }
    if ($n == -1) {
	die "Can't uncompress $gzfile: $Compress::Zlib::gzerrno";
    }
    $gz->gzclose;
    close($fh) || die "Can't write $out: $!";

    return $out;
}

1;
