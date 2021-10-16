package ActiveState::Indenter;

use strict;
require IO::Handle;  # ensure print method works on real file handles

sub new {
    my $class = shift;
    my $fh = shift;
    bless {
	fh => $fh,
	col => 0,
	line => 1,
	indent => 4,
	width => 70,
	tab => [],
    }, $class;
}

sub _print {
    my $self = shift;
    my $str = shift;
    my $fh = $self->{fh};

    return $fh->print($str) if $str eq "\n";

    my $indent_needed = $self->indent - $self->{col};
    if ($indent_needed > 0) {
	$fh->print(" " x $indent_needed);
	$self->{col} += $indent_needed;
    }
    $fh->print($str);
}

sub handle {
    my $self = shift;
    $self->{fh};
}

sub line_width {
    my $self = shift;
    my $old = $self->{width};
    $self->{width} = shift if @_;
    $old;
}

sub indent_offset {
    my $self = shift;
    my $old = $self->{indent};
    $self->{indent} = shift if @_;
    $old;
}

sub line {
    my $self = shift;
    my $old = $self->{line};
    $self->{line} = shift if @_;
    $old;
}

sub print {
    my $self = shift;
    my $fh = $self->{fh};
    my $str = join(" ", @_);
    my $indent = $self->indent;
    while ((my $nl = index($str, "\n")) >= 0) {
	$self->_print(substr($str, 0, $nl+1, ""));
	$self->{line}++;
	$self->{col} = 0;
    }
    return unless length($str);
    $self->_print($str);
    $self->{col} += length($str);
}

sub soft_space {
    my $self = shift;
    $self->print($self->{col} > $self->{width} ? "\n" : " ");
}

sub indent {
    my $self = shift;
    return @{$self->{tab}} ? $self->{tab}[-1] : 0;
}

sub over {
    my $self = shift;
    push(@{$self->{tab}}, $self->indent + (shift || $self->{indent}));
}

sub over_cur {
    my $self = shift;
    push(@{$self->{tab}}, $self->column);
}

sub over_abs {
    my $self = shift;
    push(@{$self->{tab}}, shift || 0);
}

sub back {
    my $self = shift;
    die "Underflow" unless @{$self->{tab}};
    pop(@{$self->{tab}});
}

sub column {
    my $self = shift;
    $self->{col};
}

sub depth {
    my $self = shift;
    return scalar(@{$self->{tab}});
}

1;

__END__

=head1 NAME

ActiveState::Indenter - Keep track of indentation levels

=head1 SYNOPSIS

 use ActiveState::Indenter;

 my $fh = ActiveState::Indenter->new(*STDOUT);
 $fh->print("if (foo()) {\n")
 $fh->over;
 $fh->print("# do something\n");
 $fh->back;
 $fh->print("}\n");

=head1 DESCRIPTION

The ActiveState::Indenter works like an output file handle but will
insert whitespace in the printed stream reflecting the current
indentation level.  It is useful for printing out nicely formatted
programs.

The following methods are available:

=over

=item $fh = ActiveState::Indenter->new( $filehandle )

The object constructor takes a file handle as argument.  It will
create a new indenter object that prints to the given file handle.

The $filehandle can actually be any object that implement a print()
method.

=item $fh->handle

Returns back the filehandle passed to the constructor.

=item $fh->line_width

=item $fh->line_width( $new_width )

This get/set the current line width.  The line width is used by the
soft_space() method.  The default is 70.

=item $fh->indent_offset

=item $fh->indent_offset( $new_offset )

This get/set the standard indentation offset.  The default is 4.

=item $fh->print( $string )

The print() will print the string given as argument to the wrapped
file handle but with suitable additional leading space added.  The
indenter object will never buffer output, so all content of the string
will have reached the file when print returns.

=item $fh->soft_space

This will print a space character " " if there is more room on the
line, or a newline otherwise.

=item $col = $fh->indent

This returns the current indentation column.

=item $fh->over

=item $fh->over( $offset )

This increase the indentation column with the given amount.  Without
argument the standard amount is used.

=item $fh->over_cur

This sets the indentation column the same as the current column.

=item $fh->over_abs( $col )

This set the indentation column to the number given.

=item $fh->back

Returns to the previous indentation column.

=item $col = $fh->column

Returns the current column position.

=item $line = $fh->line

=item $fh->line( $new_line )

This get/set the current line number.

=item $n = $fh->depth

Returns the current indentation depth.

=head1 COPYRIGHT

Copyright (C) 2003 ActiveState Corp.  All rights reserved.

=cut
