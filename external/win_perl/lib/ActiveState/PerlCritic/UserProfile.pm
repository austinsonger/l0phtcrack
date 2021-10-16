package ActiveState::PerlCritic::UserProfile;

use strict;

use ActiveState::PerlCritic::Policy;
use ActiveState::PerlCritic::UserProfile::Policy;
use ActiveState::Config::INI;
use File::Spec::Functions qw(catfile);

sub new {
    my $class = shift;
    my $self = bless {}, $class;
    if (@_) {
        my $filename = shift;
        $self->{config} = ActiveState::Config::INI->new(-e $filename ? $filename : ());
        $self->{filename} = $filename;
    }
    else {
        $self->{config} = ActiveState::Config::INI->new;
    }
    return $self;
}

sub new_default {
    my($class) = @_;
    return $class->new($ENV{PERLCRITIC}) if exists $ENV{PERLCRITIC};
    require File::HomeDir;
    my $home = File::HomeDir->my_home;
    return $class->new(catfile($home, ".perlcriticrc"));
}

sub filename {
    my $self = shift;
    return $self->{filename};
}

sub dirname {
    my $self = shift;
    my $f = $self->{filename};
    return undef unless defined($f);
    require File::Basename;
    return File::Basename::dirname($f);
}

sub _drop_deleted {
    my $self = shift;
    my $config = shift || $self->{config};

    for (values %{$self->{policy}}) {
        if ($_->{deleted}) {
            $config->delete_section($_->config_name);
        }
	delete $_->{deleted} if $config == $self->{config};
    }
}

sub save {
    my $self = shift;
    my $filename = @_ ? ($self->{filename} = shift) : $self->{filename};
    $self->_drop_deleted;
    $self->{config}->write($filename);
}

sub content {
    my $self = shift;

    my $deleted;
    for (values %{$self->{policy}}) {
	$deleted++, last if $_->{deleted};
    }

    return $self->{config}->content unless $deleted;

    # need to actually delete sections
    require Storable;
    my $conf_clone = Storable::dclone($self->{config});
    $self->_drop_deleted($conf_clone);
    return $conf_clone->content;
}

sub revert {
    my $self = shift;
    return $self->clear() unless -e $self->filename;
    $self->{config}->read($self->filename);
}

sub clear {
    my $self = shift;
    return $self->{config}->content("") ne "";
}

sub param {
    my $self = shift;
    return $self->{config}->property("", @_);
}

sub policies {
    my $self = shift;
    map $_->{short_name}, ActiveState::PerlCritic::Policy::policies();
}

sub policy {
    my($self, $name) = @_;
    return $self->{policy}{$name} ||= ActiveState::PerlCritic::UserProfile::Policy->new($name, $self->{config});
}

sub dump {
    my $self = shift;
    require Data::Dump;
    return Data::Dump::dump($self->{config});
}

1;

__END__

=head1 NAME

ActiveState::PerlCritic::UserProfile - Edit a perlcriticrc file

=head1 SYNOPSIS

  my $profile = ActiveState::PerlCritic::UserProfile->new( $filename );

  my $policy = $profile->policy("RegularExpressions::RequireExtendedFormatting");
  $policy->state("enabled");
  $policy->severity(2);
  $policy->param("foo" => 42);

  $profile->save( $filename );

=head1 DESCRIPTION

C<ActiveState::PerlCritic::UserProfile> objects holds a F<perlcriticrc> file where policy state
and parameters can be queried/modified and the whole configuration
file written back to disk.

The following methods are provided:

=over

=item $profile = ActiveState::PerlCritic::UserProfile->new

=item $profile = ActiveState::PerlCritic::UserProfile->new( $filename )

Creates a new profile object and optinally initialize its state from
the given filename.  If a filename is passed it's also saved so that
the calling the save method without a filename saves back to the same
file.

=item $profile = ActiveState::PerlCritic::UserProfile->new_default

Open up the user default perlcriticrc file; usually found at
F<~/.perlcriticrc>.  The file name is saved so that invoking the save
method without a filename saves the state back to the file.

=item $profile->save

=item $profile->save( $filename )

Write the current state of the userprofile object back to the given
file.  If no filename is given try to save back to the filename that
the profile object was initialized from.

=item $profile->filename

Returns the filename that the state was initialized from or last saved
to.

=item $profile->dirname

Returns the name of the directory where the profile file resides.

=item $profile->content

Returns the content that would be written if the profile had been saved now.

=item $profile->revert

Revert to the stored version of the configuration file.

=item $profile->clear

Empty the configuration file.

=item $profile->param( $name )

=item $profile->param( $name => $new_svalue )

Gets or sets the specified global parameter value

=item $profile->policies

Lists all the policies (both configured or unconfigured).

=item $profile->policy( $name )

Look up the given policy object.  The returned object provide the following methods:

=over

=item $policy->name

Returns the name of the policy; it's a string like "RegularExpressions::RequireExtendedFormatting".

=item $policy->config_name

Returns the name used in the configuration file.  This will often be
the same as C<< $policy->name >>, but not always.  There should not
really be a reason to expose this name to users.

=item $policy->state

=item $policy->state( $new_state )

Gets or sets the state of the policy.  The state is one of the following values:

  unconfigured
  enabled
  disabled

=item $policy->severity

=item $policy->severity( $int )

Gets or sets the severity for the policy.  It's a number in the range 1 to 5.

=item $policy->param( $name )

=item $policy->param( $name => $new_value )

Gets or sets policy specific parameter values

=back

=back

=head1 SEE ALSO

L<Perl::Critic>, L<ActiveState::Config::INI>

=head1 COPYRIGHT

Copyright (C) 2010 ActiveState Software Inc.  All rights reserved.

=cut
