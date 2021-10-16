package ActivePerl::PPM::ParsePPD;

use strict;

require XML::Parser::Expat;
our @ISA = qw(XML::Parser::ExpatNB);

my %FEATURE_TAG = (
   REQUIRE => "require",
   PROVIDE => "provide",
   DEPENDENCY => "require",
);

my %TEXT_TAG = (
   ABSTRACT => 1,
   AUTHOR => 1,
);

my %INSTALL_TAG = (
   INSTALL => 1,
   UNINSTALL => 1,
);

my %IGNORE_TAG = (
   TITLE => 1,
   OS => 1,
   OSVERSION => 1,
   PROCESSOR => 1,
   PERLCORE => 1,
);

sub new {
    my $class = shift;
    my $handler = shift;
    my $self = $class->SUPER::new;

    $self->{softpkg} = {};

    my $ch = sub {
        $_[0]->{txt} .= $_[1];
    };

    $self->setHandlers(
	Start => sub {
	    my $p = shift;
	    my $tag = shift;

	    if ($FEATURE_TAG{$tag}) {
                if ($_[0] eq "NAME" && (@_ == 2 || $_[2] eq "VERSION")) {
                    # the fast way to pick out the attributes
                    $_[3] = 0 if $tag eq "DEPENDENCY";
                    $p->{ctx}{$FEATURE_TAG{$tag}}{$_[1]} = $_[3] || 0;
                }
                else {
                    # the slower way to do it
                    my %attr = @_;
                    $attr{VERSION} = 0 if $tag eq "DEPENDENCY";
                    $p->{ctx}{$FEATURE_TAG{$tag}}{$attr{NAME}} = $attr{VERSION} || 0;
                }
	    }
	    elsif ($TEXT_TAG{$tag}) {
                $p->{txt} = "";
		$p->setHandlers(Char => $ch);
	    }
	    elsif ($tag eq "SOFTPKG") {
		my @c = $p->context;
		$p->xpcroak("$tag must be root") if @c && "@c" !~ /^REPOSITORY(SUMMARY)?$/;
		my %attr = @_;
		$p->xpcroak("Required SOFTPKG attribute NAME and VERSION missing")
		    unless exists $attr{NAME} && exists $attr{VERSION};

		%{$p->{softpkg}} = ( name => $attr{NAME}, version => $attr{VERSION}, release_date => $attr{DATE} );
		$p->{softpkg}{base} = $p->{base} if $p->{base};

		$p->{ctx} = $p->{softpkg};
	    }
	    elsif ($tag eq "IMPLEMENTATION") {
		$p->{ctx} = {};
		push(@{$p->{softpkg}{lc $tag}}, $p->{ctx});
	    }
	    elsif ($tag eq "ARCHITECTURE") {
		my %attr = @_;
		$p->{ctx}{lc $tag} = $attr{NAME};
	    }
	    elsif ($tag eq "CODEBASE") {
		my %attr = @_;
		$p->{ctx}{lc $tag} = $attr{HREF};
	    }
	    elsif ($INSTALL_TAG{$tag}) {
		my %attr = @_;
		my $h = $p->{ctx}{script}{lc $tag} = {};
		$h->{exec} = $attr{EXEC} if exists $attr{EXEC};
		$h->{uri} = $attr{HREF} if exists $attr{HREF};
                $p->{txt} = "";
		$p->setHandlers(Char => $ch);
	    }
	    elsif ($tag =~ /^REPOSITORY(SUMMARY)?$/) {
		$p->xpcroak("$tag must be root") if $p->depth;
		my %attr = @_;
		$p->{base} = $attr{BASE};
		$p->{arch} = $attr{ARCHITECTURE};
	    }
	    elsif ($IGNORE_TAG{$tag}) {
		# ignore
	    }
	    else {
		$p->xpcroak("Unrecognized PPD tag $tag");
	    }
	},
	End => sub {
	    my($p, $tag) = @_;
	    if ($TEXT_TAG{$tag}) {
		$p->{ctx}{lc $tag} = $p->{txt};
                $p->setHandlers(Char => undef);
	    }
	    elsif ($tag eq "SOFTPKG") {
		my $pkg = $p->{softpkg};
		if (exists $pkg->{codebase}) {
		    $pkg->{architecture} ||= $p->{arch} || "noarch";
		}
		$handler->($pkg);
	    }
	    elsif ($tag eq "IMPLEMENTATION") {
		$p->{ctx}{architecture} ||= $p->{arch} || "noarch";
		$p->{ctx} = $p->{softpkg};
	    }
	    elsif ($INSTALL_TAG{$tag}) {
		my $h = $p->{ctx}{script}{lc $tag};
		$h->{text} = $p->{txt}
		    unless defined($h->{uri}); # SCRIPT/HREF is preferred
                $p->setHandlers(Char => undef);
	    }
	},
    );

    return $self;
}

1;
