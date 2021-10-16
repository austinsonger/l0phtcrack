package ActiveState::Config::INI;

our $VERSION = "0.01";

use strict;
use Carp qw(croak);

sub new {
    my $class = shift;
    my $self = bless {}, $class;
    if (@_) {
        $self->read(shift);
    }
    else {
        $self->_init;
    }
    return $self;
}

sub _init {
    my $self = shift;
    @{$self->{lines}} = ();
    %{$self->{sect}} = ( "" => { line => -1 } );
}

sub read {
    my($self, $filename) = @_;
    $self->{filename} = $filename;
    open(my $fh, "<", $filename) || die "Can't open '$filename': $!";
    binmode($fh);
    $self->_read($fh);
}

sub _read {
    my($self, $fh) = @_;

    $self->_init;

    my $cur_sect = "";

    while (<$fh>) {
        push(@{$self->{lines}}, $_);
        my $line = @{$self->{lines}} - 1;
        if (/^(\s*\[\s*)(-\s*)?(.+?)\s*\]\s*($|;)/) {
            $cur_sect = $3;
            $self->{sect}{$cur_sect} = {
                line => $line,
                col => length($1),
                disabled => $2 || "",
            }
        }
        elsif (/^((\s*)(#\s*)?([^=]+?)\s*=[ \t]*)(.*?)\s*$/ ) {
            my $h = $self->{sect}{$cur_sect}{prop}{$4} = {
                line => $line,
                col => length($2),
                disabled => $3 || "",
                val => $5,
                val_col => length($1),
            };
            $h->{val} =~ s/\s+;\s.*//;  # inline comments not part of value
        }
    }

    if (@{$self->{lines}} && $self->{lines}[-1] !~ /\n\z/) {
        $self->{lines}[-1] .= $self->newline;
    }

}

sub write {
    my $self = shift;
    my $filename = @_ ? shift : $self->{filename};
    croak("No filename given") unless defined($filename) && length($filename);
    open(my $fh, ">", $filename) || die "Can't open '$filename': $!";
    binmode($fh);
    print $fh @{$self->{lines}};
    close($fh) || die "Can't write to '$filename': $!";
}

sub content {
    my $self = shift;
    my $old;
    $old = join("", @{$self->{lines}}) if defined(wantarray);
    if (@_) {
        my $new = shift;
        open(my $fh, "<", \$new);
        $self->_read($fh);
    }
    $old;
}

sub newline {
    my $self = shift;
    return $1 if @{$self->{lines}} && $self->{lines}[0] =~ /(\r?\n)\z/;
    return "\r\n" if $^O eq "MSWin32";
    return "\n";
}

sub sections {
    my $self = shift;
    sort { $self->{sect}{$a}{line} <=> $self->{sect}{$b}{line} }
        grep length, keys %{$self->{sect}}
}

sub insert_line {
    my($self, $idx, $line) = @_;
    my $nl = $self->newline;
    push(@{$self->{lines}}, $nl) while @{$self->{lines}} < $idx;
    $line .= $nl unless $line =~ /\n\z/;
    splice(@{$self->{lines}}, $idx, 0, $line);
    $self->_adjust_line_numbers($idx, 1);
}

sub _adjust_line_numbers {
    my($self, $start, $offset) = @_;
    for my $s (values %{$self->{sect}}) {
        $s->{line} += $offset if $s->{line} >= $start;
        for my $p (values %{$s->{prop}}) {
            $p->{line} += $offset if $p->{line} >= $start;
        }
    }
}

sub append_lines {
    my($self, @lines) = @_;
    my $nl = $self->newline;
    for (@lines) {
        $_ .= $nl unless /\n\z/;
    }
    push(@{$self->{lines}}, @lines);
}

sub delete_lines {
    my($self, $offset, $length) = @_;
    my @deleted = splice(@{$self->{lines}}, $offset, $length);
    $self->_adjust_line_numbers($offset, -$length);
    return @deleted;
}

sub _add_section {
    my($self, $sect) = @_;
    unless ($self->{sect}{$sect}) {
        my $nl = $self->newline;
        push(@{$self->{lines}}, $nl) if @{$self->{lines}} && $self->{lines}[-1] !~ /^\s*#/;
        push(@{$self->{lines}}, "[$sect]$nl");
        $self->{sect}{$sect} = {
            line => scalar(@{$self->{lines}}) - 1,
            col => 1,
            disabled => "",
        };
    }
    return $self->{sect}{$sect};
}

sub _add_property {
    my($self, $sect, $prop) = @_;
    my $s = $self->_add_section($sect);
    unless ($s->{prop}{$prop}) {
        $self->insert_line($s->{line} + 1, "$prop = ");
        $s->{prop}{$prop} = {
            line => $s->{line} + 1,
            col => 0,
            disabled => "",
            val => "",
            val_col => length($prop) + 3,
        };
    }
    return $s->{prop}{$prop};
}

sub section_enabled {
    my $self = shift;
    my $sect = shift;
    my $old = $self->{sect}{$sect} && !$self->{sect}{$sect}{disabled};
    if (@_) {
        my $disabled = shift() ? "" : "-";
        croak("Can't enable or disable global section") if $sect eq "";
        my $s = $self->_add_section($sect);
        if ($s->{disabled} ne $disabled) {
            my $line = $s->{line};
            substr($self->{lines}[$line], $s->{col}, length($s->{disabled})) = $disabled;
            $s->{disabled} = $disabled;
        }
    }
    return $old;
}

sub have_section {
    my($self, $sect) = @_;
    return exists $self->{sect}{$sect};
}

sub add_section {
    my($self, $sect, $comment) = @_;
    return if $self->have_section($sect);
    if ($comment) {
        my @comment = split(/\r?\n/, $comment);
        for (@comment) {
            $_ = "# $_" unless /^\s*#/;
        }
        $self->append_lines("", @comment);
    }
    $self->_add_section($sect);
}

sub delete_section {
    my($self, $sect) = @_;
    my $s = $self->{sect}{$sect};
    return () unless $s;
    my $beg = $s->{line};
    my $end;
    for (values %{$self->{sect}}) {
        my $line = $_->{line};
        next if $line <= $beg;
        $end = $line if !defined($end) || $line < $end;
    }
    $end = @{$self->{lines}} unless defined($end);
    $beg = 0 if $beg == -1;  # global sect
    return () if $beg == $end;

    for ($beg, $end) {
        while ($_ && $self->{lines}[$_ - 1] =~ /^\s*#/) {
            last if $self->{lines}[$_ - 1] =~ m/^\s*#\s*?([^=]+?)\s*=/;
            $_--;
        }
    }

    if ($sect eq "") {
        %$s = (line => -1);  # reset
    }
    else {
        delete $self->{sect}{$sect};
    }

    return $self->delete_lines($beg, $end - $beg);
}

sub properties {
    my($self, $sect) = @_;
    my $s = $self->{sect}{$sect};
    return unless $s && $s->{prop};
    sort { $s->{prop}{$a}{line} <=> $s->{prop}{$b}{line} } keys %{$s->{prop}};
}

sub _property {
    my $self = shift;
    my $sect = shift;
    my $prop = shift;
    my $old = $self->{sect}{$sect} &&
              $self->{sect}{$sect}{prop}{$prop} &&
              $self->{sect}{$sect}{prop}{$prop}{val};
    if (@_) {
        my $val = shift;
        my $p = $self->_add_property($sect, $prop);
        if (defined $val) {
            my $line = $p->{line};
            substr($self->{lines}[$line], $p->{val_col}, length($p->{val})) = $val;
            $p->{val} = $val;
        }
        if (!@_ || defined($_[0])) {
            $self->property_enabled($sect, $prop, defined($val) && (!@_ || $_[0]));
        }
    }
    return $old;
}

sub property {
    my $self = shift;
    my $sect = shift;
    my $prop = shift;
    my $old = $self->_property($sect, $prop, @_);
    undef($old) unless $self->property_enabled($sect, $prop);
    return $old;
}

sub have_property {
    my($self, $sect, $prop) = @_;
    return $self->{sect}{$sect} && $self->{sect}{$sect}{prop}{$prop};
}

sub property_enabled {
    my $self = shift;
    my $sect = shift;
    my $prop = shift;
    my $old = $self->{sect}{$sect} &&
              $self->{sect}{$sect}{prop}{$prop} &&
              !$self->{sect}{$sect}{prop}{$prop}{disabled};
    if (@_) {
        my $disabled = shift() ? "" : "# ";
        my $p = $self->_add_property($sect, $prop);
        my $line = $p->{line};
        substr($self->{lines}[$line], $p->{col}, length($p->{disabled})) = $disabled;
        $p->{val_col} += length($disabled) - length($p->{disabled});
        $p->{disabled} = $disabled;
    }
    return $old;
}

1;

__END__

=head1 NAME

ActiveState::Config::INI - Access and edit INI style config files

=head1 SYNOPSIS

 use ActiveState::Config::INI;
 my $conf = ActiveState::Config::INI->new( $filename );
 $conf->property($section, $param => $value);
 $conf->save;

=head1 DESCRIPTION

The C<ActiveState::Config::INI> class allow INI style configuration
files to be accessed and edited safely without rearranging the file or
dropping comments.  The diff to the file will only affect the lines of
properties and sections that has been touched.  The existing line
ending sequence is preserved regardless of platform.

The INI file dialect that this module support is identical to the one
supported by the L<Config::Tiny> module:  Lines that start with '#' as
the first non-whitespace character are ignored (or introduce commented
out parameters).  The ' ; ' sequence can be used for trailing comments
on parameter values.  Quotes have no special meaning in attribute values.

The following methods are provided:

=over

=item $conf = ActiveState::Config::INI->new

=item $conf = ActiveState::Config::INI->new( $filename )

This constructs a new C<ActiveState::Config::INI> object and returns
it.  If a file name is passed in, initialize the object by reading the
file.

=item $conf->read( $filename )

Will initialize the object from the given file.  Croaks if the file
can't be opened.  Any previous state is lost.

No return value.

=item $conf->write

=item $conf->write( $filename )

Write the current state back to the given file.  The filename can be
ommited if the state was previously read from a file.  In this case
the file will be overwritten.

Croaks if the file can't be opened or if write fails.  The file might
have been partly overwritten in this case.

No return value.

=item $conf->content

=item $conf->content( $buffer )

Without argument returns the content that would be written to the file
by C<< $conf->write >>.

With argument initialize the object from the given buffer.  The result
is the same as if C<< $conf->read >> was invoked on a file with
$buffer as its content.

=item $conf->sections

Returns the list of section names used in the file.  The section names
are returned in the same order as found in configuration file.

There will not be an "" entry (the global section) in the returned
list, even though this section can be regarded as always present.

The return value in scalar context is undefined.

=item $conf->have_section( $section )

Returns TRUE if the given section is present.  Disabled sections are
still regarded as present.

=item $conf->add_section( $section, $comment )

Adds the given section to the end of the configuration file.  If a
comment is provided it's added just before the section in the file.
The comment might be multi-lined.

If the section is already present in the configuration file nothing is
done.  There is no check that the leading comment for the section
match the given comment.

=item $conf->section_enabled( $section )

=item $conf->section_enabled( $section => $bool )

Get/set the flag that indicate if a section is enabled or not.  If set
for a section not already present then the section will be
automatically added first.

This uses the perlcritic convension of prepending "-" to the section
name for disabled sections.

=item $conf->delete_section( $section )

Removes all the lines from the given section from the configuration
file.  The removed lines are returned as a list.  In scalar context
the number of deleted lines is returned.

=item $conf->properties( $section )

Returns the list of the names of the properties currently in use for
the given section.  Disabled properties are also included in the
list.

The return value in scalar context is undefined.

=item $conf->property( $section, $param )

=item $conf->property( $section, $param => $value)

=item $conf->property( $section, $param => $value, $enabled)

Get/set the given property value.  When a property is set then the
previous value it had is returned.

If a property value is set it will also become enabled unless $enabled
is passed as FALSE or C<undef>.  If FALSE is passed the property will
be disabled.  If C<undef> is passed then the enabledness of the
property stay as it was.

Use the empty string as $section to get/set the global property
values.

Setting a property to the C<undef> value has the same effect as
disabling it; that is C<< $conf->property_enabled( $section, $param, 0 ) >>.

When setting; if the given section is not present it be added first,
and if the given property does not exist a line for it will be
inserted as the first property of the section.

=item $conf->_property( $section, $param )

Works like C<< $conf->property >> but will even return the current
value for disabled properties, while C<< $conf->property >> returns
C<undef> for these.

=item $conf->have_property( $section, $param )

Returns TRUE if the given property is present in the configuration
file.  Disabled properties are considered present.

=item $conf->property_enabled( $section, $param )

=item $conf->property_enabled( $section, $param => $enabled )

Get/set the flag that indicate if the given property is enabled or not.

In the file format disabled properties are commented out by prefixing
their line with "# ".

=item $conf->append_lines( @lines )

Will append the given lines to the configuration file.  The passed in
strings do not have to be "\n" terminated and they should not have
embedded newlines.

=item $conf->insert_line( $offset, $line )

Inserts the given line into the configuration file at the given
position (0-based line number).

=item $conf->delete_lines( $offset, $length )

Removes the given lines from the configuration file and returns them.

=back

=head1 BUGS

C<Config::Tiny> removes "inline comments" (C<< /\s;\s.*/ >>) before it
tries to recognize section headers or parameters.  This cause an
incompatiblity with this module which only recognize inline comments
in parameter values.

When a new parameter is inserted it's not aligned with the section
header indentation.  Should not really be a big deal as section
headers should not really be indented.

C<Config::Tiny> uses "_" as the name for the global section.  This
module use "".

=head1 SEE ALSO

L<Config::Tiny>
L<http://en.wikipedia.org/wiki/INI_file>
