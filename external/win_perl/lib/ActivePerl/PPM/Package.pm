package ActivePerl::PPM::Package;

use strict;
use Carp qw(croak);
use ActiveState::Version qw(vcmp);
use ActivePerl::PPM::Logger qw(ppm_status ppm_log);

sub BASE_FIELDS {
    return (
       [id       => "integer primary key"],
       [name     => "text not null"],
       [version  => "text"],
       [release_date => "datetime"],
       [author   => "text"],
       [abstract => "text"],
       [ppd_uri  => "text"],
       [ppd_etag => "text"],
       [ppd_lastmod => "text"],
       [codebase => "text"],
    );
}

our $COMPARE_FEATURES = 1;
$COMPARE_FEATURES = $ENV{ACTIVEPERL_PPM_COMPARE_FEATURES} if exists $ENV{ACTIVEPERL_PPM_COMPARE_FEATURES};

#
# constructors
#

sub new {
    my $class = shift;
    my $self = bless +(@_ == 1 ? shift : do{ my %hash = @_; \%hash }), $class;
    croak("No name given for package") unless $self->{name};
    $self->{provide}{$self->{name}} ||= 0;  # always provide ourself
    $self;
}

sub clone {
    my $self = shift;
    require Storable;
    my $other = Storable::dclone($self);
    delete $other->{id};
    return $other;
}

#
# accessors
#

sub AUTOLOAD
{
    our $AUTOLOAD;
    my $method = substr($AUTOLOAD, rindex($AUTOLOAD, '::')+2);
    return if $method eq "DESTROY";

    my $self = shift;
    unless (grep $_->[0] eq $method, $self->BASE_FIELDS) {
	croak(qq(Can't locate object method "$method" via package ) . (ref($self) || $self));
    }
    my $old = $self->{$method};
    if (@_) {
	$self->{$method} = shift;
    }
    return $old;
}

sub name_version {
    my $self = shift;
    my $tmp = $self->{name};
    if (my $v = $self->{version}) {
	$tmp .= "-$v";
    }
    return $tmp;
}

sub provides {
    my $self = shift;
    return %{$self->{provide}};
}

sub requires {
    my $self = shift;
    return %{$self->{require} || {}};
}

sub features_declared {
    my $self = shift;
    my $p = $self->{provide};
    return keys(%$p) > 1 || $p->{$self->{name}};
}

sub codebase_abs {
    my $self = shift;
    return unless $self->{codebase};
    return URI->new_abs($self->{codebase}, $self->{ppd_uri});
}

#
# comparators
#

sub compare {
    my($a, $b) = @_;

    if ($a->{name} eq $b->{name}
        && defined($a->{version})
        && defined($b->{version})
        && $a->{version} eq $b->{version}
        && length($a->{version}))
    {
	return 0;
    }

    my $c = undef;
    my $c_mod = undef;

    if ($COMPARE_FEATURES && $a->features_declared && $b->features_declared) {
	# compare the shared features to see if we have a winner
	for my $mod (keys %{$a->{provide}}) {
	    next unless exists $b->{provide}{$mod};
	    my $c2 = abs($a->{provide}{$mod} - $b->{provide}{$mod}) < 1e-7 ? 0 : $a->{provide}{$mod} <=> $b->{provide}{$mod};
	    $c = 0 unless defined $c;
	    next if $c2 == 0;
	    if ($c) {
		unless ($c == $c2) {
		    ppm_log("ERR", "Version compare conflict for package $a->{name}; $c_mod ($a->{provide}{$c_mod} <=> $b->{provide}{$c_mod}) and $mod ($a->{provide}{$mod} <=> $b->{provide}{$mod}) have inconsistent ordering");
		    return undef;
		}
	    }
	    else {
		$c_mod = $mod if !$c && $c2;
		$c = $c2;
	    }
	}

	if (defined($c) && $c == 0) {
	    # if the shared features compared the same, break the tie
	    # by selecting the package with more features.
	    $c = (keys %{$a->{provide}} <=> keys %{$b->{provide}});
	}
    }

    if (!$c && $a->{name} eq $b->{name}) {
	# last resort is heuristic comparison of version labels and release dates
	$c = vcmp($a->{version}, $b->{version});
    }

    return $c;
}

sub better_than {
    my($self, $other) = @_;
    my $c = compare($self, $other);
    unless (defined($c)) {
	croak("No ordering between package " .
	      $self->name_version . " and " . $other->name_version);
    }
    return $c > 0;
}

sub best {
    my $best = shift;
    my @dunno;
    for my $p (@_) {
        my $c = compare($best, $p);
        if (defined $c) {
            $best = $p if $c < 0;
        }
        else {
            push(@dunno, $p);
        }
    }
    die "Can't determine best" if @dunno;  # XXX can we do better

    return $best;
}

#
# SQL storage
#

sub sql_create_tables {
    my($class, %opt) = @_;
    my @fields = $class->BASE_FIELDS;
    if ($opt{name_unique}) {
	my($name) = grep $_->[0] eq "name", @fields;
	$name->[1] .= " unique";
    }
    my @sql = (
"CREATE TABLE IF NOT EXISTS package (\n    " .
    join(",\n    ", map join(" ", @$_), @fields) .
"
)",
"CREATE TABLE IF NOT EXISTS feature (
     package_id integer not null,
     role char(1) not null,  /* 'p' or 'r' */
     name text not null,
     version double not null
)",
"CREATE INDEX IF NOT EXISTS feature_package_id ON feature(package_id)",
"CREATE INDEX IF NOT EXISTS feature_name_role ON feature(name,role)",
"CREATE TABLE IF NOT EXISTS script (
     package_id integer not null,
     role text not null, /* 'install' or 'uninstall' */
     exec text, /* interpreter */
     uri text,
     text text
)",
    );
    @sql = grep /^CREATE INDEX/, @sql
	if $opt{indexes_only};
    return @sql;
}

