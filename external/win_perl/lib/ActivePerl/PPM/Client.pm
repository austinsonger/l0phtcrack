package ActivePerl::PPM::Client;

use strict;
use Config qw(%Config);

use ActivePerl;
use ActivePerl::PPM ();
use ActivePerl::PPM::InstallArea ();
use ActivePerl::PPM::Package ();
use ActivePerl::PPM::RepoPackage ();
use ActivePerl::PPM::PPD ();
use ActivePerl::PPM::Logger qw(ppm_log ppm_debug ppm_status);
use ActivePerl::PPM::Web qw(web_ua $BE_REPO_HOST);
use ActivePerl::PPM::Arch ();
use ActivePerl::PPM::SudoPath;
use ActivePerl::PPM::Util qw(join_with update_html_toc gunzip);

use ActiveState::Path qw(is_abs_path join_path);
use ActiveState::Handy qw(xml_esc);
use File::Basename;

use base 'ActivePerl::PPM::DBH';

# for HTTP::Response::freshness_lifetime
my $DAY = 24*60*60;
my %EXPIRY_DEFAULTS = (
    h_min => 1 * $DAY,
    h_max => 7 * $DAY,
);

sub new {
    my $class = shift;
    my $dir;
    $dir = shift if @_ % 2;
    my %opt = @_;

    my $build = $opt{activeperl_build} || ActivePerl::BUILD;

    $dir ||= $opt{home} || $ENV{ACTIVEPERL_PPM_HOME} ||
        do {
	    if ($^O eq "MSWin32") {
		require Win32;
		my $appdata = Win32::GetFolderPath(Win32::CSIDL_LOCAL_APPDATA()) ||
                    Win32::GetFolderPath(Win32::CSIDL_APPDATA()) ||
		    $ENV{APPDATA} || $ENV{HOME};
		die "No valid setting for APPDATA\n" unless $appdata;
		$appdata = Win32::GetShortPathName($appdata) || $appdata;
		"$appdata/ActiveState/ActivePerl/$build";
	    }
	    else {
		"$ENV{HOME}/.ActivePerl/$build";
	    }
	};

    my $config = $opt{perl_config} || \%Config;

    my $arch = $opt{arch} || ActivePerl::PPM::Arch::arch();
    my $ppmarch = ActivePerl::PPM::Arch::versioned_arch($opt{ppmarch} || $Config{ppmarch}, $]) || $arch;

    my $etc = $dir; # XXX or "$dir/etc";
    my @inc = $opt{inc} ? @{$opt{inc}} : (@main::INC_ORIG ? @main::INC_ORIG : @INC);

    # determine what install areas exists from @INC
    my @area;
    my %area;
    if ($opt{areas}) {
        for my $area (@{$opt{areas}}) {
            my $name = $area->name || die "Can't create client with nameless installarea";
            push(@area, $name);
            $area{$name} = $area;
        }
    }
    else {
        my @tmp = @inc;
        while (@tmp) {
            my $dir = shift(@tmp);
            next unless is_abs_path($dir);
            if (my $name = _known_area($dir, $config)) {
                push(@area, $name) unless grep $_ eq $name, @area;
                next;
            }

            my $base = File::Basename::basename($dir);
            my $archlib;
            if ($base eq $config->{archname} || $base eq "arch") {
                $archlib = $dir;
                $dir = File::Basename::dirname($dir);
                $dir = join_path($dir, "lib") if $base eq "arch";
                shift(@tmp) if $tmp[0] eq $dir;
            }
            my $lib = $dir;
            $base = File::Basename::basename($dir);
            $dir = File::Basename::dirname($dir) if $base eq "lib";

            my $name = _area_name($dir, $config);
            while (grep $_ eq $name, @area) {
                # make name unique
                my $num = ($name =~ s/_(\d+)//) ? $1 : 1;
                $name .= "_" . ++$num;
            }

            push(@area, $name);
            $area{$name} = ActivePerl::PPM::InstallArea->new(
                name => $name,
                prefix => $dir,
                lib => $lib,
                archlib => $archlib,
                autoinit => _user_area($dir),
            );
        }

        # make sure these install areas always show up
        for my $a (qw(site perl)) {
            push(@area, $a) unless grep $_ eq $a, @area;
        }
    }

    my $self = bless {
	dir => $dir,
	etc => $etc,
        arch => $arch,
	ppmarch => $ppmarch,
        activeperl_build => $build,
        perl_version => $opt{perl_version} || $config->{version},
        perl_config => $config,
	area => \%area,
        area_seq => \@area,
        inc => \@inc,
    }, $class;
    return $self;
}

sub _known_area {
    my $path = shift;
    my $config = shift || \%Config;
    return "perl" if _path_eq($path, $config->{privlib}, $config->{archlib});
    return "site" if _path_eq($path, $config->{sitelib}, $config->{sitearch});
    return "vendor" if $config->{vendorlib} && _path_eq($path, $config->{vendorlib}, $config->{vendorarch});
    return undef;
}

sub _user_area {
    my $path = shift;
    if ($^O eq "darwin") {
	my @paths = ("$ENV{HOME}/Library/ActivePerl");
	# Add ActivePerl-5.8 etc to the list (and never mind the bogus 5.9 etc entries)
	push @paths, "$paths[0]-5.$_" for 8 .. $Config{PERL_VERSION};
        return _path_eq($path, @paths);
    }
    return 0;
}

sub _path_eq {
    my @paths = @_;
    for (@paths) {
	s,/,\\,g if $^O eq "MSWin32";
	$_ = lc($_) if $^O eq "MSWin32" || $^O eq "darwin";
    }

    my $first = shift(@paths);
    for my $p (@paths) {
	return 1 if $first eq $p;
    }
    return 0;
}

sub _area_name {
    my $path = shift;
    my $config = shift || \%Config;

    # obtain name from the ppm-*-area.db file if present
    if (opendir(my $dh, "$path/etc")) {
	while (defined(my $f = readdir($dh))) {
	    if ($f =~ /^ppm-(\w+)-area.db$/) {
		if ($1 eq "perl" || $1 eq "site" || $1 eq "vendor") {
		    ppm_log("WARN", "Found $f in $path/etc");
		    last;
		}
		return $1;
	    }
	}
	closedir($dh);
    }

    return "user" if _user_area($path);

    # try to find a usable name from the $path
    my @path = split(/[\/\\]/, $path);
    while (@path) {
	my $segment = pop(@path);
	my $lc_segment = lc($segment);
	next if $segment eq "lib" || $segment eq "arch" || $segment eq $config->{archname};
	next if $lc_segment eq "perl" || $lc_segment eq "site" || $lc_segment eq "vendor";
	return "pdk" if $segment =~ /\bPerl Dev Kit\b/;
	next unless $segment =~ /^[\w\-.]{1,12}$/;
	return $segment;
    }

    # last resort
    return "user";
}

sub arch {
    my $self = shift;
    return $self->{arch};
}

sub areas {
    my $self = shift;
    return @{$self->{area_seq}};
}

sub area {
    my($self, $name) = @_;
    return undef unless $name;
    return $self->{area}{$name} ||= do {
	die "Install area '$name' does not exist" unless grep $_ eq $name, @{$self->{area_seq}};
	ActivePerl::PPM::InstallArea->new(name => $name, autoinit => 1, perl_config => $self->{perl_config});
    }
}

sub default_install_area {
    my $self = shift;
    my $area = "site";
    if ($self->area($area)->readonly) {
	my @areas = $self->areas;
	while (defined($area = shift(@areas))) {
	    next if $area eq "perl" || $area eq "site" || $area eq "vendor";
	    next if $area eq "pdk";
	    last unless $self->area($area)->readonly;
	}
    }
    return $area;
}

sub _init_db {
    my $self = shift;
    my $etc = $self->{etc};
    my $file_arch = $self->{arch};
    $file_arch =~ s/\./_/g;  # don't confuse version number dots with file extension
    my $db_file = "$etc/ppm-$file_arch.db";
    my $sudo = ActivePerl::PPM::SudoPath->new($db_file);
    unless (-d $etc) {
	require File::Path;
	File::Path::mkpath($etc) || die "Can't mkpath($etc): $!";
    }
    require DBI;
    my $dbh = DBI->connect("dbi:SQLite:dbname=$db_file", "", "", {
        AutoCommit => 1,
        RaiseError => 1,
        sqlite_use_immediate_transaction => 0,
    });
    die "$db_file: $DBI::errstr" unless $dbh;
    $sudo->chown;

    local $dbh->{AutoCommit} = 0;
    my $v = $dbh->selectrow_array("PRAGMA user_version");
    die "Assert" unless defined $v;
    if ($v == 0) {
	ppm_log("WARN", "Setting up schema for $db_file");
	_init_ppm_schema($dbh, $self->{arch}, $self->{ppmarch}, $self->{activeperl_build});
	$dbh->do("PRAGMA user_version = 1");
	$dbh->commit;
    }
    elsif ($v != 1) {
	die "Unrecognized database schema $v for $db_file";
    }

    $self->{db_file} = $db_file;
    $self->{dbh} = $dbh;
}

