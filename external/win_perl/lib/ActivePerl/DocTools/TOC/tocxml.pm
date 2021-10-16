package ActivePerl::DocTools::TOC::tocxml;

use strict;
use warnings;

use base 'ActivePerl::DocTools::TOC';

use Config qw(%Config);

my($count,$depth,$max_depth);
my @index;

my $close_pending;

sub text {
    my $text =  join("\n", @_, "");
    return sub { $text };
}


# extra info is tedious to collect -- is done in a subclass or something.
sub extra { '' };

sub header { '' }

# pod
sub before_pods {
    return _page("", "Core Documentation") . library_indent_open();
}


sub pod_separator { '' } # XXX?

sub pod {
    my($self, $file) = @_;
    my $key = $^O eq "darwin" ? "pods::$file" : "Pod::$file";
    $key = "pods::$file" if $^O eq "MSWin32" && Win32::BuildNumber() > 820;
    return _page($self->{pods}->{$key}, $file, $self->extra($file));
}

sub after_pods { return library_indent_close() }

# scripts
sub before_scripts {
    return _page("", "Programs") . library_indent_open();
}

sub script {
    my($self, $file) = @_;
    return _page($self->{scripts}->{$file}, $file, $self->extra($file));
}

sub after_scripts { return library_indent_close() }

# pragmas
sub before_pragmas {
    return _page("", "Pragmas") . library_indent_open();
}

sub pragma {
    my($self, $file) = @_;
    return _page($self->{pragmas}->{$file}, $file, $self->extra($file));
}

sub after_pragmas { return library_indent_close() }

# libraries
sub before_libraries {
    return _page("", "Modules") . library_indent_open();
}

sub library_indent_open  {
    ++$depth;
    $max_depth = $depth if $depth > $max_depth;
    $index[$depth] = 0;
    if ($close_pending) {
	$close_pending = 0;
	return ">\n";
    }
    return '';
}

sub library_indent_close {
    --$depth;
    my $close = "";
    if ($close_pending) {
	$close_pending = 0;
	$close = "/>\n";
    }
    my $indent = "  " x $depth;
    return "$close$indent</node>\n";
}

sub library_indent_same  { '' }

sub library {
    my($self, $file, $showfile, $depth) = @_;
    return _page($self->{files}->{$file}, $showfile, $self->extra($file));
}

sub library_container {
    my($self, $file, $showfile, $depth) = @_;
    return _page("", $showfile, $self->extra($file));
#    return _folder($showfile);
}

sub after_libraries { return library_indent_close() }

sub footer {
    my $nCols = $max_depth + 1;
    return <<HERE;

</toc>
HERE
}

sub _folder {
    my($text) = @_;
    die "no argument to _folder!" unless defined $text;
    return qq'<img src="images/greysmallbullet.gif" width="5" height="5" alt="*"> $text<br>\n';
}

sub _page {
    my($href, $text, $extra) = @_;
    my $close = $close_pending ? "/>\n" : "";
    die "bad arguments to _page: ($href, $text, $extra)" unless defined $href and defined $text;
    $extra ||= '';
    $extra = " $extra" if $extra;  # just to make it EXACTLY identical to the old way.
    ++$count;
    ++$index[$depth];
    my $level = join(".", @index[0..$depth]);
    $close_pending = 1;
    my $indent = "  " x $depth;
    return qq($close$indent<node name="$text" link="$href");
}


sub boilerplate {
    return boiler_header() . boiler_links();
}

sub boiler_header {
    $count = -1;
    $depth = 0;
    $max_depth = 0;
    $index[$depth] = -1;
    my $product = defined &ActivePerl::PRODUCT ? ActivePerl::PRODUCT() : "ActivePerl";
    my $version = "$Config{PERL_REVISION}.$Config{PERL_VERSION}";
    return <<"HERE";
<?xml version="1.0" encoding="UTF-8"?>
<toc name="$product" version="$version">

HERE
}


