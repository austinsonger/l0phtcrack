package ActiveState::Prompt;

use strict;

our $VERSION = '1.01';

use base 'Exporter';
our @EXPORT_OK = qw(prompt yes enter);

my $isa_tty = -t STDIN && (-t STDOUT || !(-f STDOUT || -c STDOUT));

our $SILENT;
our $USE_DEFAULT;
unless (defined $USE_DEFAULT) {
    # The PERL_MM_USE_DEFAULT is for compatibility with
    # ExtUtils::MakeMaker's prompt function.
    $USE_DEFAULT = !$isa_tty || $ENV{PERL_MM_USE_DEFAULT};
}

my $RE_yes_or_no = qr/^(y(es)?|no?)$/i;

sub prompt {
    die "prompt function called without an argument" unless @_;
    my $mess = shift;
    unshift(@_, "default") if @_ == 1;
    my %opt = @_;

    my $default = $opt{default};
    my $has_default = defined $default;
    $default = "" unless $has_default;
    my $dispdef = $has_default ? " [$default] " : " ";

    return $default if exists $opt{silent} ? $opt{silent} : $SILENT;

    local $| = 1;
    local $\;
    my $count;
ASK:
    while (1) {
	$count++;
	if ($count > 25) {
	    die "No valid answer given after $count tries, stopped";
	}
	elsif ($count == 2 || $count > 5) {
	    my $msg = "";
	    if ($opt{no_match_msg}) {
		$msg .= $opt{no_match_msg};
		$msg .= "." unless $msg =~ /[.!?]$/;
		$msg .= " ";
	    }
	    $msg .= "Please try again!";
	    if (length($msg) > 60) {
		require Text::Wrap;
		$Text::Wrap::columns = 60;  # consider actual terminal width
		$msg = Text::Wrap::wrap("", "", $msg);
	    }
	    $msg =~ s/^/    /mg;
	    print "\n$msg\n\n";
	}
	print "$mess$dispdef";

	if (exists $opt{use_default} ? $opt{use_default} : $USE_DEFAULT) {
	    print "$default\n";
	    return $default;
	}

	my $ans;
	
	# turn echo off
	if ( $opt{noecho} )
	{
	    my $fd_stdin = fileno(STDIN);

	    # grab POSIX at runtime
	    sub ECHO; sub ECHOK; sub ICANON; sub VTIME; sub TCSANOW;
	    require POSIX;
	    POSIX->import(":termios_h");

	    my $term     = POSIX::Termios->new();

	    # Store current term settings, and turn echo off
	    $term->getattr($fd_stdin);
	    my $oterm    = $term->getlflag();
	    my $echo     = ECHO | ECHOK | ICANON;
	    my $noecho   = $oterm & ~$echo;
	    $term->setlflag($noecho);
	    $term->setcc(VTIME, 1);
	    $term->setattr($fd_stdin, TCSANOW);

	    $ans = <STDIN>;

	    # Restore previous term settings
	    $term->setlflag($oterm);
	    $term->setcc(VTIME,0);
	    $term->setattr($fd_stdin, TCSANOW);

	    # We aren't echoing the user's return key, so print one
	    # to ensure the next question is on a new line
	    print "\n";
	}
	else # read normally
	{
	    $ans = <STDIN>;
	}
	
	unless (defined $ans) {
	    # Ctrl-D
	    print "\n";
	    $ans = "";
	}
	else {
	    chomp($ans);
	}

	return $default if $has_default && $ans eq "";

	if ($opt{trim_space}) {
	    $ans =~ s/^\s+//;
	    $ans =~ s/\s+$//;
	    $ans =~ s/\s+/ /g;
	}

	if (my $mm = $opt{must_match}) {
	    if (ref($mm) eq "CODE") {
		redo ASK unless &$mm($ans);
	    }
	    elsif (ref($mm) eq "ARRAY") {
		unless (grep $_ eq $ans, @$mm) {
		    unless ($opt{no_match_msg}) {
			my @v = @$mm;
			my $last = pop @v;
			$opt{no_match_msg} = "Valid answers are " .
			                      join(" or ", join(", ", @v), $last);
		    }
		    redo ASK;
		}
	    }
	    else {
		# assume regexp
		redo ASK unless $ans =~ $mm;
	    }
	}

	return $ans;
    }

}

sub yes {
    my $prompt = shift;
    unshift(@_, "default") if @_ == 1;
    my %opt = @_;

    unless ($opt{default} && $opt{default} =~ $RE_yes_or_no) {
	$opt{default} = $opt{default} ? "yes" : "no";
    }
    $opt{trim_space} = 1 unless exists $opt{trim_space};
    unless (exists $opt{must_match}) {
	$opt{must_match} = $RE_yes_or_no;
	$opt{no_match_msg} = "The answer must be 'yes' or 'no'";
    }
    return prompt($prompt, %opt) =~ /^y/i;
}

sub enter {
    prompt(shift || "Press ENTER to continue...", @_);
}

1;

=head1 NAME

ActiveState::Prompt - Interactive questions

=head1 SYNOPSIS

 use ActiveState::prompt qw(prompt yes enter);

=head1 DESCRIPTION

The following functions are provided:

=over 4

=item prompt( $question )

=item prompt( $question, $default )

=item prompt( $question, %opts )

This function will ask a question on the terminal and return the
answer given.  The return value will always be defined.

Options can be passed in as key/value pairs.  The following options
are recognized:

  default
  trim_space
  must_match
  no_match_msg
  use_default
  silent

The C<default> is returned if the user simply press return.  The
default value is shown in brackets.  If no default is provided then no
brackets are added to the question.

If C<trim_space> is TRUE, then any leading and trailing space in the
anwer is trimmed off and any internal space is collapsed to a single
space.

The C<must_match> value is used to validate answers.  It can be either
a function, array or regular expression.  If it is a function, then
the function is called with the answer given and should return TRUE if
it is to be accepted.  If it is an array, then the answer must be one
of the values in the array.  If it is an regular expression then it
must match it.

The C<no_match_msg> is printed if the given answer does not validate
as specified in C<must_match>.

If C<use_default> is TRUE, then no answer is read from the terminal
and the default is returned (or "" if there is no default).  The
C<use_default> is by default TRUE if the program is not running on a
terminal or if the PERL_MM_USE_DEFAULT environment variable is TRUE.

The C<silent> option works like C<use_default> but it also prevent the
prompt text from being printed.  This forces prompt() to return the default
without actually prompting.

=item yes( $question )

=item yes( $question, $default )

Will use the prompt function to ask a question and then return a TRUE
value if the answer was "yes" or "y".

The $default can either be a string like "yes", "y", "no" or "n", or a
boolean value in which case "yes" will be the default if TRUE and "no"
otherwise.  If no $default is given it defaults to "no".

=item enter()

Ask the use to press some key before the function returns.

=back

=head1 BUGS

None.

=cut