sub _init_ppm_schema {
    my($dbh, $arch, $ppmarch, $build) = @_;
    $dbh->do(<<'EOT');
CREATE TABLE config (
    key text primary key,
    value text
)
EOT
    $dbh->do(<<'EOT');
CREATE TABLE repo (
    id integer primary key,
    name text not null,
    prio integer not null default 1,
    enabled bit not null default 1,
    packlist_uri text not null unique,
    packlist_version text,
    packlist_last_status_code int,
    packlist_last_access integer,
    packlist_etag text,
    packlist_size integer,
    packlist_lastmod text,
    packlist_fresh_until integer
)
EOT
    for my $create (ActivePerl::PPM::RepoPackage->sql_create_tables()) {
	$dbh->do($create) || die "Can't create database table";
    }

    $dbh->do("CREATE TABLE search ( id integer )");

    # initial values
    $dbh->do("INSERT INTO config(key, value) VALUES ('arch', ?)", undef, $arch);
    $dbh->do("INSERT INTO config(key, value) VALUES ('repo_dbimage', 1)")
	if $ActivePerl::VERSION >= 1800;
    unless (ActivePerl::PRODUCT() =~ /enterprise/i) {
	if (my @repo = activestate_repo($ppmarch, $build)) {
	    $dbh->do(qq(INSERT INTO repo(name,packlist_uri) VALUES (?, ?)), undef, @repo);
	}
    }
}

sub config_get {
    my $self = shift;
    my $dbh = $self->dbh;
    my @res;
    for (@_) {
	my $v = $dbh->selectrow_array("SELECT value FROM config WHERE key = ?", undef, $_);
	push(@res, $v);
    }
    return wantarray ? @res : $res[0];
}

sub config_list {
    my $self = shift;
    my $dbh = $self->dbh;
    my $sth = $dbh->prepare(
        "SELECT key, value FROM config" .
	(@_ ? " WHERE key GLOB ?" : "") .
        " ORDER BY key",
    );
    $sth->execute(@_);
    my @kv;
    while (my($k, $v) = $sth->fetchrow_array) {
	push(@kv, $k, $v);
    }
    return @kv;
}

sub config_save {
    my $self = shift;
    my $dbh = $self->dbh;
    local $dbh->{AutoCommit} = 0;
    while (@_) {
	my $k = shift;
	my $v = shift;
	$dbh->do("INSERT OR REPLACE INTO config (key,value) VALUES (?, ?)", undef, $k, $v);
    }
    $dbh->commit;
}

sub be_state {
    my $self = shift;
    my $state = $self->{be_state} || $ENV{ACTIVEPERL_PPM_BE_STATE};
    return $state if $state;

    my $ua = web_ua();
    if ($ua->be_credentials) {
	if (my $last_state = $self->config_get("_be_state")) {
	    ($state, my $expiry) = split(" ", $last_state);
	    undef $state if !$expiry || $expiry < time;
	}
	unless ($state) {
	    my $status_url = "http://$BE_REPO_HOST/status";
	    my $resp = web_ua()->get($status_url);
	    $state = {
		200 => "valid",
		401 => "expired",
		403 => "expired",
	    }->{$resp->code} || "unknown";
	    $self->config_save("_be_state", join(" ", $state, time + 60));
	}
    }
    else {
	$state = "invalid";
    }

    $self->{be_state} = $state;
    return $state;
}

sub cannot_install {
    my($self, $pkg) = @_;
    $self->package_set_abs_ppd_uri($pkg);  # ensure ppd_uri is set
    my $codebase = $pkg->codebase_abs;
    return "missing codebase" unless $codebase;
    if ($codebase->host eq $BE_REPO_HOST) {
	my $be_state = $self->be_state;
	return "needs business edition license installed" if $be_state eq "invalid";
	return "business edition subscription expired" if $be_state eq "expired";
    }
    return "";  # no restrictions
}

sub activestate_repo {
    my $arch = ActivePerl::PPM::Arch::short_arch(shift);
    $arch =~ s,-(5.\d+)$,/$1,;
    my $v = shift || ActivePerl::BUILD;
    my $repo_uri = "http://ppm4.activestate.com/$arch/$v/package.xml";
    if ($^O =~ /^(MSWin32|linux|darwin|hpux|solaris)$/ || web_ua()->head($repo_uri)->is_success) {
	return $repo_uri unless wantarray;
	return ("ActiveState Package Repository", $repo_uri);
    }
    return;
}

sub repos {
    my $self = shift;
    @{$self->dbh->selectcol_arrayref("SELECT id FROM repo ORDER BY id")};
}

sub repo {
    my($self, $id) = @_;
    my $dbh = $self->dbh;
    my $hash;
    if ($id =~ /^\d+$/) {
	$hash = $dbh->selectrow_hashref("SELECT * FROM repo WHERE id = ?", undef, $id);
    }
    else {
	$hash = $dbh->selectrow_hashref("SELECT * FROM repo WHERE name = ?", undef, $id);
	unless ($hash) {
	    my $sth = $dbh->prepare("SELECT * FROM repo WHERE name LIKE ?");
	    $sth->execute("%$id%");
	    my @h;
	    while (my $h = $sth->fetchrow_hashref) {
		push(@h, $h);
	    }
	    $hash = $h[0] if @h == 1;  # unique
	}
    }
    if ($hash) {
	my $pkgs = $dbh->selectrow_array("SELECT count(*) FROM package WHERE repo_id = ?", undef, $id);
	$hash->{pkgs} = $pkgs;
    }
    return $hash;
}

sub repo_enable {
    my $self = shift;
    my $id = shift;
    my $enabled = @_ ? (shift(@_) ? 1 : 0) : 1;

    my $dbh = $self->dbh;
    if ($dbh->do("UPDATE repo SET enabled = ?, packlist_etag = NULL, packlist_lastmod = NULL, packlist_size = NULL, packlist_fresh_until = NULL WHERE id = ?", undef, $enabled, $id)) {
	if ($enabled) {
	    $self->repo_sync;
	}
	else {
            my $uri = $dbh->selectrow_array("SELECT packlist_uri FROM repo WHERE id = ?", undef, $id);
            if ($uri =~ s/\.db\.gz$/.xml/) {
                $dbh->do("UPDATE repo SET packlist_uri = ?, packlist_last_status_code = NULL, packlist_last_access = NULL WHERE id = ?", undef, $uri, $id);
            }
	    local $dbh->{AutoCommit} = 0;
	    _repo_delete_packages($dbh, $id);
	    $dbh->commit;
	}
    }
}

sub repo_add {
    my($self, %attr) = @_;
    my $dbh = $self->dbh;
    local $dbh->{RaiseError} = 0;
    if ($dbh->do("INSERT INTO repo (name, packlist_uri, prio) VALUES (?, ?, ?)", undef,
	         $attr{name}, $attr{packlist_uri}, ($attr{prio} || 0)))
    {
	my $id = $dbh->func('last_insert_rowid');
	$self->repo_sync unless defined $attr{enabled} && !$attr{enabled};
	return $id;
    }
    my $err = $DBI::errstr;
    if (my $repo = $self->dbh->selectrow_hashref("SELECT * FROM repo WHERE packlist_uri = ?", undef, $attr{packlist_uri})) {
	die "Repo ", $repo->{name} || $repo->{id}, " already set up with URL $attr{packlist_uri}";
    }
    die $err;
}

sub repo_delete {
    my($self, $id) = @_;
    my $dbh = $self->dbh;
    local $dbh->{AutoCommit} = 0;
    _repo_delete_packages($dbh, $id);
    $dbh->do("DELETE FROM repo WHERE id = ?", undef, $id);
    $dbh->commit;
}

sub _repo_delete_packages {
    my($dbh, $id) = @_;
    $dbh->do("DELETE FROM feature WHERE package_id IN (SELECT id FROM package WHERE repo_id = ?)", undef, $id);
    $dbh->do("DELETE FROM script WHERE package_id IN (SELECT id FROM package WHERE repo_id = ?)", undef, $id);
    $dbh->do("DELETE FROM package WHERE repo_id = ?", undef, $id);
}

sub repo_set_name {
    my($self, $id, $name) = @_;
    my $dbh = $self->dbh;
    $dbh->do("UPDATE repo SET name = ? WHERE id = ?", undef, $name, $id);
}

