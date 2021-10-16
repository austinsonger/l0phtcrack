package ActiveState::Tkx::TextSyntaxTags;

use strict;
use base 'Exporter';

our @EXPORT_OK = qw(update_syntax_tags);

use Carp qw(carp);

our %TAG_CONF = (
    syn_comment  => [-foreground => "#696969"],
    syn_keyword  => [-foreground => "#781f87"],
  # syn_variable => [-foreground => "#000000"],
    syn_operator => [-foreground => "#871f78"],
    syn_number   => [-foreground => "#8e2323"],
    syn_string   => [-foreground => "#00008b"],
    syn_regex    => [-foreground => "#00008b"],
);

sub setup_tag {
    my($t, $tag) = @_;
    if (my $c = $TAG_CONF{$tag}) {
	$t->tag_configure($tag, @$c);
        $t->tag_lower($tag, "sel");
	return 1;
    }

    #warn "XXX $tag not tagged.\n";
    return 0;
}

sub update_syntax_tags {
    my($t, %opt) = @_;

    # use ActiveState::Scineplex 1.01;
    return unless eval { require ActiveState::Scineplex };
    return unless $ActiveState::Scineplex::VERSION >= 1.01;

    my $file = delete $opt{file};
    my $lang = delete $opt{lang};
    my $range = delete $opt{range};
    my $linemap = delete $opt{linemap};
    my $verbose = delete $opt{verbose};

    if ($^W) {
	carp("Unrecognized option '$_'") for sort keys %opt;
    }

    if (ref($linemap) eq "HASH") {
	my $h = $linemap;
	$linemap = sub { $h->{$_[0]} };
    }

    my($range_beg, $range_end) = ("1.0", "end");
    if ($range) {
        my ($line_beg, $line_end) = ref($range) ? @$range : split(/-/, $range);
        $line_end++;
        $linemap = sub { ($line_beg + $_[0] - 1) . ".0" };
        $range_beg = "$line_beg.0";
        $range_end = "$line_end.0";
    }

    my %tags;
    for (Tkx::SplitList($t->tag_names)) {
	next unless /^syn_/;
	$t->tag_remove($_, $range_beg, $range_end);
	$tags{$_}++;
    }

    eval {
	my $code;
	if ($file) {
	    open(my $fh, "<", $file) || die "Can't open '$file': $!";
	    local $/;
	    $code = <$fh>;
	}
	else {
	    $code = $t->get($range_beg, $range_end);
	}
	my $arr = eval ActiveState::Scineplex::Annotate(
            $code,
            $lang,
	    outputFormat => "json",
        );

	for my $e (@$arr) {
	    my($style, $line, $col, $len) = @$e;
	    $line = $linemap ? &$linemap($line) : "$line.0";
	    next unless $line;

	    my $tag = "syn_$style";
	    unless ($tags{$tag}) {
		next if exists $tags{$tag} ||
		        !($tags{$tag} = setup_tag($t, $tag));
	    }
	    my $end_col = $col + $len;
	    $t->tag_add($tag, "$line + $col chars", "$line + $end_col chars");
	}
    };
    if ($verbose && $@) {
	warn $@;
    }

    return;
}


1;

__END__

=head1 NAME

ActiveState::Tkx::TextSyntaxTags - perform syntax coloring of text wigets

=head1 SYNOPSIS

  use ActiveState::Tkx::TextSyntaxTags qw(update_syntax_tags);
  my $text = $mw->new_text;
  $text->insert("end", file_content($file));
  update_syntax_tags($text, lang => "perl");

=head1 DESCRIPTION

The C<ActiveState::Tkx::TextSyntaxTags> module provide a single function:

=over

=item update_syntax_tags( $text_w, %options )

This will assign tags to ranges of the text widget that correspond to
syntax elements.

=over

=item file => $filename

Tell update_syntax_tags() which file is displayed in the text widget.
If not provided all text in the widget is assumed to be code (unless
the 'range' option is provided).

=item lang => $lang

The programming language displayed.  The $lang argument can be one of
'perl', 'python', 'ruby', 'vbscript', or 'xslt'.  The default is
'perl'.  Passed on to C<ActiveState::Scineplex>.

=item linemap => \%linemap

=item linemap => \&linemap

The optional C<linemap> argument can be used to specify where the
source lines from the file specified with the 'file' argument is
located in the text widget.  If not provided it is assumed that the
content of the text widget match the source text exactly; this is same
as C<< linemap => sub { "$_[0].0" } >>.

The C<linemap> maps from a line number (1 based) to a position (index)
into the text widget of the standard "line.char" form.  If the linemap
return a false value then no syntax tagging is performed for the given
line.

=item range => [$first, $last]

=item range => "$first-$last"

Add syntax color to the given line range within the text widget.  The
line number range can either be passed as a 2 element array reference
or as a string with the line numbers separated by "-".

This option can't be combined with the 'file' and 'linemap' options.

=back

=back

=head1 SEE ALSO

L<ActiveState::Scineplex>

=head1 COPYRIGHT

Copyright (C) 2010 ActiveState Software Inc.  All rights reserved.

=cut
