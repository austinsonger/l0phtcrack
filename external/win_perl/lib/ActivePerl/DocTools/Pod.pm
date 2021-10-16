package ActivePerl::DocTools::Pod;

use strict;
use base qw(Exporter);
our @EXPORT_OK = qw(pod2html pod2html_remove_cache_files);

use Carp qw(croak carp);
use Config qw(%Config);
use ActiveState::Path qw(realpath);

require Pod::Html;

my($basename, %expand); # XXX
my $scineplex = eval { require ActiveState::Scineplex };

sub pod2html {
    my(%opt) = @_;
    my $infile = delete $opt{infile} || croak("Required infile argument missing");
    my $outfile = delete $opt{outfile} || croak("Required outfile argument missing");
    my $depth = delete $opt{depth} || 0;
    my $podroot = realpath(delete $opt{podroot} || $Config{prefix});
    my $podpath = delete $opt{podpath} || "bin:lib";
    my $index = delete $opt{index} || 0;

    if (%opt && $^W) {
	carp("Unrecognized option $_ passed to pod2html")
	    for sort keys %opt;
    }

    chmod(0644, $outfile);
    unlink($outfile);

    my $html_root = substr("../" x $depth || "./", 0, -1);
    $index = $index ? "index" : "noindex";
    $podpath = join(":", map { s/:/|/g; $_ } @$podpath) if ref($podpath);

    Pod::Html::pod2html("--quiet",
	     "--$index",
	     "--htmlroot=$html_root",
	     "--podroot=$podroot",
	     "--podpath=$podpath",
	     "--infile=$infile",
	     "--outfile=$outfile",
	     "--css=${html_root}/Active.css",
	 );

    open (HTMLFILE, "<$outfile") or die "Couldn't open $outfile: $!";
    open (TMPFILE, ">$outfile.tmp") or die "Couldn't open $outfile.tmp: $!";
    my $first_header = 1;
    my $title;
    while (my $content = <HTMLFILE> ) {
	# Despite what Pod::Html says, this is not XHTML.
	# IE6 doesn't display things correctly with the wrong DOCTYPE.
	$content =~ s#^<!DOCTYPE .*?>#<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">#i;
	$content =~ s#<html xmlns="http://www.w3.org/1999/xhtml">#<html>#i;

	if ($content =~ s/^(\s*)<HEAD>\s*//i) {
	    print TMPFILE <<EOT;
$1<head>
$1<!-- saved from url=(0017)http://localhost/ -->
$1<script language="JavaScript" src="$html_root/displayToc.js"></script>
$1<script language="JavaScript" src="$html_root/tocParas.js"></script>
$1<script language="JavaScript" src="$html_root/tocTab.js"></script>
EOT
	    if ($scineplex) {
	        print TMPFILE qq($1<link rel="stylesheet" type="text/css" href="$html_root/scineplex.css">\n);
	    }
	}

	# Join split TITLE lines
	if ($content =~ /<TITLE>/i) {
	    until ($content =~ /<\/TITLE>/i) {
		chomp $content;
		$content .= " " . <HTMLFILE>;
	    }
	}

	if ($content =~ /<TITLE>(.*?)<\/TITLE>/i) {
	    $title = $1;
	}

	if ($index eq "index" && $content =~ /^<p><a name="__index__"><\/a><\/p>$/i) {
	    if ($title) {
		$content = <<EOT;
<script>writelinks('__top__',$depth);</script>
<h1><a>$title</a></h1>
$content
EOT
	    }
	    else {
		warn "DocTools: $outfile has no TITLE\n";
	    }
	}

	# Don't duplicate the title if we don't have an index.
	# Instead put the TOC buttons on the first header in the document.
	# This is being used for release notes and changelogs.
	if ($first_header && $index eq "noindex" && $content =~ /^<H\d>/i) {
	    $first_header = 0;
		$content = <<EOT;
<script>writelinks('__top__',$depth);</script>
$content
EOT
	}

	if (1) {
	    # XXX Skip the rest of the PDK doc tweaking for now...
	    print TMPFILE $content;
	    next;
	}

	if ($content =~ /^(.*<H1><A NAME=".*?_top">).*?(<\/A><\/H1>.*)$/i) {
	    # XXX die unless $title; ???
	    # substitute "NAME" with actual title
	    $content = "$1$title$2";
	}
	elsif ($content =~ /^(.*)<H(\d)><A NAME="(.*)">(.*)<\/A><\/H\2>(.*)$/i) {
	    my($prefix,$level,$name,$header,$suffix) = ($1,$2,$3,$4,$5);
	    if ($first_header && $header eq $title) {
		# skip redundant header
		$content = "$prefix<!-- $header -->$suffix";
	    }
	    else {
		# push all other heading one level down
		++$level;
		my $toc = "";
		my $expand = defined $expand{$basename} ? $expand{$basename} : 3;
		if (2 <= $level && $level <= $expand) {
		    $toc = "<script>writelinks('$name');</script>\n";
		}
		$content = "$prefix$toc<h$level><a name=\"$name\">$header</a></h$level>$suffix\n";
	    }
	    $first_header = undef;
	}
	elsif ($content =~ m,^(.*)<A HREF="(http://.*?)">(.*?)</A>(.*)$,i) {
	    # Add open external links in new browser and add xlink image after link text
	    my($prefix,$href,$text,$suffix) = ($1,$2,$3,$4);
	    # XXX treat .activestate.com URL differently?
	    $text .= qq( <img src="images/xlink.gif" border="0">) unless $href eq $text;
	    $content = qq($prefix<a target="_blank" href="$href">$text</a>$suffix\n);
	}

	print TMPFILE $content;
    }
    close (TMPFILE) || die "Couldn't write all of $outfile.tmp: $!";
    close (HTMLFILE);
    unlink($outfile); # bug 61127
    rename("$outfile.tmp", $outfile) || die "Couldn't rename $outfile.tmp back to $outfile: $!";
}

