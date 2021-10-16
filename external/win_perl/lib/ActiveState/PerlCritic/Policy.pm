package ActiveState::PerlCritic::Policy;

use strict;
use Perl::Critic;
use Perl::Critic::Utils qw($EMPTY $SEVERITY_LOWEST);

use base 'Exporter';
our @EXPORT_OK = qw(policies);

my $plist;
my $phash;

sub _get_policies {
    my @p;
    my $c = Perl::Critic->new(-profile => $EMPTY, -severity => $SEVERITY_LOWEST);
    for my $policy ($c->policies) {
	my $name = $policy->get_short_name;

	my %h = (
	   short_name => $name,
	   themes => [$policy->get_themes],
	   default_themes => [$policy->default_themes],
	   severity => $policy->get_severity,
	   default_severity => $policy->default_severity,
	   abstract => $policy->get_abstract,
	);

	my $mod = "Perl::Critic::Policy::$name";
	(my $fname = $mod) =~ s,::,/,g;
	$fname .= ".pm";
	$h{fname} = $INC{$fname} if $INC{$fname};

	if ($policy->parameter_metadata_available) {
	    for my $param (@{$policy->get_parameters || []}) {
		my %param = (
		    name => $param->get_name,
		    default_string => $param->get_default_string,
		    description => $param->get_description,
		);
		push(@{$h{parameters}}, \%param);
	    }
	}
	push(@p, \%h);
    }
    $plist = \@p;
}

sub policies {
    unless ($plist) {
	eval {
	    _get_policies();
	};
	if ($@) {
	    die "Failed to obtain list of polices from perlcritic-dump\n$@";
	}
    }
    return @$plist;
}

sub policy {
    my $name = shift;
    unless ($phash) {
        for my $p (policies()) {
            $phash->{$p->{short_name}} = $p;
        }
    }
    return $phash->{$name};
}

1;
