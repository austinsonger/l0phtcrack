package ActiveState::Menu;

use strict;
our $VERSION = '1.00';

use base 'Exporter';
our @EXPORT_OK = qw(menu prompt yes);

use ActiveState::Prompt qw(prompt yes);
use Term::ANSIColor qw(color);

my %color = (
    '*' => color('bold'),
    '_' => color("underline")
);

sub menu {
    unshift(@_, "menu") if @_ == 1;
    my %cnf = @_;

    my @menu = map ref($_) ? $_ : [$_], @{$cnf{menu}};
    return undef unless @menu;

    if (my $intro = $cnf{intro}) {
	$intro =~ s/([*_])(\w.*\w|\w)\1/$color{$1} . $2 . color("reset")/ge;
	print "\n$intro\n";
    }

    print "\n";

    my %seen;
    my $shortcut = sub {
	my $key = shift;
	return $key if $seen{lc $key}++ || shift;
	return color("underline") . $key . color("reset");
    };

    my @index;
    for (my $i = 0; $i < @menu; $i++) {
	my $text = $menu[$i][0];
	if ($text =~ /^\((.*)\)$/) {
	    #$text = $1;
	    $text =~ s/&(.)/&$shortcut($1, 1)/e;
	    $text = color("yellow") . $text . color("reset");
	}
	elsif ($text =~ /^&(.)$/) {
	    # invisible entry
	    &$shortcut($1);  # register it
	    next;
	}
	elsif ($text =~ /^---+$/) {
	    print "\n";  # separator
	    next;
	}
	else {
	    $text =~ s/&(.)/&$shortcut($1)/e;
	}
	push(@index, $i);
	printf "%3d. %s", int(@index), $text;
	print "\n";
    }
    print "\n";

    my $prompt = $cnf{prompt} || "Please select an item:";

    while (1) {
	my $ans = prompt($prompt);
	if ($ans =~ /^\s*\d+\s*$/) {
	    if ($ans < 1 || $ans > @index) {
		print "\n    Please select a number in the range 1..", scalar(@index), "\n\n";
		next;
	    }
	    $ans = $index[$ans-1];
	}
	elsif ($ans) {
	    my $n = 0;
	    my $re = qr/&\Q$ans/i;
	    undef($ans);
	    for (@menu) {
		$ans = $n, last if $_->[0] =~ $re;
		$n++;
	    }
	    unless (defined $ans) {
		print "\n    Please select one of the menu letters\n\n";
		next;
	    }
	}
	elsif (!$cnf{force}) {
	    return undef;
	}
	else {
	    next;
	}

	if ($menu[$ans][0] =~ /^\(/ && !$cnf{disabled_selectable}) {
	    # disabled
	    if ($cnf{force}) {
		print "\n    This item has been disabled.  Please select another one.\n\n";
		next;
	    }
	}
	elsif (ref($menu[$ans][1]) eq "HASH") {
	    menu(%{$menu[$ans][1]});
	}
	elsif (ref($menu[$ans][1]) eq "CODE") {
	    my $action = $menu[$ans][1];
	    local $_ = $menu[$ans][0];
	    &$action();
	}
	if (exists $cnf{loop_until}) {
	    my $exits = $cnf{loop_until};
	    if (ref($exits) eq "ARRAY"
	        ? !grep($ans == $_, @$exits)
		: $ans != $exits)
	    {
		$ans = menu(%cnf);
	    }
	}
	return $ans;
    }
}

1;

__END__

=head1 NAME

ActiveState::Menu - Present a menu

=head1 SYNOPSIS

 use ActiveState::Menu qw(menu prompt yes);

 my $sel = menu([qw(Foo Bar Baz)]);


 menu(intro  => "M E N U",
      menu   => [["&Foo", \&do_foo],
                 ["S&ub", {
		     intro      => "S U B  M E N U",
		     menu       => [qw(Apples Oranges Exit)],
		     loop_until => 2,
		 }],
                 ["Ba&r", \&do_bar],
                 "-----",
                 ["&h", \&do_help],
                 ["(Ba&z)", \&do_baz]],
      prompt => "What (type 'h' for help)?",
      force  => 1,
     );

  my $ans = (prompt("What is your favourite colour?", "blue"));

  if (yes("Do you really want to quit?")) {
      print "Bye\n";
      exit;
  }

=head1 DESCRIPTION

This module provide the following functions:

=over 4

=item menu( %opts )

This function will present a menu on the terminal.  The return value
is the index of menu item selected (or C<undef> if no item was selected).

The following options are recognised:

   menu:    something to select from (array ref)
   intro:   heading text
   prompt:  prompt text
   force:   force something to be selected (bool)
   disabled_selectable: allow disabled items to be selected
   loop_until: show menu in a loop until a particular item is selected

The items of the C<menu> array can either be plain text strings or an
array reference containing a string and a function reference.

If a function is provided, it will be invoked if the item is selected.
The value of $_ is set to the menu text before the function is called,
so the same function can serve multiple entries and change it
behaviour based on what it finds in $_.

If the menu text contains the letter "&" then the next letter is taken
as a selector.  The menu item can be selected by typing this selector.
The "&" character itself is not rendered.

If the menu text is enclosed in parenthesis, then that selection is
shown as disabled.  Any action associated with the item is not
triggered if it is selected.

If the menu text only consist of the letter "&" followed by a
selector char, then the given menu item is invisible, but the
associated action could still be triggered.  This can be used for
hidden commands or help behaviour as demonstrated in the synopsis
above.

If the menu text consist of a line of at least 3 dashes, then it is
taken as a separator.  An action is associated with a separator line
is always ignored.

If only a single argument is given to the menu() function, it is taken
to be the same as the C<menu> option.

The C<intro> text is presented as a heading above the menu.  Simple
markup like *bold*and _underline_ can be used for visual effect.  If
not provided, no heading is used.

The C<prompt> option replace the default "Please select an item?" text.

If the C<force> option has a TRUE value then it prevents the menu()
function from returning undef (if the user did not select any item).
It also prevents the index of any disabled items from being returned.
The option C<disabled_selectable> can be used to counter that
behaviour.

The loop_until option, if specified, makes the menu run in a loop
until one of the indices specified by the option is selected.  The
value of the option should be either a single index, or a reference
to an array of indices.

=item prompt( $question, $default )

This function will ask a question on the terminal and return the
answer given.  If the program is not running on a terminal then the
$default is returned.

This function comes from the C<ActiveState::Prompt> module.  Refer to
the documentation of that module for the complete story.

=item yes( $question, $default )

Will use the prompt function to ask a question and then return a TRUE
value if the answer was "yes".  If no $default is given it defaults to
"no".

This function also comes from the C<ActiveState::Prompt> module.

=head1 SEE ALSO

L<ActiveState::Prompt>.

=cut