sub pod2html_remove_cache_files {
    # 5.6 uses ".x~~" and 5.8 uses ".tmp" extensions
    unlink("pod2htmd.$_", "pod2htmi.$_") for qw(x~~ tmp);
}

1;

__END__

=head1 NAME

ActivePerl::DocTools::Pod - Functions to process POD for ActivePerl

=head1 DESCRIPTION

The following functions are provided:

=over

=item pod2html( %args )

Convert a POD document into an HTML document.  This is a wrapper for
the pod2html() function of C<Pod::Html> that also modify the document
produced with various ActivePerl enhancements.

The following arguments are recognized:

=over

=item infile => $filename

The name of the POD file you want to convert.  This argument is mandatory.

=item outfile => $filename

The name of the HTML file you want as output.  This argument is mandatory.

=item depth => $int

How many directory levels down from the root of the HTML tree will the
generated file eventually be installed.  The root of the HTML tree is
normally found at $Config{installhtmldir}, which is normally
"$Config{prefix}/html".

The default is 0.

=item podroot => $dirname

Specify the base directory for finding library pods.
The default is $Config{prefix}.

=item podpath => [$dir1, $dir2,...]

What subdirectories of the C<podroot> should be searched for POD files
in order to discover targets for links from the generated HTML file.
The specified directories must all exist.

The links are generated with the assumption that the discovered POD
files are converted into HTML files with and F<.html> extension and
placed into an hierarchy (the HTML tree rooted at
$Config{installhtmldir}) using the same layout as the one found under
C<podroot>.

Instead of passing an array reference, the directories can
alternatively be specified as a single string of directory names
separated by C<:>.

The default is C<< [qw(bin lib)] >>.

=item index => $bool

Should a table on contents be created at the start of the HTML
document.  By default no table of contents is generated.

=back

=item pod2html_remove_cache_files( )

The pod2html() will create cache files with names starting with
F<pod2htm> in the current directory.  These cache files allow pod2html
to save link state between runs.  Call this function to clean up these
cache files.

=back

=head1 SEE ALSO

L<Pod::Html>

=head1 BUGS

none.
