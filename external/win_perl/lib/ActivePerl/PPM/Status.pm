package ActivePerl::PPM::Status;

use strict;
use Carp qw(croak);

sub new {
    my $class = shift;
    bless {
        activity => [],
    }, $class;
}

sub begin {
    my $self = shift;
    croak "missing argumet" unless @_;
    my $what = shift;
    push(@{$self->{activity}}, $what);
    return ActivePerl::PPM::Status::Activity->new($self)
	if defined wantarray;
}

sub depth {
    my $self = shift;
    return scalar(@{$self->{activity}});
}

sub activity {
    my $self = shift;
    return $self->{activity}[-1];
}

sub end {
    my $self = shift;
    croak "no activity to end" unless @{$self->{activity}};
    return pop(@{$self->{activity}});
}

sub tick {
    my $self = shift;
    croak "no current activity" unless @{$self->{activity}};
    return;
}

sub reset {
    my $self = shift;
    $self->end while @{$self->{activity}};
    return;
}

package ActivePerl::PPM::Status::Activity;

sub new {
    my $class = shift;
    my $status = shift;
    bless \$status, $class;
}

sub name {
    my $self = shift;
    $$self->activity;
}

sub tick {
    my $self = shift;
    $$self->tick(@_);
}

sub end {
    my $self = shift;
    $$self->end(@_);
    $self->forget;
}

sub forget {
    my $self = shift;
    $$self = undef;
}

sub DESTROY {
    my $self = shift;
    $self->end if $$self;
}

1;