my %ROLE = (
    'p' => 'provide',
    'r' => 'require',
);

sub new_dbi {
    my($class, $dbh, $id_or_name, $version) = @_;

    my @bind = ($id_or_name);
    my $where;
    if ($id_or_name =~ /^\d+$/) {
        $where = "id = ?"
    } else {
        $where = "name = ? AND ";
        if (defined $version) {
            $where .= "version = ?";
            push(@bind, $version);
        }
        else {
            $where .= "version ISNULL";
        }
    }

    my $pkg = $dbh->selectrow_hashref("SELECT * FROM package WHERE $where", undef, @bind);
    return undef unless $pkg;

    # fix up potential Unicode fields
    for my $f (qw(author abstract)) {
	if (($pkg->{$f} || "") =~ /[^\x00-\x7F]/) {
	    utf8::decode($pkg->{$f});
	}
    }

    if (1) {
        my $sth = $dbh->prepare("SELECT role, name, version FROM feature WHERE package_id = ?");
        $sth->execute($pkg->{id});
        while (my($role, $feature, $version) = $sth->fetchrow_array) {
            $pkg->{$ROLE{$role}}{$feature} = $version;
        }
    }

    if (1) {
        my $sth = $dbh->prepare("SELECT role, exec, uri, text FROM script WHERE package_id = ?");
        $sth->execute($pkg->{id});
        while (my($role, $exec, $uri, $text) = $sth->fetchrow_array) {
            $pkg->{script}{$role}{exec} = $exec if defined($exec);
            $pkg->{script}{$role}{uri}  = $uri  if defined($uri);
            $pkg->{script}{$role}{text} = $text if defined($text);
        }
    }

    return $class->new($pkg);
}

sub dbi_store {
    my($self, $dbh) = @_;
    my $id = $self->{id};

    my @fields = map $_->[0], $self->BASE_FIELDS;
    shift(@fields); # get rid of id

    if (defined $id) {
	$dbh->do("UPDATE package SET " . join(", ", map "$_ = ?", @fields) . " WHERE id = ?", undef, @{$self}{@fields}, $id);
	$dbh->do("DELETE FROM feature WHERE package_id = ?", undef, $id);
	$dbh->do("DELETE FROM script WHERE package_id = ?", undef, $id);
    }
    else {
	my $sth = $dbh->prepare_cached("INSERT INTO package (" . join(", ", @fields) . ") " .
				           "VALUES(" . join(", ", map "?", @fields) . ")");
	$sth->execute(@{$self}{@fields}) || return undef;
	$id = $dbh->func('last_insert_rowid');
    }

    my $sth_feature_insert = $dbh->prepare_cached("INSERT INTO feature (package_id, role, name, version) VALUES(?, ?, ?, ?)");
    for my $role (values %ROLE) {
	my $hash = $self->{$role} || next;
	while (my($feature, $version) = each %$hash) {
	    $sth_feature_insert->execute($id, substr($role, 0, 1), $feature, $version);
	}
    }

    if (my $script = $self->{script}) {
	for my $role (sort keys %$script) {
	    local $dbh->{PrintError} = 1;
	    my $v = $script->{$role};
	    $dbh->do("INSERT INTO script (package_id, role, exec, uri, text) VALUES(?, ?, ?, ?, ?)", undef,
		     $id, $role, $v->{exec}, $v->{uri}, $v->{text});
	}
    }

    return $id;
}

