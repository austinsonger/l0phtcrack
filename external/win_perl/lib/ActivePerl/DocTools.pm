package ActivePerl::DocTools;

use strict;
use warnings;

use Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(UpdateHTML UpdateHTML_blib);
our $VERSION = '2.04';

sub WriteRDF {
    require ActivePerl::DocTools::TOC::RDF;
    print ActivePerl::DocTools::TOC::RDF->new->TOC();
}

sub WriteTOC {
    require ActivePerl::DocTools::TOC;
    my $dir = $ActivePerl::DocTools::TOC::dirbase;
    return unless -d $dir;

    my($file,$toc);
    if (0) {
	require ActivePerl::DocTools::TOC::tocTab;
	$toc = ActivePerl::DocTools::TOC::tocTab->new->TOC();
	$file = "$dir/tocTab.js";
    }
    else {
	require ActivePerl::DocTools::TOC::HTML;
	$toc = ActivePerl::DocTools::TOC::HTML->new->TOC();
	$file = "$dir/perltoc.html";
    }

    unlink($file);
    my $fh;
    unless (open($fh, '>', $file)) {
	warn "Can't open '$file': $!";
	return;
    }
    print $fh $toc;
    close($fh) or die "Can't write '$file': $!";
    return 1;
}

sub UpdateHTML {
    unshift(@_, "raise_error") if @_ == 1;
    my %args = @_;

    require ActivePerl::DocTools::TOC;
    return unless -d $ActivePerl::DocTools::TOC::dirbase;

    require ActivePerl::DocTools::Tree::HTML;
    eval {
	ActivePerl::DocTools::Tree::HTML::Update(
	    verbose => $args{verbose},
	    force => $args{force},
        );
	WriteTOC();
    };
    if ($@ && $args{raise_error}) {
	if ($args{raise_error} eq 'wait') {
	    # this is somewhat bletcherous
	    print "Error building documentation: $@\n";
	    print "Press [Enter] to continue\n";
	    <STDIN>;
	    exit 1;
	}
	die $@;
    }
}

sub UpdateHTML_blib {
    unshift(@_, "verbose") if @_ == 1;
    my %args = @_;

    require ActivePerl::DocTools::Tree::HTML;
    eval {
	ActivePerl::DocTools::Tree::HTML::Update_blib(%args);
    };
    if ($@ && $args{verbose}) {
	die $@;
    }
}

# legacy
push(@EXPORT, "Pod2HTML");
*Pod2HTML = \&UpdateHTML_blib;

1;