sub boiler_links {
    my $text = _page("perlmain.html", "");
    $text .= _page("", "Getting Started");
    $text .= library_indent_open();
    $text .= _page("perlmain.html", "Welcome To ActivePerl");
    $text .= _page("release.html", "Release Notes");
    $text .= _page("install.html", "Installation Guide");
    $text .= _page("faq/ActivePerl-faq2.html", "Using PPM");
    $text .= _page("faq/Windows/ActivePerl-Winfaq6.html", "Web Server Configuration")
	if $^O eq "MSWin32";
    $text .= _page("readme.html", "Getting Started");
    $text .= _page("changes.html", "ActivePerl Change Log");
    $text .= _page("unsupported.html", "Unsupported Features")
	if -e "$ActivePerl::DocTools::TOC::dirbase/unsupported.html";
    $text .= _page("resources.html", "More Resources");
    $text .= _page("Copyright.html", "License and Copyright");
    $text .= library_indent_close();

    $text .= _page("", "ActivePerl Components");
    $text .= library_indent_open();

    $text .= _page("Components/Descriptions.html", "Overview");

    if ($^O eq "MSWin32") {
	my $site = "site/";
	$site = "" if -f "$ActivePerl::DocTools::TOC::dirbase/lib/Win32/OLE/Browser.html";

	$text .= _page("", "Windows Specific");
	$text .= library_indent_open();
	$text .= _page("${site}lib/Win32/OLE/Browser.html", "OLE Browser");
	$text .= _page("Components/Windows/PerlScript.html", "PerlScript");
	$text .= _page("../eg/IEExamples/index.htm", "PerlScript Examples");
	$text .= _page("Components/Windows/PerlISAPI.html", "Perl for ISAPI");
	$text .= PerlEx_links();
	$text .= _page("Components/Windows/PerlEz.html", "PerlEz");
	$text .= library_indent_close();
    }
    $text .= library_indent_close();

    $text .= _page("", "ActivePerl FAQ");
    $text .= library_indent_open();
    $text .= _page("faq/ActivePerl-faq.html", "Introduction");
    $text .= _page("faq/ActivePerl-faq1.html", "Availability &amp; Install");
    $text .= _page("faq/ActivePerl-faq3.html", "Docs &amp; Support");

    if ($^O eq "MSWin32") {
	$text .= _page("", "Windows Specific");
	$text .= library_indent_open();
	$text .= _page("faq/Windows/ActivePerl-Winfaq2.html", "Perl for ISAPI");
	$text .= _page("faq/Windows/ActivePerl-Winfaq4.html", "Windows 9X/Me/NT/200X/XP");
	$text .= _page("faq/Windows/ActivePerl-Winfaq5.html", "Windows Quirks");
	$text .= _page("faq/Windows/ActivePerl-Winfaq7.html", "Web Programming");
	$text .= _page("faq/Windows/ActivePerl-Winfaq8.html", "Windows Programming");
	$text .= _page("faq/Windows/ActivePerl-Winfaq9.html", "Modules &amp; Samples");
	$text .= _page("faq/Windows/ActivePerl-Winfaq10.html", "Embedding &amp; Extending");
	$text .= _page("faq/Windows/ActivePerl-Winfaq12.html", "Using OLE with Perl");
	$text .= library_indent_close();
    }
    $text .= library_indent_close();

    if ($^O eq "MSWin32") {
	$text .= _page("", "Windows Scripting");
	$text .= library_indent_open();
	$text .= _page("Windows/ActiveServerPages.html", "Active Server Pages");
	$text .= _page("Windows/WindowsScriptHost.html", "Windows Script Host");
	$text .= _page("Windows/WindowsScriptComponents.html", "Windows Script Components");
	$text .= library_indent_close();
    }

    return $text;
}

sub PerlEx_links {
    return "" unless -e "$ActivePerl::DocTools::TOC::dirbase/PerlEx/Welcome.html";

    my $text = _page("", "PerlEx");
    $text .= library_indent_open();

    $text .= _page("", "Getting Started");
    $text .= library_indent_open();
    $text .= _page("PerlEx/Welcome.html", "Welcome");
    $text .= _page("PerlEx/QuickStart.html", "Getting Started");
    $text .= _page("PerlEx/HowItWorks.html", "How PerlEx Works");
    $text .= library_indent_close();

    $text .= _page("", "Configuration");
    $text .= library_indent_open();
    $text .= _page("PerlEx/WebServerConfig.html", "WebServer Configuration");
    $text .= _page("PerlEx/IntrpClass.html", "PerlEx Interpreter Classes");
    $text .= _page("PerlEx/RegistryEntries.html", "PerlEx Registry Entries");
#   $text .= _page("PerlEx/PerfMon.html", "Performance Monitor");
    $text .= _page("PerlEx/Debugging.html", "Debugging PerlEx Scripts");
    $text .= library_indent_close();

    $text .= _page("", "Features");
    $text .= library_indent_open();
    $text .= _page("PerlEx/BEGIN-ENDBlocks.html", "BEGIN and END Blocks");
    $text .= _page("PerlEx/PersistentConnections.html", "Persistent Connections");
    $text .= _page("PerlEx/Embedded.html", "Embedding Perl in HTML files");
    $text .= _page("PerlEx/Reload.html", "Reload &amp; ReloadAll");
    $text .= _page("PerlEx/PerlExCoding.html", "Coding with PerlEx");
    $text .= library_indent_close();

    $text .= _page("", "Reference");
    $text .= library_indent_open();
    $text .= _page("PerlEx/FAQ.html", "PerlEx FAQ");
    $text .= _page("PerlEx/Troubleshooting.html", "Troubleshooting");
    $text .= _page("PerlEx/Precompiler.html", "PerlEx Precompiler");
    $text .= _page("PerlEx/ErrorMessages.html", "Event Log and Error Messages");
    $text .= _page("PerlEx/Bugs.html", "Reporting Bugs");
    $text .= library_indent_close();

    $text .= _page("", "Examples");
    $text .= library_indent_open();
    $text .= _page("http://localhost/PerlEx/examples.aspl", "Examples");
    $text .= _page("http://localhost/PerlEx/bm.htm", "Benchmarks");
    $text .= library_indent_close();

    $text .= library_indent_close();

    return $text;
}

1;
