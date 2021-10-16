package ActiveState::Table;

our $VERSION = "1.00";

use strict;
use Carp qw(carp);

sub new {
    my $class = shift;
    bless {
	fields => [],
        rows => [],
	null => "NULL",
    }, $class;
}

sub fields {
    my $self = shift;
    @{$self->{fields}};
}

sub rows {
    my $self = shift;
    @{$self->{rows}};
}

sub fetchrow {
    my($self, $i) = @_;
    return $self->{rows}[$i] unless wantarray;
    my @row = @{$self->{rows}[$i] || []};
    push(@row, (undef) x ($self->fields - @row));
    @row;
}

sub fetchrow_arrayref {
    my($self, $i) = @_;
    return $self->{rows}[$i];
}

sub fetchrow_hashref {
    my($self, $i) = @_;
    my $row = $self->{rows}[$i] || return undef;
    my %hash;
    my @fields = @{$self->{fields}};
    $i = 0;
    $hash{shift @fields} = $row->[$i++] while @fields;
    return \%hash;
}

sub add_field {
    my $self = shift;
    for my $field (@_) {
	push(@{$self->{fields}}, $field)
	    unless grep $_ eq $field, @{$self->{fields}};
    }
}

sub add_row {
    my $self = shift;
    my $row = shift;
    $row = [$row, @_] unless ref($row);

    if (ref($row) eq "HASH") {
	my @row;
	for my $k (sort keys %$row) {
	    my $i = 0;
	    for my $h (@{$self->{fields}}) {
		last if $h eq $k;
		$i++;
	    }
	    $self->{fields}[$i] = $k;
	    $row[$i] = $row->{$k};
	}
	push(@{$self->{rows}}, \@row);
    }
    elsif (ref($row) eq "ARRAY") {
	die "NYI";
    }
    else {
	die "Can't handle " . ref($row) . " rows";
    }
}

sub add_sep {
    my($self, $idx) = @_;
    $idx = @{$self->{rows}} - 1 unless defined $idx;
    $self->{sep_f} ||= do {
	my $h = $self->{sep_idx} = {};
	sub {
	    my $row = shift;
	    return $h->{$row};
	};
    };
    $self->{sep_idx}{$idx}++;
}

sub sort {
    my $self = shift;
    my $comparator = shift;
    package main;  # so that $a, $b is visible to the $comparator
    @{$self->{rows}} = sort $comparator @{$self->{rows}};
    return;
}

