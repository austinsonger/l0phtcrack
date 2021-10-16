package ActiveState::CPAN::Utils;

use strict;
use base 'Exporter';
our @EXPORT_OK = qw($PKG_EXT distname_info);

our $PKG_EXT = qr/\.(tar\.(?:bz2|gz)|tgz|zip)\z/;


# Reimplementation and simplification of http://search.cpan.org/dist/CPAN-DistnameInfo/

sub distname_info {
    my $path = shift;
    my @a = split(/[\/\\]/, $path);
    my $name = pop(@a);
    my $author;
    if (@a) {
	if ($a[0] eq "authors") {
	    $author = $a[4];
	}
	elsif (@a >= 3 &&
	       $a[0] =~ /^([A-Z])\z/ &&
	       $a[1] =~ /^($1[A-Z])\z/ &&
	       $a[2] =~ /^$1[A-Z]+\z/)
	{
	    $author = $a[2];
	}
	elsif ($a[0] =~ /^[A-Z]+\z/) {
	    $author = $a[0];
	}
    }
    my $extension = ($name =~ s/$PKG_EXT//) ? $1 : "";
    my $version = ($name =~ s/-([^-]+)$//) ? $1 : "";
    if (length $version) {
        if ($name =~ s/-(\d+(\.\d+)+(?:-cvs)?)$//) {
            # MITREHC/HoneyClient-Agent-0.98-stable
            # GRICHTER/HTTP-Webdav-0.1.18-0.17.1
            # PFEIFFER/makepp-1.50-cvs-080517
            $version = "$1-$version";
	    $version =~ s/-withoutworldwriteables$//;
        }
        elsif ($version =~ /^\d+$/ && $name =~ s/-(\d+(?:-\d+)*)$//) {
            # MIKO/String-Util-0-11
            $version = "$1-$version";
        }
        elsif ($name eq "BitArray1") {
            # GWORROLL/BitArray1-0.tar.gz
            $name = "BitArray";
            $version = "1-$version";
        }
        elsif ($name eq "Orac-alpha") {
            # ANDYDUNC/Orac-alpha-1.2.6
            $name = "Orac";
            $version = "alpha-$version";
        }
        elsif ($name =~ /^(perl-pdf)-(\d.*)$/) {
            # SREZIC/perl-pdf-0.06.1b-SREZIC-3
            $name = $1;
            $version = "$2-$version";
        }
        elsif ($version =~ /^(?:[A-Z][A-Za-z]|db$)/ && $version ne "LATEST" ||
               $version =~ /^[a-z]{2,}_/ ||  # DJPADZ/finance-yahooquote_0.19
              0)
        {
            # HAKANARDO/Math-Expr-LATEST.tar.gz :-(

            # undo version split
            $name .= "-$version";
            $version = "";
        }
    }
    unless (length $version) {
        # some exceptions
        if ($name =~ s/(?<=^subclust)(v1_0)$// || # SIMATIKA/subclustv1_0
            $name =~ s/(?<=^perl)-?(5\.00.*)$// || # TIMB/perl5.004_04
            $name =~ s/_(\d+(?:_\d+)+)$//      || # JWHITE/SlideMap_1_2_2
            $name =~ s/_([vV]?\d[^_]*)$//      || # SMANROSS/Win32-Exchange_v0.046a
            $name =~ s/\.?(v?\d+(?:.\d+)+)$//  || # GABOR/Text-Format0.52
                                                  # JHIVER/Unicode-Transliterate.0.3
            0)
        {
            $version = $1;
        }
    }
    $version =~ s/-withoutworldwriteables$//;
    my $maturity = ($version =~ /\d\D\d+_\d/) ? "developer" : "released";
    return ($name, $version, $maturity, $author, $extension, $path) if wantarray;
    return {
        name => $name,
        version => $version,
        maturity => $maturity,
        author => $author,
        extension => $extension,
        path => $path,
    };
}

1;