#
# Script support
#

sub has_script {
    my($self, $kind) = @_;
    return !!$self->{script}{$kind};
}

sub run_script {
    my($self, $kind, $area, $tmpdir, $pkg_info, $run_cb) = @_;

    my $script = $self->{script}{$kind};
    return unless $script;

    $tmpdir ||= do { require File::Temp; File::Temp::tempdir("ppm-XXXXXX", TMPDIR => 1, CLEANUP => 1) };

    my @commands;
    if (defined(my $uri = $script->{uri})) {
	require ActivePerl::PPM::Web;
	my $ua = ActivePerl::PPM::Web::web_ua();
	local $ua->{progress_what} = "Downloading " . $self->name_version . " $kind script";
	my $res = $ua->get(URI->new_abs($uri, $self->{ppd_uri}));
	die $res->status_line unless $res->is_success;
	if (my $len = $res->content_length) {
	    my $save_len = length($res->content);
	    if ($save_len != $len) {
		die "Aborted download ($len bytes expected, got $save_len).\n";
	    }
	}
	if ($script->{exec}) {
	    my $file = "$tmpdir/${kind}_script";
	    open(my $fh, ">:utf8", $file) || die "Can't create $file: $!";
	    print $fh $res->decoded_content;
	    close($fh) || die "Can't write $file: $!";
	    chmod(0755, $file);
	    push(@commands, _expand_exec($script->{exec}, "${kind}_script"));
	}
	else {
	    push(@commands, grep length, split(/\n/, $res->decoded_content));
	}
    }
    else {
	if (my $exec = $script->{exec}) {
	    my $text = $script->{text};
	    $text =~ s/;;/\n/g;  # what ugliness

	    my $file = "$tmpdir/${kind}_script";
	    open(my $fh, ">", $file) || die "Can't create $file: $!";
	    print $fh $text;
	    close($fh) || die "Can't write $file: $!";
	    chmod(0755, $file);

	    push(@commands, _expand_exec($script->{exec}, "${kind}_script"));
	}
	else {
	    push(@commands, grep length, split(/;;/, $script->{text}));
	}
    }
    if (@commands) {
	require Cwd;
	my $old_cwd = Cwd::cwd();
	local $ENV{PPM_INSTROOT} = $area->prefix;
	local $ENV{PPM_INSTLIB} = $area->lib;
	local $ENV{PPM_INSTARCHLIB} = $area->archlib;
	local $ENV{PPM_VERSION} = do { require ActivePerl::PPM; $ActivePerl::PPM::VERSION };
	local $ENV{PPM_ACTION} = $kind;
	local $ENV{PPM_NEW_VERSION} = $pkg_info->{new_version} if exists $pkg_info->{new_version};
	local $ENV{PPM_PREV_VERSION};
	if (exists $pkg_info->{old_version}) {
	    $ENV{PPM_ACTION} = "upgrade" if $ENV{PPM_ACTION} eq "install";
	    $ENV{PPM_PREV_VERSION} = $pkg_info->{old_version};
	}
	local $ENV{PPM_INSTPACKLIST} = $pkg_info->{packlist} if exists $pkg_info->{packlist};;
	local $ENV{PPM_PERL} = $^X;

	$run_cb ||= do {
	    require ActiveState::Run;
	    \&ActiveState::Run::run;
	};

	eval {
	    chdir $tmpdir;
	    ppm_status("begin", "Running " . $self->name_version . " $kind script");
	    for my $cmd (@commands) {
		&$run_cb(ref($cmd) ? @$cmd : $cmd);
	    }
            ppm_status("end");
	};
	chdir($old_cwd) || die "Can't chdir back to '$old_cwd': $!";
	die if $@;
    }
}

sub _expand_exec {
    my $exec = shift;
    my @args;
    if ($exec =~ /\W/) {
	require Text::ParseWords;
	($exec, @args) = Text::ParseWords::shellwords($exec);
    }
    if (uc($exec) eq "SELF") {
	$exec = shift;
	$exec = "./$exec" if $^O ne "MSWin32";
    }
    elsif (uc($exec) eq "PPM_PERL" || $exec eq "perl") {
	$exec = $^X;
    }
    $exec = '@' . $exec;  # silence command echo
    return [$exec, @args, @_];
}


1;

__END__

=head1 NAME

ActivePerl::PPM::Package - Package class