sub repo_set_packlist_uri {
    my($self, $id, $uri) = @_;
    my $dbh = $self->dbh;
    $dbh->{AutoCommit} = 0;
    $dbh->do("UPDATE repo SET packlist_uri = ?, packlist_last_status_code = NULL, packlist_last_access = NULL, packlist_etag = NULL, packlist_size = NULL, packlist_lastmod = NULL, packlist_fresh_until =  NULL WHERE id = ?", undef, $uri, $id);
    _repo_delete_packages($dbh, $id);
    $dbh->commit;
}

sub repo_sync {
    my($self, %opt) = @_;

    # force implies validate
    $opt{validate} ||= $opt{force};

    my @repos;
    my $dbh = $self->dbh;
    my $sth = $dbh->prepare("SELECT * FROM repo WHERE enabled == 1" .
			    ($opt{repo} ? " AND id = $opt{repo}" : "") .
			    " ORDER BY id");
    $sth->execute();
    while (my $h = $sth->fetchrow_hashref) {
	push(@repos, $h);
    }
    if ($opt{repo} && !@repos) {
	if ($dbh->selectrow_array("SELECT count(*) FROM repo WHERE id = $opt{repo}")) {
	    die "Repo $opt{repo} is not enabled";
	}
	else {
	    die "Repo $opt{repo} does not exist";
	}
    }

    if (@repos == 1 && (!$repos[0]->{packlist_last_status_code} || $repos[0]->{packlist_uri} =~ /\.db\.gz$/)) {
	if ($dbh->selectrow_array("SELECT count(*) FROM repo") == 1) {
	    return if $self->_repo_sync_dbimage($repos[0], $dbh, %opt)
	}
    }

    local $dbh->{AutoCommit} = 0;

    for my $repo (@repos) {
	my @check_ppd;
	my %delete_package;
	if (!$opt{validate} && $repo->{packlist_fresh_until} && $repo->{packlist_fresh_until} >= time) {
	    @check_ppd = (); # XXX should we still check them?
	    ppm_log("DEBUG", "$repo->{packlist_uri} is still fresh");
	}
	else {
	    my $ua = web_ua();
	    local $ua->{progress_what} = "Downloading $repo->{name} packlist";
	    my $res;
	    if ($repo->{packlist_last_status_code}) {
		# if we continue to get errors from repo, only hit it occasionally
		if (!$opt{validate} &&
		    HTTP::Status::is_error($repo->{packlist_last_status_code}) &&
		    (time - $repo->{packlist_last_access} < 5 * 60))
		{
		    ppm_log("WARN", "$repo->{packlist_uri} is known to err, skipping sync");
		    next;
		}
	    }
	    else {
		# first time, try to find the package.xml or package.lst file
		my $uri = $repo->{packlist_uri};
		unless ($uri =~ m,/package\.(xml|lst)$,) {
		    $uri = URI->new($uri);
		    my @try;
		    my $uri_slash = $uri;
		    unless ($uri_slash->path =~ m,/$,) {
			$uri_slash = $uri->clone;
			$uri_slash->path( $uri->path . "/");
		    }
		    push(@try, URI->new_abs("package.xml", $uri_slash));
		    push(@try, URI->new_abs("package.lst", $uri_slash));
		    my $try;
		    for $try (@try) {
			my $try_res = $ua->get($try);
			if ($try_res->is_success && $try_res->decoded_content(default_charset => "none") =~ /<REPOSITORY(?:SUMMARY)?\b/) {
			    $repo->{packlist_uri} = $try->as_string;
			    $dbh->do("UPDATE repo SET packlist_uri = ? WHERE id = ?", undef, $repo->{packlist_uri}, $repo->{id});
			    $res = $try_res;
			    ppm_log("WARN", "Will use $repo->{packlist_uri} instead");
			    last;
			}
		    }
		}
	    }

	    unless ($res) {
		my @h;
		if (!$opt{force}) {
		    push(@h, "If-None-Match", $repo->{packlist_etag}) if $repo->{packlist_etag};
		    push(@h, "If-Modified-Since", $repo->{packlist_lastmod}) if $repo->{packlist_lastmod};
		}
		$res = $ua->get($repo->{packlist_uri}, @h);
	    }
	    $dbh->do("UPDATE repo SET packlist_last_status_code = ?, packlist_last_access = ? WHERE id = ?", undef, $res->code, time, $repo->{id});
	    #print $res->status_line, "\n";
	    if ($res->code == 304) {  # not modified
		@check_ppd = @{$dbh->selectcol_arrayref("SELECT ppd_uri FROM package WHERE ppd_uri NOTNULL AND repo_id = ?", undef, $repo->{id})};
		$dbh->do("UPDATE repo SET packlist_fresh_until=? WHERE id=?", undef, $res->fresh_until(%EXPIRY_DEFAULTS), $repo->{id});
	    }
	    elsif ($res->is_success) {
		$dbh->do("UPDATE repo SET packlist_etag=?, packlist_lastmod=?, packlist_size=?, packlist_fresh_until=? WHERE id=?", undef,
			 scalar($res->header("ETag")),
			 scalar($res->header("Last-Modified")),
			 scalar($res->header("Content-Length")),
			 $res->fresh_until(%EXPIRY_DEFAULTS),
			 $repo->{id});

		# parse document
		eval {
		my $cref = $res->decoded_content(ref => 1, default_charset => "none", raise_error => 1);
		if ($res->content_type eq "text/html") {
		    my $base = $res->base;
		    require HTML::Parser;
		    my $p = HTML::Parser->new(
	                report_tags => [qw(a)],
	                start_h => [sub {
			    my $href = shift->{href} || return;
			    push(@check_ppd, URI->new_abs($href,$base)->rel($repo->{packlist_uri})) if $href =~ /\.ppd$/;
			}, "attr"],
		    );
		    $p->parse($$cref)->eof;
		    ppm_log("WARN", "No ppds found in $repo->{packlist_uri}") unless @check_ppd;

		    %delete_package = map { $_ => 1 } @{$dbh->selectcol_arrayref("SELECT id FROM package WHERE repo_id = ?", undef, $repo->{id})};

		    my $max_ppd = $opt{max_ppd} || 100;
		    if (@check_ppd > $max_ppd) {
			my $num_ppd = @check_ppd;
			ppm_log("ERR", "PPD downloads blocked. The repo $repo->{packlist_uri} links to $num_ppd PPD files.  Current download limit is $max_ppd.");
			@check_ppd = ();
			%delete_package = ();
		    }

		}
		elsif ($$cref =~ /<REPOSITORY(?:SUMMARY)?\b/) {
                    #use Time::HiRes qw(time);
                    #my $t = time;
		    my $status = ppm_status();
		    $status->begin("Updating $repo->{name} database");

		    # It's faster to insert lots of rows in the database if the engine doesn't
		    # have to update the indexes as it goes.  We'll recreate these indexes again
		    # before this routine returns.  This "trick" reduce the time spent updating
		    # the database by 15%.  Not having these indexes at all makes it 30% faster
		    # to update, but then it's really slow to look up stuff.
		    $dbh->do("DROP INDEX IF EXISTS feature_package_id");
		    $dbh->do("DROP INDEX IF EXISTS feature_name_role");

                    my %have;
                    if ($opt{force}) {
                        _repo_delete_packages($dbh, $repo->{id});
                    }
                    else {
                        keys %have = 12000; # pre-extend
                        my $sth = $dbh->prepare("SELECT id, name, version FROM package WHERE repo_id = $repo->{id}");
                        $sth->execute;
                        while (my($id, $name, $version) = $sth->fetchrow_array) {
                            $have{"$name $version"} = $id;
                        }
                    }
		    $status->tick;
                    my %seen;
                    keys %seen = scalar(keys %have);
		    require ActivePerl::PPM::ParsePPD;
		    my $p = ActivePerl::PPM::ParsePPD->new(sub {
			my $pkg = shift;
                        my $k = "$pkg->{name} $pkg->{version}";
                        if ($have{$k}) {
                            $seen{$k}++;
                            return;
                        }
			$pkg = ActivePerl::PPM::RepoPackage->new_ppd($pkg, arch => $self->{arch});
                        return unless $pkg->{codebase};
			$pkg->{repo_id} = $repo->{id};
                        $pkg->dbi_store($dbh);
			$status->tick;
		    });
		    $p->parse_more($$cref);
		    $p->parse_done;
                    if (keys(%have) != keys(%seen)) {
                        delete @have{keys %seen};
                        my $ids = join(",", sort {$a <=> $b} values %have);
                        $dbh->do("DELETE FROM feature WHERE package_id IN ($ids)");
                        $dbh->do("DELETE FROM script WHERE package_id IN ($ids)");
                        $dbh->do("DELETE FROM package WHERE id IN ($ids)");
                    }
		    $status->end;
                    #printf "Used %.2f sec\n", time - $t;
		}
		else {
		    ppm_log("ERR", "Unrecognized repo type " . $res->content_type);
		}
		};
		if ($@) {
		    ppm_log("ERR", "Unable to process response from repo $repo->{name}: $@");
		}
	    }
	    else {
		$dbh->do("UPDATE repo SET packlist_fresh_until=? WHERE id=?", undef, 0, $repo->{id});
	    }
	}

	for my $ppd (@check_ppd) {
	    _check_ppd($ppd, $self->{arch}, $repo, $dbh, \%delete_package);
	}

	$dbh->do("DELETE FROM package WHERE id IN (" . join(",", sort keys %delete_package) . ")")
	    if %delete_package;

	$dbh->commit;
    }

    # Recreate indexes that we might have dropped during the update
    for (ActivePerl::PPM::RepoPackage->sql_create_tables(indexes_only => 1)) {
	$dbh->do($_);
    }
    $dbh->commit;

    return;
}