sub as_csv {
    my($self, %opt) = @_;
    my $sep = delete $opt{field_separator};
    my $eol = delete $opt{row_separator};
    my $null = delete $opt{null};
    my $show_header = delete $opt{show_header};

    if (%opt && $^W) {
	carp("Unrecognized option '$_'") for keys %opt;
    }

    # defaults;
    $sep = "," unless defined $sep;
    $eol = "\n" unless defined $eol;
    $null = $self->{null} unless defined $null;
    $show_header = 1 unless defined $show_header;

    my $fields = $self->{fields};
    my @lines;
    push(@lines, join($sep, @$fields)) if $show_header;
    for my $row (@{$self->{rows}}) {
	my @v = (@$row, ((undef) x (@$fields - @$row)));
	for (@v) {
	    $_ = $null unless defined;
	    if (length($sep) && (index($_, $sep) >= 0 || /"/)) {
		s/"/""/g;
		$_ = qq("$_");
	    }
	}
	push(@lines, join($sep, @v));
    }
    return join($eol, @lines, "");
}

sub as_box {
    my($self, %opt) = @_;
    my $null = delete $opt{null};
    my $show_header = delete $opt{show_header};
    my $show_trailer = delete $opt{show_trailer};
    my $align = delete $opt{align};
    my $box_chars = delete $opt{box_chars};
    my $max_width = delete $opt{max_width};

    if (%opt && $^W) {
	carp("Unrecognized option '$_'") for keys %opt;
    }

    # defaults;
    $null = $self->{null} unless defined $null;
    $show_header = 1 unless defined $show_header;
    $show_trailer = 1 unless defined $show_trailer;

    my $rows = $self->rows;
    my $sep_f = $self->{sep_f};
    my @out;
    if (my @title = $self->fields) {
	@title = ("") x @title unless $show_header;
	my @w = map length, @title;
	my @align = map $align->{$_} || "left", @title;

	# find optimal width
	my $max = $rows - 1;
	my $i;
	for $i (0 .. $max) {
	    my @field = $self->fetchrow($i);
	    my $j = 0;
	    for (@field) {
		$_ = $null unless defined;
		$w[$j] = length if $w[$j] < length;
		$j++;
	    }
	}

	my $PAD = " ";
	my $PAD_DASH = "-" x length($PAD);
	if ($max_width) {
	    my $width = @w * (length($PAD) * 2 + 1) + 1;
	    $width += $_ for @w;
	    my $too_much = $width - $max_width;
	    if ($too_much > 0 && length($PAD) && $box_chars && $box_chars ne "ascii") {
		$too_much -= @w * length($PAD) * 2;
		$PAD = "";
		$PAD_DASH = "";
	    }
	    while ($too_much > 0) {
		# try to shrink the fields until we fit
		my $widest = @w - 1;
		for (my $j = $widest - 1; $j >= 0; $j--) {
		    if ($w[$j] > $w[$widest]) {
			$widest = $j;
		    }
		}
		die unless $w[$widest] > 1;
		$w[$widest]--;
		$too_much--;
	    }
	}

	_stretch(\@title, \@w, undef, $box_chars);
	my $sep = "q$PAD_DASH" . join("${PAD_DASH}w$PAD_DASH", map { "-" x length } @title) . "${PAD_DASH}e\n";
	my $I = _lines("|", $box_chars);
	push(@out, _lines($sep, $box_chars));
	$sep =~ tr/qwe/asd/;
	if ($show_header) {
	    push(@out, "$I$PAD", join("$PAD$I$PAD", @title), "$PAD$I\n");
	    push(@out, _lines($sep, $box_chars)) if $rows;
	}

	for $i (0 .. $max) {
	    my @field = $self->fetchrow($i);
	    for (@field) { $_ = $null unless defined }
	    _stretch(\@field, \@w, \@align, $box_chars);
	    push(@out, "$I$PAD", join("$PAD$I$PAD", @field), "$PAD$I\n");
	    if ($sep_f && $i < $max && $sep_f->($i)) {
		push(@out, _lines($sep, $box_chars))
	    }
	}
	$sep =~ tr/asd/zxc/;
	push(@out, _lines($sep, $box_chars));
    }
    if ($show_trailer) {
	push(@out, "  (1 row)\n") if $rows == 1;
	push(@out, "  ($rows rows)\n") if $rows != 1;
    }

    return join("", @out) if defined wantarray;
    print @out;
}

sub _lines {
    my($box, $box_chars) = @_;
    if (!$box_chars || $box_chars eq "ascii") {
	$box =~ tr/qweasdzxc/+-+++++-+/;
    }
    elsif ($box_chars eq "dos") {
	$box =~ tr/qweasdzxc\-|/\xDA\xC2\xBF\xC3\xC5\xB4\xC0\xC1\xD9\xC4\xB3/;
    }
    elsif ($box_chars eq "unicode") {
	$box =~ tr/qweasdzxc\-|/\x{250C}\x{252C}\x{2510}\x{251C}\x{253C}\x{2524}\x{2514}\x{2534}\x{2518}\x{2500}\x{2502}/;
    }
    else {
	$box =~ tr/qweasdzxc\-|/$box_chars/;
    }
    $box;
}

sub _stretch {
    my($text, $w, $a, $box_chars) = @_;
    my $i = 0;
    for (@$text) {
	my $align = $a->[$i] || "left";
	my $pad = ($w->[$i] - length);
	if ($pad > 0) {
	    $pad = " " x $pad;
	    if ($align eq "right") {
		substr($_, 0, 0) = $pad;
	    }
	    elsif ($align eq "center") {
		my $left_pad = substr($pad, 0, length($pad)/2, "");
		$_ = $left_pad . $_ . $pad;
	    }
	    else {
		$_ .= $pad;
	    }
	}
	elsif ($pad < 0) {
	    $box_chars ||= "ascii";
	    if ($box_chars eq "dos") {
		substr($_, $w->[$i] - 1) = "\xAF";
	    }
	    elsif ($box_chars eq "unicode") {
		substr($_, $w->[$i] - 1) = "\xBB";
	    }
	    elsif ($w->[$i] > 10) {
		substr($_, $w->[$i] - 3) = "...";
	    }
	    elsif ($w->[$i] >= 1) {
		substr($_, $w->[$i] - 1) = "+";
	    }
	    else {
		$_ = "";  # ultimate shrinkage, should not happen
	    }
	}
	$i++;
    }
}

1;

__END__

=head1 NAME

ActiveState::Table - Simple table class

=head1 SYNOPSIS

 $t = ActiveState::Table->new;
 $t->add_row({ a => 1, b => 2 });
 print $t->as_csv;

=head1 DESCRIPTION

Instances of the C<ActiveState::Table> class represent a 2 dimensional
table of fields (or columns if you wish) and rows.  The fields are
ordered and have case-sensitive names. The rows are numbered.

The following methods are provided:

=over

=item $t = ActiveState::Table->new

This creates a new empty table object.

=item $t->fields

This returns the current field names.  In scalar context it returns
the number of fields.

=item $t->rows

This returns the current rows.  Each row is returned as reference to an
array of values in the same order as the fields. The array might be
shorter than the number of fields, when the trailing values are C<undef>.

In scalar context it returns the number of rows in the table.

=item $t->fetchrow( $index )

This returns the given row.  An array reference is returned in scalar
context.  The array might be shorter than the number of fields, when
the trailing values are C<undef>.

In list context the values are returned one by one.  There will be as
many values as there are fields in the table.  Some values might be
C<undef>.

If there is no row with the given $index, then C<undef> is returned in
scalar context and the empty list in list context.

=item $t->fetchrow_arrayref( $index )

Same as fetchrow() but will return an array reference even in list
context.

=item $t->fetchrow_hashref( $index )

This returns the given row.  A hash reference is returned with keys
corresponding to the field names and the values corresponding to the
given row.  The values might be C<undef>, but a key for all the fields
will exist.

If there is no row with the given $index, then C<undef> is returned.

=item $t->add_field( $field )

This adds another field to the table.  The field must be a string.  If
the field already exists it is not added again, and the add_field()
call does nothing.

There is no return value.

=item $t->add_row( $row )

This adds another row to the table.  The row must currently be a hash
reference.  If the hash contains new fields they are added
automatically in sorted order.  To enforce an order add the fields
before adding rows.

There is no return value.

=item $t->add_sep

=item $t->add_sep( $index )

This adds a separator to the table, shown as a horizonal line in the
as_box() output.  If $index is provided the separator will appear
after the given row, otherwise the separator will appear after last
row added.

=item $t->sort( $comparator )

This will sort the rows of the table using the given $comparator
function to compare elements.  The $comparator is called as for perl's
builtin sort function.  References to the rows to compare is available
in C<$::a> and C<$::b> in the form returned by
C<< $t->fetchrow_arrayref >>.

=item $t->as_box( %options )

This formats the table as text and returns it. The following options
might be provided as key/value pairs:

   name                 | default
   ---------------------+----------
   align                | {}
   null                 | "NULL"
   show_header          | 1
   show_trailer         | 1
   box_chars            | "ascii"
   max_width            | undef
   ---------------------+----------

The C<align> option is a hash with field names as keys and the strings
"left", "right" or "center" as values.  Alignment for fields not found
in this hash is "left".

The C<box_chars> is either the name of a box drawing scheme or the 11
characters to use; starting with the upper left corner and going down
one row at a time to the lower right corner, and finally the
horizontal and vertical line character.  The current named schemes are
"dos" and "unicode".

If C<max_width> is specified it limits how wide the box can get.  The
longest fields are truncated until the box is no wider than the
given number of characters.  Truncated fields are shown with trailing
"..." or "+" marker.

=item $t->as_csv( %options )

This formats the table as a CSV file ("comma-separated-values") and
returns it.  The following options might be provided as key/value
pairs:

   name                 | default
   ---------------------+----------
   field_separator      | ","
   row_separator        | "\n"
   null                 | "NULL"
   show_header          | 1
   ---------------------+----------

Fields that contains the C<field_separator> or the quote character
will be quoted.

=back

=head1 BUGS

none.

=head1 SEE ALSO

RFC 4180 and L<http://en.wikipedia.org/wiki/Comma-separated_values>

=cut