=head1 SYNOPSIS

  my $pkg = ActivePerl::PPM::Package->new(name => 'Foo',...);
  # or
  my $pkg = ActivePerl::PPM::Package->new(\%hash);

=head1 DESCRIPTION

The C<ActivePerl::PPM::Package> class wraps hashes that describes
packages; the unit that the PPM system manages.

=head2 Constructors

The following constructor methods are provided:

=over

=item $pkg = ActivePerl::PPM::Package->new( %opt );

=item $pkg = ActivePerl::PPM::Package->new( \%self );

The constructor either take key/value pairs or a hash reference as
argument.  The only mandatory field is C<name>.  If a hash reference
is passed then it is turned into an C<ActivePerl::PPM::Package> object
and returned; which basically pass ownership of the hash.

=item $copy = $pkg->clone

Returns a copy of the current package object.  The attributes of the
clone can be modified without changing the original.

=item ActivePerl::PPM::Package->new_dbi($dbh, $id);

=item ActivePerl::PPM::Package->new_dbi($dbh, $name, $version);

Read object from a database and return it.  Returns C<undef> if no
package with the given key is found.

=item $pkg->dbi_store( $dbh )

Writes the current package to a database.  If $pkg was constructed by
C<new_dbi> then this updates the package, otherwise this creates a new
package object in the database.

Returns the $id of the object stored if successful, otherwise C<undef>.

=back

=head2 Attributes

The attributes of a package can be accessed directly using hash syntax
or by accesor methods.  The most common attributes are described
below, but the set of attributes is extensible.

=over

=item $str = $pkg->id

Returns the database id of package.  This attribute is set if the
object exists in a database.

=item $str = $pkg->name

Returns the name of the package.

=item $str = $pkg->version

Returns the version identifier for the package.  This string
can be anything and there is no reliable way to order packages based
on these version strings.

=item $str = $pkg->name_version

Returns the name and version concatenated together.  This form might
be handy for display, but there is no reliable way to parse back what
is the name and what is the version identifier.

=item $str = $pkg->release_date

Returns the date the package was released on as an ISO 8601 date
(YYYY-MM-DDThh:mm:ss).  For CPAN packages this is the date the package
was uploaded to CPAN.

=item $str = $pkg->author

The name and email address of the current maintainer of the package.

=item $str = $pkg->abstract

A short sentence describing the purpose of the package.

=item $url = $pkg->ppd_uri

This is the URI for the PPD file itself.

=item $str = $pkg->ppd_etag

This is the C<ETag> that the server reported for the PPD last time.

=item $str = $pkg->ppd_lastmod

This is the C<Last-Modified> field that the server reported for the
PPD last time.

=item $url = $pkg->codebase

Returns the URL to implementation; a blib tarball.
Interpret this URL relative to I<ppd_uri>.

=item $url = $pkg->codebase_abs

Returns the absolute URL to the 'codebase'

=item %features = $pkg->provides

Returns a list of (feature, version) pairs describing what features
this package provide.  A feature name with a double colon in it
represent a perl module.  A package always provide its own name as a
feature.

=item %features = $pkg->requires

Returns a list of (feature, version) pairs describing what features
this package require to be installed for it to work properly.  A
feature name with a double colon in it represent a perl module.

=back

=head2 Comparators

The following functions/methods can be used to order packages.

=over

=item $pkg->compare( $other )

Returns -1, 0, 1 like perl's builtin C<cmp>.  Return C<undef> if no order is defined.

=item $pkg->better_than( $other )

Returns TRUE if this package is better than the package passed as
argument.  This method will croak if no order is defined.

=item $pkg->best( @others )

=item ActivePerl::PPM::Package::best( @pkgs )

Returns the best package.  Might croak if no order is defined among
the packages passed in.

=back

=head2 Misc methods

=over

=item ActivePerl::PPM::Package->sql_create_tables

This returns SQL C<CREATE TABLE> statements used to initialize the
database that the C<new_dbi> and C<dbi_store> methods depend on.

=item $bool = $pkg->features_declared

Returns TRUE if this package declare what features it provide.  PPM4
style packages should declare what modules and other features they
provide, but packages from older repositories might not.

=item $pkg->has_script( $kind )

Return TRUE if the package has the given $kind of script attached.
The $kind argument should be either "install" or "uninstall".

=item $pkg->run_script( $kind, $area, $tmpdir, \%pkg_info )

Execute the given kind of script for the package.  The $kind argument
should be either "install" or "uninstall".  The $kind and $area
argument must be provided.

=back

=head1 BUGS

none.
