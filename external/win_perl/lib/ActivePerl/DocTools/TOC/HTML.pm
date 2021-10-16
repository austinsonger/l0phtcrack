package ActivePerl::DocTools::TOC::HTML;

use strict;
use warnings;

use base 'ActivePerl::DocTools::TOC';

sub text {
    my $text =  join("\n", @_, "");
    return sub { $text };
}


# extra info is tedious to collect -- is done in a subclass or something.
sub extra { '' };

*header = text ("</ul>","<hr>","<strong>Perl Core Documentation</strong>","<ul>",);

# pod
sub before_pods { '' }

*pod_separator = text('<br />');

sub pod {
    my($self, $file) = @_;
    my $key = $^O eq "darwin" ? "pods::$file" : "Pod::$file";
    $key = "pods::$file" if $^O eq "MSWin32" && Win32::BuildNumber() >= 821;
    return (qq'  <li>' . _page($self->{pods}->{$key}, $file, $self->extra($file)));
}

sub after_pods { '</ul>' }

# scripts
*before_scripts = text("<hr>","<strong>Programs</strong><br />","<ul>",);

sub script {
    my($self, $file) = @_;
    return (qq'  <li>' . _page($self->{scripts}->{$file}, $file, $self->extra($file)));
}

sub after_scripts { '</ul>' }

# pragmas
*before_pragmas = text("<hr>","<strong>Pragmas</strong><br />","<ul>",);

sub pragma {
    my($self, $file) = @_;
    return (qq'  <li>' . _page($self->{pragmas}->{$file}, $file, $self->extra($file)));
}

sub after_pragmas { '</ul>' }

# libraries
*before_libraries = text("<hr>","<strong>Modules</strong><br />","<ul>",);

sub library_indent_open  { '' }  # text('<ul compact>');
sub library_indent_close { '' }  # text('</ul>');
sub library_indent_same  { '' }

sub library {
    my($self, $file, $showfile, $depth) = @_;
    return (qq'  <li style="margin-left: ' . $depth . qq'em;">' . _page($self->{files}->{$file}, $showfile, $self->extra($file)));
}

sub library_container {
    my($self, $file, $showfile, $depth) = @_;
    return qq(  <li style="margin-left: ${depth}em;">$showfile</li>\n);
}

sub after_libraries { '</ul>' }

*footer = text("\n\n</div>\n</body>\n</html>");

sub _page {
    my($href, $text, $extra) = @_;
    die "bad arguments to _page: ($href, $text, $extra)" unless defined $href and defined $text;
    $extra ||= '';
    $extra = " $extra" if $extra;  # just to make it EXACTLY identical to the old way. 
    return qq'<a class="doc" href="$href">$text</a>$extra</li>\n';
}


sub boilerplate {
    return boiler_header() . boiler_links();
}

sub boiler_header {
    return <<'HERE';
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>

<head>
<!-- saved from url=(0017)http://localhost/ -->
<title>ActivePerl User Guide - Table of Contents</title>
<base target="PerlDoc">
<link rel="STYLESHEET" href="Active.css" type="text/css">
</head>

<body>

<h1>Table of Contents</h1>
HERE
}