sub _check_ppd {
    my($rel_url, $arch, $repo, $dbh, $delete_package) = @_;

    my $row = $dbh->selectrow_hashref("SELECT id, ppd_etag, ppd_lastmod, ppd_fresh_until FROM package WHERE repo_id = ? AND ppd_uri = ?", undef, $repo->{id}, $rel_url);

    my @h;
    if ($row) {
	if ($row->{ppd_fresh_until} && $row->{ppd_fresh_until} > time) {
	    delete $delete_package->{$row->{id}} if $delete_package;
	    return;
	}
	push(@h, "If-None-Match", $row->{ppd_etag}) if $row->{ppd_etag};
	push(@h, "If-Modified-Since", $row->{ppd_lastmod}) if $row->{ppd_lastmod};
    }

    my $abs_url = URI->new_abs($rel_url, $repo->{packlist_uri});
    my $ua = web_ua();
    (my $base_url = $rel_url) =~ s,.*/,,;
    $base_url =~ s,\.ppd$,,;
    local $ua->{progress_what} = "Downloading $repo->{name} $base_url PPD";
    my $ppd_res = $ua->get($abs_url, @h);
    #print $ppd_res->as_string, "\n" unless $ppd_res->code eq 200 || $ppd_res->code eq 304;
    if ($row && $ppd_res->code == 304) {  # not modified
	$dbh->do("UPDATE package SET ppd_fresh_until = ? WHERE id = ?", undef, $ppd_res->fresh_until(%EXPIRY_DEFAULTS), $row->{id});
	delete $delete_package->{$row->{id}} if $delete_package;
    }
    elsif ($ppd_res->is_success) {
	my $ppd = ActivePerl::PPM::RepoPackage->new_ppd(
	    $ppd_res->decoded_content(default_charset => "none"),
            arch => $arch,
            base => $ppd_res->base,
            rel_base => $abs_url,
        );
	if ($ppd->{codebase}) {
	    $ppd->{id} = $row->{id} if $row;
	    $ppd->{repo_id} = $repo->{id};
	    $ppd->{ppd_uri} = $rel_url;
	    $ppd->{ppd_etag} = $ppd_res->header("ETag");
	    $ppd->{ppd_lastmod} = $ppd_res->header("Last-Modified");
	    $ppd->{ppd_fresh_until} = $ppd_res->fresh_until(%EXPIRY_DEFAULTS);

	    $ppd->dbi_store($dbh);
	    delete $delete_package->{$row->{id}} if $delete_package && $row;
	}
    }
}


sub _repo_sync_dbimage {
    my($self, $repo, $dbh, %opt) = @_;

    if (!$opt{validate} && $repo->{packlist_fresh_until} && $repo->{packlist_fresh_until} >= time) {
	ppm_log("DEBUG", "$repo->{packlist_uri} is still fresh");
	return 1;
    }

    my $db_file = $self->{db_file};
    my $db_file_old = "$db_file.old";
    my $db_file_gz = "$db_file.gz";

    my $ua = web_ua();
    local $ua->{progress_what} = "Downloading $repo->{name} dbimage";
    my $res;

    if ($repo->{packlist_last_status_code}) {
	# if we continue to get errors from repo, only hit it occasionally
	if (!$opt{validate} &&
	    HTTP::Status::is_error($repo->{packlist_last_status_code}) &&
	    (time - $repo->{packlist_last_access} < 5 * 60))
	{
	    ppm_log("WARN", "$repo->{packlist_uri} is known to err, skipping sync");
	    return 1;
	}
    }
    else {
	# first time
	my $uri = $repo->{packlist_uri};
	return 0 unless $self->config_get('repo_dbimage');
	return 0 unless $uri =~ s,/(package)\.xml$,/$1.db.gz,;
	$res = $ua->get($uri, ':content_file' => $db_file_gz);
	return 0 unless $res->is_success;
	$repo->{packlist_uri} = $uri;
	$dbh->do("UPDATE repo SET packlist_uri = ? WHERE id = ?", undef, $repo->{packlist_uri}, $repo->{id});
    }

    unless ($res) {
	my @h;
	if (!$opt{force}) {
	    push(@h, "If-None-Match", $repo->{packlist_etag}) if $repo->{packlist_etag};
	    push(@h, "If-Modified-Since", $repo->{packlist_lastmod}) if $repo->{packlist_lastmod};
	}
	$res = $ua->get($repo->{packlist_uri}, @h, ':content_file' => $db_file_gz);
    }
    $dbh->do("UPDATE repo SET packlist_last_status_code = ?, packlist_last_access = ? WHERE id = ?", undef, $res->code, time, $repo->{id});
    if ($res->code == 304) {  # not modified
	$dbh->do("UPDATE repo SET packlist_fresh_until=? WHERE id=?", undef, $res->fresh_until(%EXPIRY_DEFAULTS), $repo->{id});
	return 1;
    }

    unless ($res->is_success) {
	$dbh->do("UPDATE repo SET packlist_fresh_until=? WHERE id=?", undef, 0, $repo->{id});
	return 1;
    }

    $dbh->do("UPDATE repo SET packlist_etag=?, packlist_lastmod=?, packlist_size=?, packlist_fresh_until=? WHERE id=?", undef,
	     scalar($res->header("ETag")),
	     scalar($res->header("Last-Modified")),
	     scalar($res->header("Content-Length")),
	     $res->fresh_until(%EXPIRY_DEFAULTS),
	     $repo->{id});

    my $user_version = $dbh->selectrow_array("PRAGMA user_version");

    # process new dbimage file
    undef($dbh);
    $self->dbh_disconnect;

    rename($db_file, $db_file_old) || die "Can't rename $db_file: $!";
    gunzip($db_file_gz);

    # Fixup stuff
    $dbh = $self->dbh;  # reconnect to new database

    # Don't continue if these for some reason don't match the old values
    die "Mismatched arch in dbimage" unless $self->config_get('arch') eq $self->{arch};
    die "Mismatched schema version in dbimage" unless $dbh->selectrow_array("PRAGMA user_version") eq $user_version;

    # Copy some tables over from the old database
    $dbh->do("ATTACH DATABASE ? AS old", undef, $db_file_old);
    for my $table ("config", "repo") {
	$dbh->do("REPLACE INTO $table SELECT * from old.$table");
    }
    $dbh->do("DETACH DATABASE old");

    # Final fixup
    $dbh->do("DELETE FROM search");
    for (ActivePerl::PPM::RepoPackage->sql_create_tables(indexes_only => 1)) {
	$dbh->do($_);
    }

    return 1;
}


sub repo_dbimage_disable {
    my $self = shift;

    my @fixup;

    my $dbh = $self->dbh;
    my $sth = $dbh->prepare("SELECT id, packlist_uri FROM repo");
    $sth->execute;
    while (my($id, $uri) = $sth->fetchrow_array) {
	if ($uri =~ s,/ppm-[^/]+\.db\.gz$,/package.xml,) {
	    push(@fixup, [$id, $uri]);
	}
    }
    for (@fixup) {
	my($id, $uri) = @$_;
	$dbh->do("UPDATE repo SET packlist_uri = ?, packlist_last_status_code=NULL, packlist_last_access=NULL, packlist_etag=NULL, packlist_size=NULL, packlist_lastmod=NULL WHERE id = ?", undef, $uri, $id);
    }
}

sub _package_fields {
    my($self, %opt) = @_;

    my @fields = @{$opt{fields}};
    my $prefix = $opt{prefixed} ? "package." : "";

    for my $f (@fields) {
	if ($f eq "cannot_install") {
	    my $be_state = $self->be_state;
	    if ($be_state eq "invalid" || $be_state eq "expired") {
		$f = qq(like("http://$BE_REPO_HOST/%", ${prefix}codebase));
	    }
	    else {
		$f = "0";
	    }
	}
	else {
	    $f = $prefix . $f;
	}
    }

    return @fields if wantarray;
    return join(",", @fields);
}

