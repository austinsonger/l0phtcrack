package ActiveState::PerlCritic::UserProfile::Policy;

use strict;
use ActiveState::PerlCritic::Policy;

use Carp qw(croak);

sub new {
    my($class, $name, $config) = @_;
    my $self = bless { name => $name, config => $config }, $class;
    $self->{p} = ActiveState::PerlCritic::Policy::policy($name);
    return $self;
}

sub name {
    my $self = shift;
    return $self->{name};
}

*short_name = \&name;

sub config_name {
    my $self = shift;
    $self->{config_name} ||= do {
        my $c = $self->{config};
        my $n = $self->name;
        my $f = "Perl::Critic::Policy::$n";
        $c->have_section($n) ? $n : $c->have_section($f) ? $f : $n;
    };
}

sub state {
    my $self = shift;
    my $c = $self->{config};
    my $n = $self->config_name;
    my $old = $self->{deleted} || !$c->have_section($n) ? "unconfigured" :
        $c->section_enabled($n) ? "enabled" : "disabled";
    if (@_) {
        my $state = shift;
        if ($state ne $old) {
            if ($state eq "unconfigured") {
                $self->{deleted}++ if $c->have_section($n);
            }
            else {
                croak("Can't set state to '$state'")
                    unless $state eq "enabled" || $state eq "disabled";
                $c->section_enabled($n, $state eq "enabled");
                delete $self->{deleted};
            }
        }
    }
    return $old;
}

sub severity {
    my $self = shift;
    my $n = $self->config_name;
    my $c = $self->{config};
    my $old = $c->property($n, "severity");
    $old = $self->{p}{default_severity} unless defined $old;
    if (@_) {
        $c->property($n, "severity" => shift);
    }
    return $old;
}

sub themes {
    my $self = shift;
    my $n = $self->config_name;
    my $c = $self->{config};
    my $old = $c->property($n, "set_themes");
    $old = join(" ", @{$self->{p}{default_themes}}) unless defined($old);
    if (defined(my $add = $c->property($n, "add_themes"))) {
        $old .= " $add";
        $old =~ s/^\s+//;
    }
    $old =~ s/\s+/ /g;

    if (@_) {
        my $new = shift;

        # figure out if we should use set_themes or add_themes

        my %default = map { $_ => 1 } @{$self->{p}{default_themes}};
        my %new     = map { $_ => 1 } split(" ", $new);
        for (keys %new) {
            delete $default{$_} && delete $new{$_};
        }
        if (%default) {
            # trying to remove themes from the default set, need to use set_themes
            $c->property($n, "set_themes", $new);
            $c->property_enabled($n, "add_themes", 0) if $c->property_enabled($n, "add_themes");
        }
        else {
            if (%new) {
                # only adding to the default set
                $c->property($n, "add_themes", join(" ", sort keys %new));
            }
            else {
                # set to the default set
                $c->property_enabled($n, "add_themes", 0) if $c->property_enabled($n, "add_themes");
            }
            $c->property_enabled($n, "set_themes", 0) if $c->property_enabled($n, "set_themes");
        }
    }
    return $old
}

sub param {
    my $self = shift;
    my $param = shift;

    return $self->severity(@_) if $param eq "severity";
    return $self->themes(@_) if $param eq "themes";

    my $n = $self->config_name;
    my $c = $self->{config};
    my $old = $c->property($n, $param);
    unless (defined $old) {
        if ($self->{p}{parameters}) {
            for my $p (@{$self->{p}{parameters}}) {
                if ($p->{name} eq $param) {
                    $old = $p->{default_string};
                    last;
                }
            }
        }
    }
    if (@_) {
        $c->property($n, $param => shift);
    }
    return $old;
}

1;
