package ActiveState::Tkx::Widget;

# Our own widget class so that we can override a few methods
# for convenience

use strict;
use base qw(Tkx::widget);

sub messageBox {
    my $self = shift;
    Tkx::tk___messageBox(-parent => $self, @_);
}

sub getOpenFile {
    my $self = shift;
    Tkx::tk___getOpenFile(-parent => $self, @_);
}

sub getSaveFile {
    my $self = shift;
    Tkx::tk___getSaveFile(-parent => $self, @_);
}

sub chooseDirectory {
    my $self = shift;
    Tkx::tk___chooseDirectory(-parent => $self, @_);
}

sub bell {
    my $self = shift;
    Tkx::bell(-displayof => $self, @_);
}

sub children {
    my $self = shift;
    Tkx::SplitList($self->g_winfo_children);
}

sub rstate_disabled {
    my $self = shift;
    return if $self->m_instate("disabled");
    my $d = $self->_data;
    my $walk;
    $walk = sub {
        my $w = shift;
        if (eval{ $w->m_instate("disabled") } || $@) {
            # we also set "disabled" for widgets that do not implement
            # the 'instate' method, because that prevents us from calling
            # the 'state' method on them in 'rstate_normal'.
            $d->{disabled}{$w}++;
        }
        else {
            $w->m_state("disabled");
        }
        for my $k ($w->children) {
            $walk->($w->_nclass->new($k));
        }
    };
    $walk->($self);
}

sub rstate_normal {
    my $self = shift;
    return unless $self->m_instate("disabled");
    my $d = $self->_data;
    my $walk;
    $walk = sub {
        my $w = shift;
        unless (delete $d->{disabled}{$w}) {
            $w->m_state("!disabled");
        }
        for my $k ($w->children) {
            $walk->($w->_nclass->new($k));
        }
    };
    $walk->($self);
}

sub pack_scrolled {
    my($self, $widget, %opt) = @_;

    if (defined(my $bd = delete $opt{-bd})) {
	$opt{-borderwidth} = $bd;
    }

    my %sw_opt = (-borderwidth => 0);
    for (qw(-borderwidth -relief)) {
	next unless exists $opt{$_};
	$sw_opt{$_} = delete $opt{$_};
    }
    if (my $bg = ($opt{-bg} || $opt{-background})) {
	$sw_opt{-background} = $bg;
    }

    my %pack_opt = (-fill => "both", -expand => 1);
    for (qw(-side -fill -expand)) {
	next unless exists $opt{$_};
	$pack_opt{$_} = delete $opt{$_};
    }

    my $sw = $self->new_ScrolledWindow(
        -managed => 0,
	%sw_opt,
    );
    $widget = "new_$widget";
    $opt{-borderwidth} = 0 unless $widget eq "new_ScrollableFrame";
    my $w = $sw->$widget(
        %opt,
    );
    $sw->setwidget($w);
    $sw->g_pack(%pack_opt);

    if ($widget eq "new_ScrollableFrame") {
	$w = ref($w)->new($w->getframe);
    }

    return $w;
}

sub _nclass {
    return __PACKAGE__;
}

1;