sub packages {
    my $self = shift;
    my $dbh = $self->dbh;
    if (@_) {
	my $aref = $dbh->selectall_arrayref(
	    "SELECT " . $self->_package_fields(fields => \@_) .
	    " FROM package"
	);
	my $i = 0;
	for my $f (@_) {
	    if ($f eq "abstract" || $f eq "author") {
		# Potential Unicode field
		for my $row (@$aref) {
		    if (($row->[$i] || "") =~ /[^\x00-\x7F]/) {
			utf8::decode($row->[$i]);
		    }
		}
	    }
	    $i++;
	}
	return @$aref;
    }
    return @{$dbh->selectcol_arrayref("SELECT id FROM package")}
	if wantarray;
    return $dbh->selectrow_array("SELECT count(*) FROM package");
}

sub search {
    my($self, $pattern, @fields) = @_;
    @fields = ("name") unless @fields;

    my $dbh = $self->dbh;

    $dbh->do("DELETE FROM search");
    local $dbh->{AutoCommit} = 0;

 SEARCH: {
	if ($pattern =~ /::/) {
	    my $op = ($pattern =~ /\*/) ? "GLOB" : "=";
	    $dbh->do("INSERT INTO search SELECT id FROM package WHERE id IN (SELECT package_id FROM feature WHERE lower(name) $op ? AND role = 'p') ORDER BY name", undef, lc($pattern));
	    last SEARCH;
	}

	if ($pattern eq '*') {
	    $dbh->do("INSERT INTO search SELECT id FROM package ORDER BY name");
	    last SEARCH;
	}

	unless ($pattern =~ /\*/) {
	    $dbh->do("INSERT INTO search SELECT id FROM package WHERE name = ?", undef, $pattern);
	    last SEARCH if $dbh->selectrow_array("SELECT count(*) FROM search");
	    # try again with a wider net
	    $dbh->rollback;
	    $pattern = "*$pattern*";
	}
	$dbh->do("INSERT INTO search SELECT id FROM package WHERE lower(name) GLOB ? ORDER BY name", undef, lc($pattern));
    }
    $dbh->commit;

    my $fields = $self->_package_fields(fields => \@fields, prefixed => 1);
    my $select_arrayref = @fields > 1 ? "selectall_arrayref" : "selectcol_arrayref";
    return @{$dbh->$select_arrayref("SELECT $fields FROM package,search WHERE package.id = search.id ORDER by search.rowid")};
}

sub search_lookup {
    my($self, $row) = @_;
    my $dbh = $self->dbh;
    my $id = $dbh->selectrow_array("SELECT id FROM search WHERE rowid = $row");
    return $self->package($id) if defined $id;
    return undef;
}

sub feature_best {
    my($self, $feature) = @_;
    my $dbh = $self->dbh;
    my($max) = $dbh->selectrow_array("SELECT max(version) FROM feature WHERE name = ? AND role = 'p'",
 undef, $feature);
    return $max;
}

sub package_best {
    my($self, $feature, $version) = @_;
    my $dbh = $self->dbh;

    my $ids = $dbh->selectcol_arrayref("SELECT package.id FROM package, feature WHERE package.id = feature.package_id AND feature.role = 'p' AND feature.name = ? AND feature.version >= ?", undef, $feature, $version);

    my @pkg = map $self->package($_), @$ids;

    return ActivePerl::PPM::Package::best(@pkg);
}