sub boiler_links {
    my $retval = <<HERE;
<div nowrap>
<div class="toc">
<ul>
<li><strong>Getting Started</strong>
<ul>
  <li><a class="doc" href="perlmain.html">Welcome To ActivePerl</a></li>
  <li><a class="doc" href="release.html">Release Notes</a></li>
  <li><a class="doc" href="install.html">Installation Guide</a></li>
  <li><a class="doc" href="readme.html">Getting Started</a></li>
  <li><a class="doc" href="changes.html">ActivePerl Change Log</a></li>
HERE
  $retval .= <<HERE if -e "$ActivePerl::DocTools::TOC::dirbase/unsupported.html";
  <li><a class="doc" href="unsupported.html">Unsupported Features</a></li>
HERE
  $retval .= <<HERE;
  <li><a class="doc" href="resources.html">More Resources</a></li>
  <li><a class="doc" href="Copyright.html">License and Copyright</a></li>
</ul>
</li>
<li><strong>ActivePerl Components</strong>
<ul>
  <li><a class="doc" href="Components/Descriptions.html">Overview</a></li>
  <li><a class="doc" href="faq/ActivePerl-faq2.html">Using PPM</a></li>
HERE
my $site = "site/";
$site = "" if -f "$ActivePerl::DocTools::TOC::dirbase/lib/Win32/OLE/Browser.html";
$retval .= <<HERE if $^O eq "MSWin32";
  <li style="list-style: none"><strong>Windows Specific</strong>
  <ul>
    <li><a class="doc" href="${site}lib/Win32/OLE/Browser.html" target="_blank">OLE Browser</a></li>
    <li><a class="doc" href="Components/Windows/PerlScript.html">PerlScript</a></li>
    <li><a class="doc" href="../eg/IEExamples/index.htm">PerlScript Examples</a></li>
    <li><a class="doc" href="Components/Windows/PerlEz.html">PerlEz</a></li>
    <li><a class="doc" href="Components/Windows/PerlISAPI.html">Perl for ISAPI</a></li>
  </ul>
HERE
    $retval .= PerlEx_links();
    $retval .= <<HERE;
  </li>
</ul>
</li>
<li><strong>ActivePerl FAQ</strong>
<ul>
  <li><a class="doc" href="faq/ActivePerl-faq.html">Introduction</a></li>
  <li><a class="doc" href="faq/ActivePerl-faq1.html">Availability &amp; Install</a></li>
  <li><a class="doc" href="faq/ActivePerl-faq3.html">Docs &amp; Support</a></li>
HERE
    $retval .= <<HERE if $^O eq "MSWin32";
  <li style="list-style: none"><strong>Windows Specific FAQ</strong>
    <ul>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq2.html">Perl for ISAPI</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq4.html">Windows 9X/Me/NT/200X/XP</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq5.html">Windows Quirks</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq6.html">Web Server Configuration</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq7.html">Web Programming</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq8.html">Windows Programming</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq9.html">Modules &amp; Samples</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq10.html">Embedding &amp; Extending</a></li>
      <li><a class="doc" href="faq/Windows/ActivePerl-Winfaq12.html">Using OLE with Perl</a></li>
    </ul>
  </li>
</ul>
</li>
<li><strong>Windows Scripting</strong>
<ul>
  <li><a class="doc" href="Windows/ActiveServerPages.html">Active Server Pages</a></li>
  <li><a class="doc" href="Windows/WindowsScriptHost.html">Windows Script Host</a></li>
  <li><a class="doc" href="Windows/WindowsScriptComponents.html">Windows Script Components</a></p></li>
</ul>
</li>
</ul>


HERE
    return $retval;
}

sub PerlEx_links {
    return "" unless -e "$ActivePerl::DocTools::TOC::dirbase/PerlEx/Welcome.html";

    return <<HERE;

<strong>PerlEx</strong>
<ul>
  <li><strong>Getting Started</strong>
    <ul>
      <li><a class="doc" href="PerlEx/Welcome.html">Welcome</a></li>
      <li><a class="doc" href="PerlEx/QuickStart.html">Getting Started</a></li>
      <li><a class="doc" href="PerlEx/HowItWorks.html">How PerlEx Works</a></li>
    </ul>
  </li>
  <li><strong>Configuration</strong>
    <ul>
      <li><a class="doc" href="PerlEx/WebServerConfig.html">WebServer Configuration</a></li>
      <li><a class="doc" href="PerlEx/IntrpClass.html">PerlEx Interpreter Classes</a></li>
      <li><a class="doc" href="PerlEx/RegistryEntries.html">PerlEx Registry Entries</a></li>
      <!-- <li><a class="doc" href="PerlEx/PerfMon.html">Performance Monitor</a></li> -->
      <li><a class="doc" href="PerlEx/Debugging.html">Debugging PerlEx Scripts</a></li>
    </ul>
  </li>
  <li><strong>Features</strong>
    <ul>
      <li><a class="doc" href="PerlEx/BEGIN-ENDBlocks.html">BEGIN and END Blocks</a></li>
      <li><a class="doc" href="PerlEx/PersistentConnections.html">Persistent Connections</a></li>
      <li><a class="doc" href="PerlEx/Embedded.html">Embedding Perl in HTML files</a></li>
      <li><a class="doc" href="PerlEx/Reload.html">Reload &amp; ReloadAll</a></li>
      <li><a class="doc" href="PerlEx/PerlExCoding.html">Coding with PerlEx</a></li>
    </ul>
  </li>
  <li><strong>Reference</strong>
    <ul>
      <li><a class="doc" href="PerlEx/FAQ.html">PerlEx FAQ</a></li>
      <li><a class="doc" href="PerlEx/Troubleshooting.html">Troubleshooting</a></li>
      <li><a class="doc" href="PerlEx/Precompiler.html">PerlEx Precompiler</a></li>
      <li><a class="doc" href="PerlEx/ErrorMessages.html">Event Log and Error Messages</a></li>
      <li><a class="doc" href="PerlEx/Bugs.html">Reporting Bugs</a></li>
    </ul>
  </li>
  <li><strong>Examples</strong>
    <ul>
      <li><a class="doc" href="http://localhost/PerlEx/examples.aspl">Examples</a></li>
      <li><a class="doc" href="http://localhost/PerlEx/bm.htm">Benchmarks</a></li>
    </ul>
  </li>
</ul>
HERE
}

1;
