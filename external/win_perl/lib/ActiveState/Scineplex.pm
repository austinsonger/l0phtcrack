package ActiveState::Scineplex;

use 5.006001;
use strict;
use warnings;
use Carp qw(carp croak);

require Exporter;

our @ISA = qw(Exporter);
our @EXPORT_OK = qw(Annotate %SCE_TOKEN);

our $VERSION = '1.01';

use ActiveState::Scineplex::Constants;
our %SCLEX_LANG_CODE;
our %SCE_TOKEN;

require XSLoader;
XSLoader::load('ActiveState::Scineplex', $VERSION);

use constant CLASSIC_SCINEPLEX => 0;
use constant LINE_MAPPER_SCINEPLEX => 1;
use constant SINGLE_BUFFER_SCINEPLEX => 2;
use constant ASCII_LINE_MAPPER_SCINEPLEX => 3;
use constant ASCII_SINGLE_BUFFER_SCINEPLEX => 4;
use constant HTML_SCINEPLEX => 5;
use constant JSON_POS_GENERIC_SCINEPLEX => 6;

my %outputFormatName2Code = (
    html    => HTML_SCINEPLEX,
    line    => ASCII_LINE_MAPPER_SCINEPLEX,
    classic => CLASSIC_SCINEPLEX,
    json    => JSON_POS_GENERIC_SCINEPLEX,
);

my @Annotate_Opts = qw(
    outputFormat
    parsingStartState
    DumpSource
    DumpEndState
    DumpFoldLevels
    StopAfterDataSectionLine1
);

my %Annotate_Opts = map { $_ => 1 } @Annotate_Opts;


sub Annotate {
    my($buf, $lang, %opt) = @_;
    $lang = $SCLEX_LANG_CODE{lc($lang || 'perl')}
        || croak("Can't handle language '$lang'");

    my $format = $outputFormatName2Code{$opt{outputFormat} || 'line'};
    croak("Unrecognized output format '$opt{outputFormat}'")
	unless defined $format;  # can be zero
    $opt{outputFormat} = $format;

    if ($^W) {
	for (sort keys %opt) {
	    carp("Unrecognized option '$_'") unless $Annotate_Opts{$_};
	}
    }

    return AnnotateXS($buf, $lang, map $_ || 0, @opt{@Annotate_Opts});
}

1;

__END__

=head1 NAME

ActiveState::Scineplex - Perl extension to access Scineplex code lexer.

=head1 SYNOPSIS

  use ActiveState::Scineplex qw(Annotate);
  $color_info = Annotate($code, $lang, %options);

=head1 DESCRIPTION

Scineplex is a C library for heuristic parsing of source code in
various languages.  Scineplex is based on the Scintilla sources.  The
C<ActiveState::Scineplex> module provide a Perl interface to this library.

Currently this module implements an interface consisting of one function,
Annotate, which returns a scineplex-driven colorization for one or
more lines of source code.  It either returns a string giving the
colorization or throws an exception.

    $color_info = Annotate($code, $lang, %options);

The $code is one or more lines of source-code to be analyzed passed as
a single string.  The lines are separated by any newline sequence.

The $lang argument can be one of 'perl', 'python', 'ruby', 'vbscript', or 'xslt'.  The
default is 'perl'.

Additional %options can be passed as key/value pairs.  The following
options are supported (defaults in parentheses):

    outputFormat => 'html' | 'json' | 'line' | 'classic' ('line')
    parsingStartState => number (0) 
    DumpSource => 0 | 1 (0)
    DumpEndState => 0 | 1 (0)
    DumpFoldLevels => 0 | 1 (0)
    StopAfterDataSectionLine1 => 0 | 1 (0)

The C<outputFormat> is the most important option.  In C<classic> mode,
C<Annotate> echos back each character on the start of a line, followed
by separating white-space and its style value:

    $res = Annotate('$abc = 3;', 'perl', outputFormat => 'classic');
    print $res;

    $       12
    a       12
    b       12
    c       12
    chr(32) 0
    =       10
    chr(32) 0
    3       4
    ;       10
    chr(10) 0

Symbolic names for the numeric style values can be looked up in the
%SCE_TOKEN hash (exportable).  For example $SCE_TOKEN{perl}{12} is the
string "SCE_PL_SCALAR".

Setting C<outputFormat> to C<line> gives a terser output, and
represents each numeric style with the character corresponding to the
style added to the ASCII value of character '0':

    $res = Annotate('$abc = 3;', 'perl', outputFormat => 'line');
    print $res;

    <<<<0:04:

Setting C<outputFormat> to C<html> returns an HTML-encoded string
containing the original code wrapped in C<span> tags with generic
classes with names like "variable", "operator", etc.  This kind of
output is designed to be wrapped in C<pre> tags, and styled with a CSS
file of that contains rules like

    pre span.comments {
      color: 0x696969;
      font-style: italic;
    }

Default text is not placed in a span tag.

Setting C<outputFormat> to C<json> returns a JSON array of arrays.
Each one of the inner arrays contains a generic style label together
with the span in positions; [$tag, $line, $col, $len].  The returned
JSON array will also be valid Perl code and can be converted to a Perl
array using Perl's builtin C<eval> function.

Example:


    $res = Annotate('$abc = 3;', 'perl', outputFormat => 'json');
    print $res;
    $array = eval $res;

    [
     ["variable",1,0,4],
     ["operator",1,5,1],
     ["number",1,7,1],
     ["operator",1,8,1]
    ]

The C<parsingStartState> setting should be used only when you know
that the code starts with a given style, such as lines 3-5 of a
multi-line string.

The C<DumpSource> flag is used only with C<line> output.  It is
intended mostly for human consumption, and produces output like the
following:

    $res = Annotate('$abc = 3;', 'perl', DumpSource=>1);
    print $res;

    $abc = 3;
    <<<<0:04:

The C<DumpEndState> is used only in C<line> mode, and gives the styles
for whichever characters constitute the line-end sequence:

    $res = Annotate(qq($abc = 3;\r\n), 'perl', DumpSource=>1, DumpEndState=>1);
    print $res;

    $abc = 3;
    <<<<0:04:00

The C<DumpFoldLevels> is used only in C<line> mode, and gives the fold
levels as a 4-hex-digit sequence in a leading column.

    $res = Annotate(qq(if(1) {\n$abc = 3;\n}\n), 'perl', DumpSource=>1, DumpEndState=>1);
    print $res;

    2400 if(1) {
         55:4:0:
    0401 $abc = 3
         <<<<0:04
    0401 }
         :

The C<StopAfterDataSectionLine1> is used only for Perl code in C<line>
mode.

=head1 SEE ALSO

Info on scintilla available at http://www.scintilla.org.

=head1 COPYRIGHT

Copyright (C) 2005 by ActiveState Software Inc.

=cut