sub feature_have {
    if (my $memo = $_[0]->{feature_have_memoize}) {
	return $memo->{join("\0", @_[1..$#_])} ||= &_feature_have;
    }
    goto &_feature_have;
}

sub _feature_have {
    my $self = shift;
    my $feature = shift;
    for my $area_name (@_ ? @_ : $self->areas) {
	my $area = $self->area($area_name);
	next if !@_ && !$area->initialized;
	if (defined(my $have = $area->feature_have($feature))) {
	    return $have;
	}
    }

    if (!@_ && $feature =~ /::/) {
	require ActiveState::ModInfo;
        require ActiveState::Version;
	if (my $path = ActiveState::ModInfo::find_module($feature, $self->{inc})) {
	    return ActiveState::Version::vnumify(ActiveState::ModInfo::parse_version($path));
	}
    }

    return undef;
}

sub feature_fixup_case {
    my($self, $name) = @_;
    my $dbh = $self->dbh;
    my $names = $dbh->selectcol_arrayref("SELECT DISTINCT name FROM feature WHERE role = 'p' AND lower(name) = lower(?)", undef, $name);
    if (@$names && !grep $_ eq $name, @$names) {
	die "Feature name $name is ambiguous; please select one of:\n    " . join(", ", @$names)
	    if @$names > 1;
	ppm_log("WARN", "Using feature name '$names->[0]' instead of '$name'");
	$name = $names->[0];
    }
    return $name;
}

sub packages_depending_on {
    my($self, $pkg, $area_name) = @_;
    my $pkg_name;
    if (ref $pkg) {
	$pkg_name = $pkg->name;
    }
    else {
	$pkg_name = $pkg;
	$pkg = undef;
    }
    unless ($area_name) {
	for $a ($self->areas) {
	    my $area = $self->area($a);
	    next unless $area->initialized;
	    if (defined $area->package_id($pkg_name)) {
		$area_name = $a;
		last;
	    }
	}
	die "Can't find $pkg_name in any area" unless $area_name;
	#print "Found $pkg_name in $area_name\n";
    }
    $pkg = $self->area($area_name)->package($pkg_name) unless $pkg;

    my %provide = $pkg->provides;
    #print "$pkg_name provide: @{[sort keys %provide]}\n";
    my $feature_sql = join(",", map "'$_'", sort keys %provide);

    # find packages that require any of the features $pkg provide
    my @dep_pkgs;
    for $a ($self->areas) {
	my $area = $self->area($a);
	next unless $area->initialized;
	my $dbh = $area->dbh;
	my $pkg_ids = $dbh->selectcol_arrayref("SELECT package_id FROM feature WHERE role = 'r' AND name in ($feature_sql)");
	next unless $pkg_ids && @$pkg_ids;
	for my $dep_pkg (map $area->package($_), @$pkg_ids) {
	    next if $a eq $area_name && $pkg->name eq $dep_pkg->name;
	    push(@dep_pkgs, $dep_pkg);
	}
    }

    # XXX Check versions
    # XXX Check if dependant features are provided by other package

    return @dep_pkgs;
}

sub packages_missing {
    my($self, %args) = @_;
    my @pkg_have = @{delete $args{have} || []};
    my @area_have = @{delete $args{area} || []};
    my @todo = @{delete $args{want} || []};

    my $force = delete $args{force};
    my $follow_deps = delete $args{follow_deps} || "missing";
    if (my $want_deps = delete $args{want_deps}) {
	push(@pkg_have, @$want_deps);
	for my $pkg (@$want_deps) {
	    if ($follow_deps ne "none") {
		if (my $dep = $pkg->{require}) {
		    push(@todo, map [$_ => $dep->{$_}, $pkg->{name} ], keys %$dep);
		}
	    }
	}
    }

    my $error_handler = delete $args{error_handler};

    if ($^W && %args) {
	require Carp;
	Carp::carp("Unknown argument '$_' passed") for sort keys %args;
    }

    return unless @todo;

    # We might end up making lots of calls to 'feature_have' for the same
    # features while processing this call.  Enable memoizing of its values to
    # make it cheaper.
    local $self->{feature_have_memoize} = {};

    my @err;
    my @missing_upgrade;
    for my $feature (@todo) {
	$feature = [$feature, 0] unless ref($feature);
	my($name, $version) = @$feature;
	unless (defined $version) {
	    if (defined($version = $self->feature_best($name))) {
		$feature->[1] = $version;
	    }
	    else {
		push(@missing_upgrade, $name);
	    }
	}
    }
    if (@missing_upgrade) {
	push(@err, "No " . join_with("or", sort @missing_upgrade) . " available");
	@todo = grep defined($_->[1]), @todo;
    }

    my @pkg_missing;
    while (@todo) {
        my($feature, $want, $needed_by) = @{shift @todo};

        my $have;
	for my $pkg (@pkg_have, @pkg_missing) {
	    $have = $pkg->{provide}{$feature};
	    if (defined $have) {
		if ($have < $want) {
		    my $msg = "Conflict for feature $feature version $have provided by $pkg->{name}, ";
		    $msg .= "$needed_by " if $needed_by;
		    $msg .= "want version $want";
		    push(@err, $msg);
		}
		push(@{$pkg->{_needed_by}}, $needed_by) if $needed_by;
		last;
	    }
	}
	$have = $self->feature_have($feature, @area_have) unless defined($have);
	ppm_debug("Want $feature" . ($want ? " $want" : "") . (defined($have) ? ", have $feature $have" : " (missing)"));

        if ((!$needed_by && $force) ||
	    ($needed_by && $follow_deps eq "all") ||
            !defined($have) || $have < $want)
        {
            if (my $pkg = $self->package_best($feature, $want)) {
		unless ($force) {
		    my $err = $self->is_downgrade($pkg, $feature, $needed_by);
		    push(@err, $err) if $err;
		}
		push(@pkg_missing, $pkg);
		if ($needed_by) {
		    push(@{$pkg->{_needed_by}}, $needed_by);
		}
		else {
		    $pkg->{_wanted}++;
		}

		if ($follow_deps ne "none") {
		    if (my $dep = $pkg->{require}) {
			push(@todo, map [$_ => $dep->{$_}, $pkg->{name} ], keys %$dep);
		    }
		}
	    }
	    else {
		push(@err,
		     "Can't find any package that provides $feature" .
		     ($want && $have ? " version $want" : "") .
		     ($needed_by ? " for $needed_by" : ""),
		);
	    }
        }
    }

    if (@err) {
	die join("\n", @err) unless $error_handler;
	$error_handler->(@err);
    }

    return $self->package_set_abs_ppd_uri(@pkg_missing);
}

sub is_downgrade {
    my($self, $pkg, $because, $needed_by) = @_;
    my @downgrade;
    for my $feature (sort keys %{$pkg->{provide}}) {
	next if $feature eq $pkg->{name};
	my $have = $self->feature_have($feature);
        push(@downgrade, [$feature, $have, $pkg->{provide}{$feature}])
	    if $have && $have > $pkg->{provide}{$feature};
    }
    if (@downgrade) {
	my $msg = "Installing " . $pkg->name_version;
	$msg .= " to get $because" if $because && $pkg->{name} ne $because;
	$msg .= " for $needed_by" if $needed_by;
	$msg .= " would downgrade " . join_with("and",
	    map "$_->[0] from version $_->[1] to $_->[2]", @downgrade
        );
	return $msg;
    }
    return "";
}

sub package {
    my $self = shift;
    return ActivePerl::PPM::RepoPackage->new_dbi($self->dbh, @_);
}

sub package_set_abs_ppd_uri {
    my($self, @pkgs) = @_;
    my %repo_cache;
    for my $pkg (@pkgs) {
	next if $pkg->{ppd_uri} && $pkg->{ppd_uri} =~ /^[a-zA-Z]\w+:/; # already absolute
	if (defined(my $repo_id = $pkg->{repo_id})) {
	    my($uri, $etag, $lastmod) = @{$repo_cache{$repo_id} ||= [$self->dbh->selectrow_array("SELECT packlist_uri, packlist_etag, packlist_lastmod FROM repo WHERE id = ?", undef, $repo_id)]};
	    if ($pkg->{ppd_uri}) {
		$pkg->{ppd_uri} = URI->new_abs($pkg->{ppd_uri}, $uri)->as_string;
	    }
	    else {
		$pkg->{ppd_uri} = $uri;
		$pkg->{ppd_etag} = $etag;
		$pkg->{ppd_lastmod} = $lastmod;
	    }
	}
    }
    return @pkgs;
}

sub _topo_sort {
    my %pkgs = map { $_->{name} => $_ } @_;
    my @res;
    _topo_visit($_, \%pkgs, \@res) for reverse @_;
    delete $_->{_topo_visited} for @_;
    return reverse @res;
}

sub _topo_visit {
    my($p, $pkgs, $res) = @_;
    return unless $p;
    return if $p->{_topo_visited}++;
    if (my $needed_by = $p->{_needed_by}) {
	for my $dep (@$needed_by) {
	    _topo_visit($pkgs->{$dep}, $pkgs, $res);
	}
    }
    push(@$res, $p);
}

sub install {
    my($self, %args) = @_;
    my $area = delete $args{area} || $self->default_install_area || die "No area";
    $area = $self->area($area) unless ref($area);
    die "Can't install into read-only area" if $area->readonly;

    my @pkgs = @{delete $args{packages} || []};
    die "No packages to install" unless @pkgs;

    my $relocate = $^O ne "MSWin32";
    $relocate = $args{relocate} if exists $args{relocate};

    my $install_html = $args{install_html};
    $install_html = $self->config_get('install_html') unless defined $install_html;
    $install_html = 1 unless defined $install_html;
    $install_html &&= eval { require ActivePerl::DocTools; };

    my $ua = web_ua();
    my $status = ppm_status();
    my $tmpdir = do { require File::Temp; File::Temp::tempdir("ppm-XXXXXX", TMPDIR => 1) };
    my $install_summary;
    eval {
	$self->package_set_abs_ppd_uri(@pkgs);

	# determine codebase_file
	for my $pkg (@pkgs) {
	    my $name = $pkg->name_version;
	    my $codebase = $pkg->codebase_abs;
	    die "No codebase for $name" unless $codebase;

	    if ($codebase =~ /\.(tgz|zip)$/) {
		$pkg->{codebase_type} = $1;
	    }
	    elsif ($codebase =~ /\.(?:tar\.gz|ppmx)$/) {
		$pkg->{codebase_type} = "tgz";
	    }
	    die "Don't know how to unpack $codebase" unless $pkg->{codebase_type};

	    if ($codebase->scheme eq 'file') {
		$pkg->{codebase_file} = $codebase->file;
		unless (-f $pkg->{codebase_file}) {
		    die "No file _at $pkg->{codebase_file}";
		}
	    }
	    else {
		local $ua->{progress_what} = "Downloading $name";
		my $save = $pkg->{codebase_file} = "$tmpdir/$name.$pkg->{codebase_type}";
		#print "\n    $codebase ==> $save...";
		my $res = $ua->get($codebase, ":content_file" => $save);
		die $res->status_line unless $res->is_success;
		if (my $len = $res->content_length) {
		    my $save_len = -s $save;
		    if ($save_len != $len) {
			die "Aborted download ($len bytes expected, got $save_len).\n";
		    }
		}
		# XXX An MD5 checksum for the tarball would be a good thing
	    }
	}

	# unpack
	for my $pkg (@pkgs) {
	    my $pname = $pkg->name_version;
	    $status->begin("Unpacking $pname");
	    my $codebase_file = $pkg->{codebase_file};

	    require ActiveState::ModInfo;
            require ActiveState::Version;
	    my $extract_file = sub {
		my($fname, $extractor) = @_;
		return if $fname =~ m,/\.exists$,;       # don't think these are needed
		return if $fname =~ m,^blib/html/(bin|site/lib)/,;  # will always regenerate these
		return if $fname =~ m,^blib/man1/, && !$area->man1;
		return if $fname =~ m,^blib/man3/, && !$area->man3;
		return if is_abs_path($fname);

		my $to = "$tmpdir/$pname/$fname";
		$extractor->($to) || die "Can't extract to $to";
		if ($fname =~ /\.pm$/) {
		    my $mod = $fname;
		    if ($mod =~ s,^blib/(?:lib|arch)/,,) {
			$mod = ActiveState::ModInfo::fname2mod($mod);
			$mod .= "::" unless $mod =~ /::/;
			$pkg->{provide}{$mod} = ActiveState::Version::vnumify(ActiveState::ModInfo::parse_version($to));
		    }
		}
		1;
	    };

	    if (eval { require Archive::Extract::Libarchive; 1 }) {
		my $ae = Archive::Extract::Libarchive->new(archive => $codebase_file);
		$ae->extract(to => "$tmpdir/$pname")
		    || die "Can't extract files from $codebase_file using libarchive";
		# Since Archive::Extract::Libarchive can't list the files until
		# after it has extracted them, we call &$extract_file with a dummy
		# extractor and delete those files we don't care about.
		for my $f (@{$ae->files}) {
		    $extract_file->($f, sub { 1 }) or unlink("$tmpdir/$pname/$f");
		}
            }
	    elsif ($pkg->{codebase_type} eq "tgz") {
		require Archive::Tar;
		my $tar = Archive::Tar->new($codebase_file, 1)
		    || die "Can't extract files from $codebase_file";
		for my $file ($tar->get_files) {
		    next unless $file->is_file;  # don't extract links and other crap
		    my $fname = $file->full_path;
		    $extract_file->($fname,
		        sub {$tar->extract_file($fname, $_[0])},
                    );
		}
	    }
	    elsif ($pkg->{codebase_type} eq "zip") {
		require Archive::Zip;
		my $zip = Archive::Zip->new($codebase_file)
		    || die "Can't extract files from $codebase_file";
		for my $file ($zip->members) {
		    next if $file->isDirectory;
		    $extract_file->($file->fileName,
		        sub {$file->extractToFileNamed($_[0]) == Archive::Zip::AZ_OK()},
		    );
		}
	    }
	    else {
		die "Don't know how to unpack $pkg->{codebase_type} files";
	    }

	    # If the same package is attempted installed multiple times,
	    # these might still be left after last attempt.
	    delete $pkg->{blib};
	    delete $pkg->{files};

	    my $blib = "$tmpdir/$pname/blib";
	    $pkg->{blib} = $blib if -d $blib;
	    $status->end;
	}

	# hashbang lines of scripts might need relocation
	if ($relocate) {
	    require ActiveState::RelocateTree;
	    my $ppm_sponge = ActiveState::RelocateTree::spongedir('ppm');
	    my $prefix = $self->{perl_config}{prefix};
	    for my $pkg (@pkgs) {
		next unless $pkg->{blib};
		$status->begin("Relocating " . $pkg->name_version);
		ActiveState::RelocateTree::relocate (
		    to      => $pkg->{blib},
		    inplace => 1,
	            search  => $ppm_sponge,
	            replace => $prefix,
                    quiet   => 1,
		);
		$status->end;
	    }
	}

	# generate HTML from the POD
	if ($install_html) {
	    require Cwd;
	    my $pwd = Cwd::cwd();
	    chdir($tmpdir) || die "Can't chdir $tmpdir: $!";
	    eval {
		for my $pkg (@pkgs) {
		    next unless $pkg->{blib};
		    my $pname = $pkg->name_version;
		    next unless -d $pname;
		    $status->begin("Generating HTML for $pname");
		    ActivePerl::DocTools::UpdateHTML_blib(verbose => 0, blib => "$pname/blib");
		    $status->end;
		}
	    };
	    chdir($pwd) || die "Can't chdir back to $pwd: $!";
	    die $@ if $@;
	}

	# install
	my $in_area = $area->name;
	$in_area = " in $in_area area" if $in_area;
	$status->begin("Updating files$in_area");
	$install_summary = $area->install(packages => \@pkgs, force => $args{force});
	if ($install_summary) {
	    $status->end;
	}
	else {
	    my $err = $@;
	    $status->end("failed");
	    die $err || "Install failed for unknown reason";
	}

	# run install scripts
	for my $pkg (_topo_sort(@pkgs)) {
	    my $pname = $pkg->name_version;
	    $pkg->run_script("install", $area, "$tmpdir/$pname",
		$install_summary->{pkg}{$pkg->{name}},
		$args{run_cb},
	    );
	}

	update_html_toc() if $install_html;
    };
    my $err = $@;
    require File::Path;
    File::Path::rmtree($tmpdir, 0);
    die $err if $err;

    return $install_summary;
}

sub profile_xml {
    my($self, %opt) = @_;
    my %seen;
    my @pkgs;

    for my $area_name ($self->areas) {
        next if $area_name eq "perl";
        my $area = $self->area($area_name);
        for my $pkg ($area->packages("name", "version")) {
            next if $seen{$pkg->[0]}++;
            push(@pkgs, $pkg);
        }
    }

    @pkgs = sort { lc($a->[0]) cmp lc($b->[0]) } @pkgs;

    my @out;
    push(@out, qq(<PPMPROFILE>\n));
    push(@out, qq(  <ACTIVEPERL));
    push(@out, sprintf qq( PRODUCT="%s"), ActivePerl::PRODUCT) if ActivePerl::PRODUCT ne "ActivePerl";
    push(@out, sprintf qq( VERSION="%s" PERL_VERSION="%s"), $self->{activeperl_build}, $self->{perl_version});
    push(@out, sprintf qq( PPM_VERSION="%s"), $ActivePerl::PPM::VERSION);
    push(@out, qq(/>\n));
    for my $id ($self->repos) {
        my $repo = $self->repo($id);
        push(@out, sprintf qq(  <REPOSITORY NAME="%s" HREF="%s"), xml_esc($repo->{name}), xml_esc($repo->{packlist_uri}));
        push(@out, qq( ENABLED="0")) unless $repo->{enabled};
        push(@out, qq(/>\n));
    }
    for (@pkgs) {
        my($name, $version) = @$_;
        push(@out, sprintf qq(  <SOFTPKG NAME="%s" VERSION="$version"/>\n), xml_esc($name), xml_esc($version));
    }
    push(@out, qq(</PPMPROFILE>\n));

    return join("", @out);
}

sub profile_xml_restore {
    my($self, $profile_xml, %opt) = @_;

    require ActivePerl::PPM::Profile;
    my $profile = ActivePerl::PPM::Profile->new($profile_xml);

    $opt{restore_repo} = 1 unless exists $opt{restore_repo};
    $opt{restore_pkgs} = 1 unless exists $opt{restore_pkgs};

    if ($opt{restore_repo}) {
        my %REPO;
        for my $id ($self->repos) {
            my $repo = $self->repo($id);
            $REPO{$repo->{packlist_uri}} = $repo->{enabled};
        }

        for ($profile->repositories) {
            if (exists $REPO{$_->{href}}) {
                ppm_log("INFO", "Repo $_->{href} already configured") if $opt{verbose};
                next;
            }
            if ($_->{href} =~ m,^http://ppm4.activestate.com/,) {
                $_->{href} = activestate_repo($self->{ppmarch}, $self->{activeperl_build});
                if (exists $REPO{$_->{href}}) {
                    ppm_log("INFO", "Repo $_->{href} already configured (arch)") if $opt{verbose};
                    next;
                }
            }
            my $id = $self->repo_add(
                name => $_->{name},
                packlist_uri => $_->{href},
		enabled => $_->{enabled},
            );

            $self->repo_enable($id, 0) unless $_->{enabled};
        }
    }

    if ($opt{restore_pkgs}) {
        my $area = $opt{area} || $self->default_install_area;
        $area = $self->area($area) unless ref($area);

        my @pkgs;
        my $skipped = 0;
        for ($profile->packages) {
            if ($area->package_have($_->{name}, $_->{version})) {
                ppm_log("DEBUG", "$_->{name}-$_->{version} already installed");
                $skipped++;
                next;
            }
            if (my $p = $self->package($_->{name}, $_->{version})) {
                push(@pkgs, $p);
            }
            else {
                if (my $p = $self->package_best($_->{name}, 0)) {
                    my $msg = "The repositories have " . $p->name_version . " instead of ";
                    $msg .= "$_->{name}-" if $_->{name} ne $p->name;
                    $msg .= "$_->{version}";
                    if ($area->package_have($p->name, $p->version)) {
                        $msg .= " (already installed)";
                        ppm_log("WARN", $msg);
                        $skipped++;
                        next;
                    }
                    ppm_log("WARN", $msg);
                    push(@pkgs, $p);
                }
                else {
                    ppm_log("WARN", "The repositories don't have $_->{name}");
                }
            }
        }

        $self->install(
            packages => \@pkgs,
            area => $area,
            relocate => 0,
        ) if @pkgs;
        ppm_log("INFO", "$skipped packages already installed") if $skipped && $opt{verbose};
    }
}

1;

__END__

=head1 NAME

ActivePerl::PPM::Client - Client class

=head1 SYNOPSIS

  my $ppm = ActivePerl::PPM::Client->new;

=head1 DESCRIPTION

The C<ActivePerl::PPM::Client> object ties together a set of install
areas and repositories and allow the installed packages to be managed.
The install areas are deducted from the values of C<@INC> when the
object is constructed.

The following methods are provided:

=over

=item $client = ActivePerl::PPM::Client->new

=item $client = ActivePerl::PPM::Client->new( $home_dir )

=item $client = ActivePerl::PPM::Client->new( %opt )

The constructor creates a new client based on the configuration found
in $home_dir which defaults to F<$ENV{HOME}/.ActivePerl> directory of the
current user.  If no such directory is found it is created.

Alternatively, key/value pairs to configure the client is passed in.  The following options are recognized:

=over

=item home => $home_dir

Directory where the client configuration database lives.

=item inc => \@array

Override the list of locations to initialize install areas from.

=item arch => $arch

Allow to override the architecture identification string used.  Mainly
userful for debugging.

=back

=item $client->arch

A string that identifies the architecture for the current perl.  This
must match the ARCHITECTURE/NAME attribute of PPDs for them to match.

=item $client->area( $name )

Returns an object representing the given named install area.  The
method will croak if no install area with the given $name is known.
The C<perl> and C<site> areas will always be available.  See
L<ActivePerl::PPM::InstallArea> for methods available on the returned
object.

=item $client->areas

Return list of available install area names.  The list is ordered to
match the corresponding entries in C<@INC>.

=item $client->default_install_area

Return the name of the area where installations should normally go.
Might return C<undef> if there is no appropriate default.

=item $value = $client->config_get( $key )

=item ($value1, $value2, ...) = $client->config_get( $key1, $key2, ...)

Read back one or more configuration values previosly saved.

=item ($key, $value, ...) = $client->config_list

=item ($key, $value, ...) = $client->config_list( $glob_pattern )

Return all key/value pairs where $key match the given $glob_pattern.
If $glob_pattern is missing return all key/value pairs.

=item $client->config_save( $key => $value )

=item $client->config_save( %pairs )

Will persistently store the given key/value pairs.  The values can be
extracted again with $client->config_get().

=item $client->repo( $repo_id )

Returns a reference to a hash describing the repo with the given
identifier.  The interesting fields of this hash are:

=over

=item name

The full (user friendly) name of the repository.  Can be modified by
$repo->repo_set_name().

=item enabled

A boolean that indicated if the repo is enabled or not.  Can be
modified by $repo->repo_enable().

=item pkgs

The number of packages provided by this repository.

=item packlist_uri

The URI that the PPM client will monitor for changes to the
repository.  This URI can denote a F<packlist.xml>, F<packlist.lst>
file or an HTML document with links to PPD files (typically a server
generated directory listing).  The URI can use any scheme that LWP
supports and can embed a username and password for HTTP using this syntax: C<http://user:pass@ppm.example.com/>.

=item packlist_last_status

The HTTP status code reported last time the PPM client tried to access
C<packlist_uri> document.

=item packlist_last_access

When did we last try to to access the C<packlist_uri> document.  The
value is seconds from epoch as for perl's time() function.

=item packlist_fresh_until

When do we need to refetch the C<packlist_uri> document. The
value is seconds from epoch as for perl's time() function.

=back

=item $client->repos

Returns list of available repo identifiers.

=item $client->repo_add( name => $name, packlist_uri => $uri )

Will add a new repository with the given attributes.  The method will
croak if a repository with the same C<packlist_uri> already exists.
The return value is the $repo_id of the new repository.

=item $client->repo_delete( $repo_id )

Will make the client forget about the given repository.

=item $client->repo_enable( $repo_id )

=item $client->repo_enable( $repo_id, $bool )

Makes it possible to enable and disable the given reposiory.  If $bool
is provided and is FALSE, then the repository is disabled.  The return
value is TRUE if the given repository was enabled.

=item $client->repo_set_name( $repo_id, $name )

Will update the name by which the given repo is known.

=item $client->repo_set_packlist_uri( $repo_id, $uri )

Will update the address of the packlist to monitor for the given
repository.  Will croak if the $uri is already used by some other
repo.

Updating the URI will loose all cached information about the repo.  A
new 'repo_sync' is needed to update this information.

=item $client->repo_sync( %args )

Will sync the local cache of packages from the enabled repositories.
Remote repositories are not contacted if the cache is not considered
stale yet.  The following options are recognized:

=over

=item force => $bool

If TRUE force state to be transfered again from remote repositories.
Make requests unconditional.

=item validate => $bool

If TRUE validate state from remote repositories even if state still
believed to be fresh.  Will still send conditional requests.

=item max_ppd => $num

If repository is an HTML document set limit for how many PPD links it
might contain.  The default is 100.

=item repo => $repo_id

Pass C<repo> with an identifier to only sync the given repo.

=back

=item $client->search( $pattern )

=item $client->search( $pattern, $field,... )

Will search for packages matching the given glob style $pattern.
Without further arguments this will return a list of package names.
With $field arguments it will return a list of array references, each
one filled in with the corresponding values for maching packages.

The supported field names are:

    id
    name
    version
    release_date
    author
    abstract
    ppd_uri
    ppd_etag
    ppd_fresh_until
    codebase
    repo_id
    cannot_install

See L<ActivePerl::PPM::RepoPackage> (and L<ActivePerl::PPM::Package>) for a
description of these fields.  The C<cannot_install> field is a boolean that
is TRUE for packages that can't be installed for some reason.

=item $client->search_lookup( $num )

Will look up the given package from the last search() result, where
$num matches the 1-based index into the list returned by the last
search.  This will return an L<ActivePerl::PPM::RepoPackage> object.

=item $client->packages

=item $client->packages( $field,... )

Without arguments returns the ids of packages available.  In scalar
context returns the number of packages.

With arguments return a list of array references each one representing
one package.  The elements of each array are the fields requested.
For the list of field names that can be used see the description of
the L</"search"> method above.

=item $client->package( $id )

=item $client->package( $name, $version )

Returns the L<ActivePerl::PPM::RepoPackage> object matching the
arguments or C<undef> if none match.  If $version is passed as C<undef>,
the package needs to be versionless to be returned.

=item $client->feature_best( $feature )

Returns the highest version number provided for the given feature by
the packages found in all enabled repositories.  The method return
C<undef> if no package provide this feature.

=item $client->package_best( $feature, $version )

Returns the best package of all enabled repositories that provide the
given feature at or better than the given version.

=item $client->feature_have( $feature )

=item $client->feature_have( $feature, @areas )

Returns the installed version number of the given feature.  Returns
C<undef> if none of the installed packages provide this feature.

If one or more @areas are provided, only look in the areas given by
these names.

=item $client->packages_depending_on( $pkg, $area )

Returns the packages (as C<ActivePerl::PPM::Package> objects) that
would "break" if the given package was uninstalled.  This means that
the returned packages are those that depend on features that the given
package provide.  In scalar context return number of packages.

The $pkg argument can be either a package name or a package object.

=item $client->packages_missing( %args )

Returns the list of packages to install in order to obtain the
requested features.  The returned list consist of
L<ActivePerl::PPM::RepoPackage> objects.  The attribute C<_wanted>
will be TRUE if a package was requested directly.  The attribute
C<_needed_by> will be an array reference of package names listing
packages having resolved dependencies on this package.  These
attributes do not exclude each other.

The returned list will be empty if all the requested features are
already installed.

The method will croak if nothing provides the requested features, if
dependencies can't be resolved or if the packages selected have
conflicting dependencies.

The arguments to the function are passed as key/value pairs:

=over

=item want => \@features

This is the list of features to resolve.  The elements can be plain
strings denoting feature names, or references to arrays containing a
$name, $version pair.  If $version is provided as C<undef> then this
is taken as an upgrade request and the function will try to find the
packages that provide the best possible version of this feature.

=item have => \@pkgs

List of packages you already have decided to install.  The function
will check if any of these packages provide needed features before
looking anywhere else.

=item want_deps => \@pkgs

Resolve any dependencies for the given packages.

=item area => \@areas

List of names of install areas to consider when determining if
requested features or dependencies are already installed or not.

=item force => $bool

If TRUE then return packages that provide the given features even if
they are already installed.  Will also disable check for downgrades.

=item follow_deps => $str

In what way should packages dependencies be resolved.  The provided
$str can take the values C<all>, C<missing>, or C<none>.  The default
is C<missing>.  If $str is C<all> then dependent packages are returned
even if they are already installed.  If $str is C<missing> then only
missing dependencies are returned.  If $str is C<none> then
dependencies are ignored.

=item error_handler => \&callback

Call the given error handler instead of croaking in the case of
trouble.  Error messages are provided as argument.  There can be more
than one.

Providing an error_handler allow the function to return missing
packages for working dependencies even if not all dependencies worked
out.

=back

=item $client->install( %args )

Will download, unpack and install the given packages.  The function
will raise an exception of it gets into trouble, otherwise it will
return

The arguments to the function are passed as key/value pairs:

=over

=item packages => \@pkgs

Mandatory argument that provide the packages to install.  The array
passed should contain C<ActivePerl::PPM::Package> objects.

=item area => $area

What install area to install into.  If not provided, then
$client->default_install_area is used.

=item run_cb => \&func

A callback function that should behave like &ActivePerl::Run::run
which will be called to execute the commands of the post install
script.  If not provided, then &ActivePerl::Run::run will be used.

=back

=item $client->profile_xml

Returns an XML document that describes the configured repositories and
the installed packages.

=item $client->profile_xml_restore( $profile_xml, %opt )

Will try to restore the repositories and packages described by the
passed in XML document.  The document should be one generated by the
profile_xml method.  The following options are recognized:

=over

=item restore_repo => $bool

Pass a FALSE value to suppress adding the repositories found in the
profile document.

=item restore_pkgs => $bool

Pass a FALSE value to suppress installation of the packages listed in
the profile document that are missing.

=item verbose => $bool

Log extra information about the steps taken when TRUE.

=item area => $area_name

Which install area to install into.  If not provided, then
$client->default_install_area is used.

=back

=back

=head1 BUGS

none.
