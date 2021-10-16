package ActivePerl::PPM::InstallArea;

use strict;
use Config qw(%Config);
use Carp qw(croak);
use ActiveState::ModInfo qw(fname2mod parse_version);
use ActiveState::Version qw(vnumify);
use ActiveState::Path qw(join_path abs_path);
use File::Compare ();
use File::Basename ();

use ActivePerl::PPM::Package ();
use ActivePerl::PPM::Logger qw(ppm_log ppm_status ppm_debug);

use base 'ActivePerl::PPM::DBH';


sub new {
    my $class = shift;
    unshift(@_, "name") if @_ == 1;
    my %opt = @_;
    my $name = delete $opt{name} || "";
    my $config = $opt{perl_config} || \%Config;

    my %dirs;
    if ($name eq "perl") {
	%dirs = (
            prefix => $config->{prefix},
	    archlib => $config->{archlib},
            lib => $config->{privlib},
	    bin => $config->{bin},
            script => $config->{scriptdir},
            man1 => $config->{man1dir},
            man3 => $config->{man3dir},
            html => $config->{installhtmldir},   # XXX ActivePerl hack
	);
    }
    elsif ($name eq "site" || $name eq "vendor") {
	my $prefix = $config->{"${name}prefix"}
	    || croak("No $name InstallDirs configured for this perl");
	%dirs = (
	    prefix => $prefix,
	    archlib => $config->{"${name}arch"},
            lib => $config->{"${name}lib"},
	    bin => $config->{"${name}bin"},
            script => $config->{"${name}script"},
            man1 => $config->{"${name}man1dir"},
            man3 => $config->{"${name}man3dir"},
            html => $config->{installhtmldir},   # XXX ActivePerl hack
	);
    }
    else {
	my $prefix = delete $opt{prefix}
	    || croak("Neither well known name nor prefix specified");
	%dirs = (
	    prefix => $prefix,
	    archlib => $opt{archlib},
            lib => $opt{lib},
	    bin => $opt{bin},
            script => $opt{script},
            man1 => $opt{man1},
            man3 => $opt{man3},
	    html => $opt{html},
	    etc => $opt{etc},
	);
    }

    # defaults
    $dirs{etc} ||= $ENV{ACTIVEPERL_PPM_HOME};
    die "No prefix" unless $dirs{prefix};
    for my $d (qw(bin lib etc html)) {
	$dirs{$d} ||= "$dirs{prefix}/$d";
    }
    $dirs{archlib} ||= $dirs{lib};
    $dirs{script} ||= $dirs{bin};

    # cleanup
    for my $d (keys %dirs) {
	delete $dirs{$d} unless defined($dirs{$d}) && length($dirs{$d});
    }
    if ($^O eq "MSWin32") {
	s,\\,/,g for values %dirs;
    }

    my $self = bless {
        name => $name,
        dirs => \%dirs,
        autoinit => $opt{autoinit},
    }, $class;
    $self->dirty_cleanup;
    return $self;
}

sub name {
    my $self = shift;
    $self->{name};
}

sub prefix {
    my $self = shift;
    $self->{dirs}{prefix};
}

sub archlib {
    my $self = shift;
    $self->{dirs}{archlib};
}

sub lib {
    my $self = shift;
    $self->{dirs}{lib};
}

sub bin {
    my $self = shift;
    $self->{dirs}{bin};
}

sub script {
    my $self = shift;
    $self->{dirs}{script};
}

sub man1 {
    my $self = shift;
    $self->{dirs}{man1};
}

sub man3 {
    my $self = shift;
    $self->{dirs}{man3};
}

sub html {
    my $self = shift;
    $self->{dirs}{html};
}

sub etc {
    my $self = shift;
    $self->{dirs}{etc};
}

sub packages {
    my $self = shift;
    my $dbh = eval { $self->dbh };
    return wantarray ? () : undef unless $dbh;
    if (@_) {
	return @{$dbh->selectall_arrayref("SELECT " . join(",", @_) .
					  " FROM package ORDER BY name")};
    }
    return @{$dbh->selectcol_arrayref("SELECT name FROM package ORDER BY name")}
	if wantarray;
    return $dbh->selectrow_array("SELECT count(*) FROM package");
}

sub packlists {
    my $self = shift;
    my %pkg;
    my $archlib = $self->archlib;
    my $auto = "$archlib/auto";
    require File::Find;
    File::Find::find(sub {
	return unless $_ eq ".packlist";
	my $pkg = substr($File::Find::name, length($auto) + 1);
	substr($pkg, -(length(".packlist")+1)) = "";
	$pkg =~ s,/,-,g,
        $pkg{$pkg} = $File::Find::name;
    }, $auto) if -d $auto;
    if (-f "$archlib/.packlist") {
	$pkg{"Perl"} = "$archlib/.packlist";
    }
    return wantarray ? (keys %pkg) : \%pkg;
}

sub inc {
    my $self = shift;
    my @inc;
    push(@inc, $self->archlib);
    my $lib = $self->lib;
    push(@inc, $lib) unless $lib eq $inc[0];
    return @inc;
}

sub verify {
    my($self, %opt) = @_;
    my $dbh = $self->dbh;
    my $pkg = delete $opt{package};
    my $file_cb = delete $opt{file_cb};
    my $badfile_cb = delete $opt{badfile_cb};
    my $pkg_id;
    if ($pkg) {
	$pkg_id = $self->package_id($pkg);
	croak("Package $pkg is not known") unless defined($pkg_id);
    }
    my $sth = $dbh->prepare("SELECT path, md5, mode FROM file" .
        (defined($pkg_id) ? " WHERE package_id = $pkg_id" : "") .
	" ORDER BY path");
    $sth->execute;
    my %status = (
        verified => 0,
    );
    $status{id} = $pkg_id if $pkg_id;
    while (my($path, $md5, $mode) = $sth->fetchrow_array) {
	$path = $self->_expand_path($path);
	&$file_cb($path, $md5, $mode) if $file_cb;
	if (my $info = _file_info($path)) {
	    if (defined($mode) && $mode != $info->{mode}) {
		&$badfile_cb("wrong_mode", $path, $info->{mode}, $mode) if $badfile_cb;
		$status{wrong_mode}++;
	    }
	    if (defined $md5 && $md5 ne $info->{md5}) {
		&$badfile_cb("modified", $path, $info->{md5}, $md5) if $badfile_cb;;
		$status{modified}++;
	    }
	}
	else {
	    &$badfile_cb("missing", $path, $info->{md5}, $info->{mode}) if $badfile_cb;;
	    $status{missing}++;
	}
	$status{verified}++;
    }

    wantarray ? %status : !($status{wrong_mode} || $status{modified} || $status{missing});
}

sub package_id {
    my $self = shift;
    my $pkg = shift;
    my $dbh = $self->dbh;
    my $id = $dbh->selectrow_array("SELECT id FROM package WHERE name = ?", undef, $pkg);
    if (!defined($id) && @_) {
	my %opt = @_;
	if ($opt{sloppy}) {
	    # Since the package name is always a feature there is no need for
	    # caseless search for it explictly
	    my $ids = $dbh->selectcol_arrayref("SELECT package_id FROM feature WHERE lower(name) = lower(?) AND role = 'p'", undef, $pkg);
	    if (@$ids > 1) {
		my @p = map $dbh->selectrow_array("SELECT name FROM package WHERE id = ?", undef, $_), @$ids;
		die "The name $pkg is ambiguous; please select one of " . join(", ", @p);
	    }
	    $id = $ids->[0];
	}
    }
    return $id;
}

sub package {
    my $self = shift;
    my $id = shift;
    unless ($id =~ /^\d+$/) {
	$id = $self->package_id($id, @_);
	return undef unless defined($id);
    }
    return ActivePerl::PPM::Package->new_dbi($self->dbh, $id);
}

sub package_have {
    my($self, $name, $version) = @_;
    return scalar($self->dbh->selectrow_array(
        "SELECT count(*) FROM package WHERE name = ? AND version = ?", undef,
        $name, $version,
    ));
}

sub package_files {
    my($self, $id) = @_;
    return $self->dbh->selectrow_array("SELECT count(*) FROM file WHERE package_id = ?", undef, $id)
	unless wantarray;
    return map $self->_expand_path($_), @{$self->dbh->selectcol_arrayref("SELECT path FROM file WHERE package_id = ? ORDER BY path", undef, $id)}
}

sub file_owner {
    my($self, $path) = @_;
    $path = $self->_relative_path(abs_path($path));
    return $self->dbh->selectrow_array("SELECT package_id FROM file WHERE path = ?", undef, $path);
}

sub package_packlist {
    my($self, $id) = @_;
    my $dbh = $self->dbh;
    my $packlist = $dbh->selectrow_array("SELECT path FROM file WHERE package_id = ? AND path like '%/.packlist'", undef, $id);
    $packlist = $self->_expand_path($packlist) if $packlist;
    return $packlist;
}

sub feature_have {
    my($self, $feature) = @_;
    my $vers = $self->dbh->selectrow_array("SELECT max(version) FROM feature WHERE name = ? AND role = 'p'", undef, $feature);
    $vers = "0E0" if defined($vers) && !$vers;  # ensure a TRUE value
    return $vers;
}

sub install {
    my $self = shift;
    @_ = (packages => [@_]) if @_ && ref($_[0]);  # legacy
    my %args = @_;
    my @packages = @{delete $args{packages} || []};

    # check packages and default file based on blib
    croak("No packages to install") unless @packages;
    for my $pkg (@packages) {
	croak("Missing package name") unless $pkg->{name};
	if (my $blib = $pkg->{blib}) {
	    for my $d (qw(arch archlib lib bin script man1 man3 html)) {
		next unless -d "$blib/$d";
		next if $d =~ /^man/ && !$self->{dirs}{$d};
		my $dd = $d;
		$dd = "archlib" if $dd eq "arch";  # :-(
		$pkg->{files}{"$blib/$d"} = "$dd:";
	    }
	}
    }

    my $dbh = $self->dbh;
    require ExtUtils::Packlist;
    die "Can't install into read-only area"
	if $self->{readonly};

    # do install
    my %state = (
        dbh => $dbh,
	self => $self,
        pkg_id => undef,
        force => $args{force},
        commit => [],
        rollback => [],
	old_files => {},
	summary => {},
    );
    local $dbh->{AutoCommit} = 0;
    eval {
	my $dirty = $self->_dirty_file;
	die "Previous install did not clean up properly" if -e $dirty;
	{
	    open(my $fh, ">", $dirty) || die "Can't create '$dirty': $!";
	    print $fh "$$\n";
	    close($fh) || die "Can't write to '$dirty': $!";
	    ppm_debug("Created $dirty");
	}
	_on_rollback(\%state, "unlink", $dirty);
	for my $pkg (@packages) {
	    $pkg = ActivePerl::PPM::Package->new($pkg);
	    $state{summary}{pkg}{$pkg->{name}}{new_version} = $pkg->{version};
	    my $pkg_id = $self->package_id($pkg->{name});
	    if (defined $pkg_id) {
		for (@{$dbh->selectcol_arrayref("SELECT path FROM file where package_id = $pkg_id")}) {
		    $state{old_files}{$_}++;
		}
	        $dbh->do("DELETE FROM file WHERE package_id = $pkg_id");
		$pkg->{id} = $pkg_id;
		my $old_pkg = $self->package($pkg_id);
		$state{summary}{pkg}{$pkg->{name}}{old_version} = $old_pkg->{version};
            }
	    else {
		delete $pkg->{id};  # might be left over from the RepoPackage
	    }
	    $state{pkg_id} = $pkg_id = $pkg->dbi_store($dbh);
	    $state{pkg_name} = $pkg->{name};
	    $state{packlist} = ExtUtils::Packlist->new;

	    ppm_log("NOTICE", "Installing $pkg->{name} with id $pkg_id");

	    my $files = $pkg->{files};
	    next unless $files;
	    for my $from (sort keys %$files) {
		die "There is no '$from' to install from" unless -l $from || -e _;
		my $to = $self->_expand_path($files->{$from});
		ppm_debug("Copy $from --> $to");
		if (-d _) {
		    die "Can't install a directory on top of $to"
			if -e $to && !-d _;
		    for ($from, $to) {
			$_ .= "/" unless m,/\z,;
		    }
		    _copy_dir(\%state, $from, $to);
		}
		elsif (-f _) {
		    _copy_file(\%state, $from, $to);
		}
		else {
		    die "Can't install $from since it's neither a regular file nor a directory";
		}
	    }

	    # write .packlist
	    (my $packlist_pkg = $pkg->{name}) =~ s,-,/,g;
	    my $packlist_file = $self->_expand_path("archlib:auto/$packlist_pkg/.packlist");
	    my $packlist_dir = File::Basename::dirname($packlist_file);
	    _mk_path(\%state, $packlist_dir);
	    if (-e $packlist_file) {
		my $bak = "$packlist_file.ppmbak";
		die "Can't save to $bak since it exists" if -e $bak;
		rename($packlist_file, $bak) || die "Can't rename as $bak: $!";
		_on_rollback(\%state, "rename", $bak, $packlist_file);
		_on_commit(\%state, "unlink", $bak);
	    }
	    $state{packlist}->write($packlist_file) || die "Can't write '$packlist_file': $!";
	    $state{summary}{pkg}{$pkg->{name}}{packlist} = $packlist_file;
	    _on_rollback(\%state, "unlink", $packlist_file);
	    _save_file_info(\%state, $packlist_file);
	}
	for (keys %{$state{old_files}}) {
	    _on_commit(\%state, "unlink", $self->_expand_path($_));
	    $state{summary}{count}{deleted}++;
	}
	_on_commit(\%state, "unlink", $dirty);  # must be last
	die "Giving up" if our $FAIL_AT_END_OF_INSTALL;  # hook for testing
    };

    if ($@) {
	ppm_log("ERR", "Rollback $@");
	$dbh->rollback;
	_do_action(reverse @{$state{rollback}});
	return undef;
    }
    else {
	ppm_log("NOTICE", "Commit install");
	$dbh->commit;
	_do_action(@{$state{commit}});
	return $state{summary} || {};
    }
}

sub dirty_cleanup {
    my $self = shift;
    my $dirty = $self->_dirty_file;
    if (-e $dirty && time - (stat _)[9] > 60) {
	# The dirty flag file is more than a minute old.  Likely to be
	# left over from a crashed install.
	my $prefix = $self->prefix;
	ppm_log("WARN", "Cleaning up dirty install attempt in $prefix");
	my $count = 0;
	require File::Find;
	File::Find::find(sub {
	    return unless /\.ppmbak\z/;
	    my $base = substr($File::Find::name, 0, -7);
	    unlink($base);
	    rename("$base.ppmbak", $base)
		|| die "Can't restore $base: $!";
	    ppm_log("WARN", "$base restored");
	    $count++;
	}, $prefix);
	unlink($dirty) || die "Can't unlink '$dirty': $!";
	my $s = ($count == 1) ? "" : "s";
	$count ||= "no";
	ppm_log("WARN", "$count file$s needed to be restored");
    }
}

sub _do_action {
    for my $action (@_) {
	my($op, @args) = @$action;
	ppm_debug("$op @args");
	if ($op eq "rmdir") {
	    for my $d (@args) {
		rmdir($d) || ppm_log("WARN", "Can't rmdir($d): $!");
	    }
	}
	elsif ($op eq "unlink") {
	    # Some platforms (HP-UX) cannot delete in-use executables
	    # and will produce "Text file busy" (ETXTBSY) warnings
	    # here.  So make it clear this is "just" a warning.
	    unlink(@args) || ppm_log("WARN", "Can't unlink(@args): $!");
	}
	elsif ($op eq "rename") {
	    rename($args[0], $args[1]) || ppm_log("WARN", "Can't rename(@args): $!");
	}
	else {
	    # programmer error
	    die "Don't know how to '$op'";
	}
    }
}

sub _on_rollback {
    my $state = shift;
    push(@{$state->{rollback}}, [@_]);
}

sub _on_commit {
    my $state = shift;
    push(@{$state->{commit}}, [@_]);
}

sub _copy_file {
    my($state, $from, $to) = @_;

    my $copy_to = $to;
    if (-e $to) {
	if (-f _ &&
            ((stat _)[2] & 07777) == ((stat $from)[2] & 07777) &&  # same mode
            File::Compare::compare($from, $to) == 0)               # same content
        {
	    $copy_to = undef;
	    $state->{summary}{count}{unchanged}++;
	    ppm_log("INFO", "$to already present");
	}
	else {
	    my $bak = "$to.ppmbak";
	    die "Can't save to $bak since it exists" if -e $bak;
	    rename($to, $bak) || die "Can't rename as $bak: $!";
	    _on_rollback($state, "rename", $bak, $to);
	    _on_commit($state, "unlink", $bak);
	    $state->{summary}{count}{updated}++;
	}
    }
    else {
	$state->{summary}{count}{installed}++;
    }

    if ($copy_to) {
	open(my $in, "<", $from) || die "Can't open $from: $!";
	binmode($in);

	my $out;
	open($out, ">", $copy_to) || do {
	    my $err = $!;
	    my $dirname = File::Basename::dirname($copy_to);
	    _mk_path($state, $dirname);
	    if (open($out, ">", $copy_to)) {
		$err = undef;
	    }
	    die "Can't create $copy_to: $err" if $err;
	};
	binmode($out);
	_on_rollback($state, "unlink", $copy_to);

	my $n;
	my $buf;
	while ( ($n = read($in, $buf, 4*1024))) {
	    print $out $buf;
	}

	die "Read failed for file $from: $!"
	    unless defined $n;

	unless (($^O eq "MSWin32")) {
	    chmod((stat $in)[2] & 07777, $out);  # copy mode
	}

	close($in);
	close($out) || die "Write failed for file $copy_to";
	if (($^O eq "MSWin32")) {
	    chmod((stat $from)[2] & 07777, $copy_to);  # copy mode
	}

	ppm_log("INFO", "$copy_to written");
    }

    $state->{packlist}{$to}++;
    _save_file_info($state, $to);
}

sub _save_file_info {
    my($state, $path) = @_;
    my $rpath = $state->{self}->_relative_path($path);
    my $info = _file_info($path) || die "Whoa '$path' missing: $!";

    delete $state->{old_files}{$rpath};
    eval {
	$state->{dbh}->do("INSERT INTO file (package_id, path, md5, mode) VALUES (?, ?, ?, ?)", undef, $state->{pkg_id}, $rpath, $info->{md5}, $info->{mode});
    };
    if ($@) {
	my $err = $@;
	my $name = $state->{dbh}->selectrow_array("SELECT name FROM package, file WHERE package.id = package_id AND file.path = ?", undef, $rpath);
	die $err unless $name;
	if ($state->{force}) {
	    $state->{dbh}->do("INSERT OR REPLACE INTO file (package_id, path, md5, mode) VALUES (?, ?, ?, ?)", undef, $state->{pkg_id}, $rpath, $info->{md5}, $info->{mode});
	    ppm_log("WARN", "File conflict for '$path'; file owned by package $name overwritten by package $state->{pkg_name}");
	}
	else {
	    die "File conflict for '$path'.
    The package $name has already installed a file that package $state->{pkg_name}
    wants to install."
	}
    }
}

sub _copy_dir {
    my($state, $from, $to) = @_;

    _mk_path($state, $to);

    opendir(my $dh, $from) || die "Can't opendir $from: $!";
    my @files = sort readdir($dh);
    closedir($dh);

    for my $f (@files) {
	next if $f eq "." || $f eq ".." || $f eq ".exists" || $f =~ /~\z/;
	my $from_file = "$from$f";
	my $to_file = "$to$f";
	if (-l $from_file) {
	    die "Can't copy link $from_file";
	}
	elsif (-f _) {
	    _copy_file($state, $from_file, $to_file);
	}
	elsif (-d _) {
	    _copy_dir($state, "$from_file/", "$to_file/");
	}
	else {
	    die "Don't know how to copy $from_file";
	}
    }
}

sub _mk_path {
    my($state, $dir) = @_;
    return if -d $dir;
    my $parent = File::Basename::dirname($dir);
    die "$dir isn't a directory" if $parent eq $dir;  # recusion safety
    _mk_path($state, $parent);

    mkdir($dir, 0755) || die "Can't mkdir $dir: $!";
    _on_rollback($state, "rmdir", $dir);
}

sub uninstall {
    my $self = shift;
    my $pkg = shift;
    my $pkg_id = $self->package_id($pkg);
    unless (defined $pkg_id) {
	die "Package $pkg isn't installed";
    }

    # XXX check if removing this package would break any dependencies

    # Delete the files
    my $dbh = $self->dbh;
    die "Can't uninstall from read-only area"
	if $self->{readonly};
    local $dbh->{AutoCommit} = 0;

    my $sth = $dbh->prepare("SELECT path FROM file WHERE package_id = ?");
    $sth->execute($pkg_id);
    my %dir;
    while (my($path) = $sth->fetchrow_array) {
	$path = $self->_expand_path($path);
	if (unlink($path)) {
	    ppm_log("NOTICE", "rm $path");
	    $dir{File::Basename::dirname($path)}++;
	}
	else {
	    ppm_log("WARN", "Can't remove $path: $!");
	}
    }

    # Clean up any directories that ended up empty
    while (%dir) {
	# Process the directory with the longest name in each round, as
	# this ensures that we don't try to remove same directory
	# more than once.
	my $dir = (sort {length($b) <=> length($a)} keys %dir)[0];
	delete $dir{$dir};
	next if grep $dir eq $_, values %{$self->{dirs}}; # never delete any of our roots
	last if length($dir) <= length($self->{dirs}{prefix});  # safety net
	# Rely on rmdir() failing for non-empty directories
	if (rmdir($dir)) {
	    ppm_log("NOTICE", "rmdir $dir");
	    $dir{File::Basename::dirname($dir)}++
	}
	else {
	    ppm_log("WARN", "rmdir $dir: $!")
		unless $!{ENOTEMPTY};
	}
    }

    # Prune the database
    $dbh->do("DELETE FROM file WHERE package_id = ?", undef, $pkg_id);
    $dbh->do("DELETE FROM feature WHERE package_id = ?", undef, $pkg_id);
    $dbh->do("DELETE FROM script WHERE package_id = ?", undef, $pkg_id);
    $dbh->do("DELETE FROM package WHERE id = ?", undef, $pkg_id);
    $dbh->commit;
}

sub _init_db {
    my $self = shift;

    my $etc = $self->etc;
    my $db_file = "ppm-area.db";
    my $name = $self->name;
    $db_file = "ppm-$name-area.db" if $name;
    $db_file = "$etc/$db_file";
    unless (-f $db_file) {
	unless ($self->{autoinit}) {
	    my $msg = "Uninitialized install area ";
	    $msg .= $name ? $name : " _at " . $self->prefix;
	    #require Carp; Carp::confess($msg);
	    die $msg;
	}

	unless (-d $etc) {
	    require File::Path;
	    File::Path::mkpath($etc) || die "Can't mkpath($etc): $!";
	}
    }

    require DBI;
    my $dbh = DBI->connect("dbi:SQLite:dbname=$db_file", "", "", {
        AutoCommit => 1,
        RaiseError => 1,
        sqlite_use_immediate_transaction => 0,
    });
    die "$db_file: $DBI::errstr" unless $dbh;
    $self->{dbh} = $dbh;

    local $dbh->{AutoCommit} = 0;
    my $v = $dbh->selectrow_array("PRAGMA user_version");
    die "Assert" unless defined $v;
    if ($v == 0) {
	ppm_log("WARN", "Setting up schema for $db_file");
	_init_ppm_schema($dbh);
	$dbh->do("PRAGMA user_version = 1");
	$dbh->commit;
	$self->sync_db;
	return;
    }
    if ($v != 1) {
	die "Unrecognized database schema $v for $db_file";
    }

    # check if we have opened a readonly database based on technique
    # suggested in http://article.gmane.org/gmane.comp.db.sqlite.general/5171
    local $dbh->{RaiseError} = 0;
    local $dbh->{PrintError} = 0;
    unless ($dbh->do("UPDATE package SET rowid=0 WHERE 0")) {
	$self->{readonly}++;
    }
    elsif ($name =~ /^(perl|site)$/ && !$ENV{ACTIVEPERL_PPM_SETUP_TIME}) {
	# hack to make perl & site autosync when something else has installed into it
	my $mtime = (stat $db_file)[9];
	my $perllocal = "$Config{privlib}/perllocal.pod";
	if ($mtime < ((stat $perllocal)[9] || 0)) {
	    # perllocal.pod has been modified after the database file was
	    $self->sync_db;
	    # ensure that it's modified even when sync_db found nothing to do
	    my $time = time;
	    utime $time, $time, $db_file;
	}
    }
}

sub initialize {
    my $self = shift;
    local $self->{autoinit} = 1;
    delete $self->{dbh_err};
    return !!$self->dbh;
}

sub initialized {
    my $self = shift;
    return !!eval { $self->dbh };
}

sub readonly {
    my $self = shift;
    return 1 unless eval { $self->dbh };  # need to try to open database to know
    return $self->{readonly};
}

sub _init_ppm_schema {
    my $dbh = shift;
    for my $create (ActivePerl::PPM::Package->sql_create_tables(name_unique => 1)) {
	$dbh->do($create) || die "Can't create database table";
    }
    $dbh->do(<<'EOT');
CREATE TABLE IF NOT EXISTS file (
    package_id integer,
    path text unique not null,
    md5 char(32),
    mode integer
)
EOT
}

sub sync_db {
    my($self, %opt) = @_;
    my $dbh = $self->dbh;
    local $dbh->{AutoCommit} = 0;
    local $dbh->{PrintError} = 0;  # the Perl package might have conflicting files
    my $name = $self->name || "unnamed";
    ppm_status("begin", "Syncing $name PPM database with .packlists");
    my $unchanged = 0;
    require ExtUtils::Packlist;
    my $pkglists = $self->packlists;
    for my $pkg (sort keys %$pkglists) {
	my $id = $dbh->selectrow_array("SELECT id FROM package WHERE name = ?", undef, $pkg);
	if (defined $id) {
	    my $md5 = $dbh->selectrow_array("SELECT md5 FROM file WHERE package_id = ? AND path LIKE '%/.packlist'", undef, $id);
	    if ($md5 && $md5 eq _file_md5($pkglists->{$pkg})) {
		# packlist unchanged, so there is a good change package is too
		# but let's also check the main module file if present
		my $changed = 0;

		my $mainmod_fname = $pkg;
		$mainmod_fname =~ s,-,/,g;
		$mainmod_fname .= ".pm";
		my($mainmod_path, $mainmod_md5) = $dbh->selectrow_array("SELECT path,md5 FROM file WHERE package_id = ? AND (path LIKE ? or path LIKE ?)", undef, $id, "%/$mainmod_fname", "%:$mainmod_fname");
		if ($mainmod_path) {
		    $mainmod_path = $self->_expand_path($mainmod_path);
		    $changed++ if $mainmod_md5 ne _file_md5($mainmod_path);
		}
		unless ($changed) {
		    $unchanged++;
		    next;
		}
	    }
	    ppm_log("INFO", "Package $pkg: updated");
	}
	else {
	    $dbh->do("INSERT INTO package (name) VALUES (?)", undef, $pkg);
	    $id = $dbh->func('last_insert_rowid'); #$dbh->last_insert_id;
	    ppm_log("INFO", "Package $pkg: created");
	}

	my $pkglist = ExtUtils::Packlist->new($pkglists->{$pkg});
	$dbh->do("DELETE FROM file WHERE package_id = ?", undef, $id);
	$dbh->do("DELETE FROM feature WHERE package_id = ? AND role = 'p'", undef, $id);
	$dbh->do("INSERT INTO feature (package_id, name, version, role) VALUES (?, ?, 0, 'p')", undef, $id, $pkg);
	for my $f ($pkglists->{$pkg}, sort keys %$pkglist) {
	    my $path = $self->_relative_path($f);
	    my $info = _file_info($f);
	    unless ($info) {
		ppm_log("ERR", "Package $pkg: File $f missing\n");
		next;
	    }
;	    unless (eval { $dbh->do("INSERT INTO file (package_id, path, md5, mode) VALUES (?, ?, ?, ?)", undef, $id, $path, $info->{md5}, $info->{mode}) })
	    {
		my $owner = $dbh->selectrow_array("SELECT package.name FROM package, file WHERE package.id = file.package_id AND file.path = ?", undef, $path);
		if ($pkg eq "Perl") {
		    # no problem
		    ppm_debug("Package $owner: Have overwritten Perl's $f");
		    next;
		}
		if ($owner eq "Perl") {
		    ppm_debug("Package $pkg: Have overwritten Perl's $f");
		    $dbh->do("UPDATE file SET package_id = ?, md5 = ?, mode = ? WHERE path = ?", undef,
			     $id, $info->{md5}, $info->{mode}, $path);
		    next;
		    
		}
		ppm_log("ERR", "Package $pkg: File conflict for $f already owned by $owner");
		next;
	    }

	    if ($f =~ /\.pm$/) {
		my $mod = $f;
		$mod =~ s,\\,/,g if $^O eq "MSWin32";
		$mod =~ s,^$self->{dirs}{archlib}/,, or
		    $mod =~ s,^$self->{dirs}{lib}/,,;
		$mod = fname2mod($mod);
		my $vers = eval { parse_version($f) };
		unless ($opt{keep_package_version}) {
		    (my $mod_pkg = $mod) =~ s/::/-/g;
		    if ($mod_pkg eq $pkg && defined($vers)) {
			$dbh->do("UPDATE package SET version = ? WHERE id = ?", undef, "$vers", $id);
		    }
		}
		$mod .= "::" unless $mod =~ /::/;
		$dbh->do("INSERT INTO feature (package_id, name, version, role) VALUES(?, ?, ?, ?)", undef, $id, $mod, vnumify($vers), "p");
	    }
	}
	$dbh->commit;
    }

    # check if any registered packages are now gone
    for my $pkg ($self->packages) {
	next if $pkglists->{$pkg};  # already processed
	my %info = $self->verify(package => $pkg);
	if ($info{verified} && $info{verified} == ($info{missing} || 0)) {
	    # all files has been deleted, nuke package
	    die "Assert" unless $info{id};
	    ppm_log("NOTICE", "Package $pkg: deleted");
	    $dbh->do("DELETE FROM file WHERE package_id = ?", undef, $info{id});
	    $dbh->do("DELETE FROM feature WHERE package_id = ?", undef, $info{id});
	    $dbh->do("DELETE FROM script WHERE package_id = ?", undef, $info{id});
	    $dbh->do("DELETE FROM package WHERE id = ?", undef, $info{id});
	    $dbh->commit;
	}
	else {
	    ppm_log("WARN", "The $pkg package is missing its .packlist");
	}
    }
    ppm_log("NOTICE", "$unchanged packages found up-to-date")
	if $unchanged;
    ppm_status("end");
}

sub _dirty_file {
    my $self = shift;
    my $n = $self->name;
    my $base = $n ? "ppm-$n-dirty" : "ppm-dirty";
    return $self->_expand_path("etc:$base");}

sub _relative_path {
    my($self, $path) = @_;
    $path =~ s,\\,/,g if $^O eq "MSWin32";
    $path =~ s,^\Q$self->{dirs}{prefix}\E/,prefix:,;
    return $path;
}

sub _expand_path {
    my($self, $path) = @_;
    if ($path =~ s/^([a-z][a-z\d]+)://) {
	my $d = $1;
	die "No $d dirs configured" unless exists $self->{dirs}{$d};
	$path = join_path($self->{dirs}{$d}, $path);
	$path =~ s,\\,/,g if $^O eq "MSWin32";
    }
    return $path;
}

sub _file_info {
    my $file = shift;
    open(my $fh, "<", $file) || return undef;
    binmode($fh);
    my %info;

    @info{qw(dev ino mode nlink uid gid rdev size atime mtime ctime blksize blocks)} = stat($fh);
    $info{mode} &= 07777;

    require Digest::MD5;
    $info{md5} = Digest::MD5->new->addfile($fh)->hexdigest;

    return \%info;
}

sub _file_md5 {
    my $file = shift;
    if (my $info = _file_info($file)) {
	return $info->{md5};
    }
    return "";
}

1;

__END__

=head1 NAME

ActivePerl::PPM::InstallArea - Perl installation area

=head1 SYNOPSIS

  my $area = ActivePerl::PPM::InstallArea->new("site");
  # or
  my $area = ActivePerl::PPM::InstallArea->new(prefix => "$ENV{HOME}/perl");

=head1 DESCRIPTION

An C<ActivePerl::PPM::InstallArea> object provide an interface to a
Perl install area.  Different install areas might have different
protection policies and each contain a set of installed packages and
modules.  The concept is the same as C<INSTALLDIRS> provided by
L<ExtUtils::MakeMaker>.

An install area is divided into the following directories:

=over 8

=item lib

This is where architecture neutral modules go.  Packages that
are implemented in pure perl are installed here.

=item archlib

This is where architecture specific modules go.  Packages that are
implemented using XS code are installed here.  For ActivePerl this
will normally be the same as C<lib>.

=item script

This is where architecture neutral programs go.

=item bin

This is where architecture specific programs go.  For ActivePerl this
will normally be the same as C<script>.

=item etc

This is where configuration files go.

=item man1

This is where Unix style manual pages describing programs go.

=item man3

This is where Unix style manual pages describing modules go.

=item html

This is where HTML files go.

=item prefix

This just provide a prefix for the install area as a whole.  All paths
above should be at or below C<prefix>.

=back

The following methods are provided:

=over

=item $area = ActivePerl::PPM::InstallArea->new( $name )

=item $area = ActivePerl::PPM::InstallArea->new( %opts )

Constructs a new C<ActivePerl::PPM::InstallArea> object.  If constructed
based on $name, then the constructor might croak if no
install area with the given name is known.  The "perl" and "site" install areas
are always available.  Some perls might also have a "vendor" install area.

Alternatively the directories to use can be specified directly by
passing them as key/value pair %opts.  Only C<prefix> is mandatory.
All other directories are derived from this, except for the C<man*>
directories will only set up if specified explicitly.

The option C<autoinit> will if TRUE make the install area call
$self->initialize automatically when some method need access to the
database.

=item $area->name

Returns the name.  This returns the empty string for nameless I<InstallArea>.

=item $area->prefix

=item $area->archlib

=item $area->lib

=item $area->bin

=item $area->script

=item $area->man1

=item $area->man3

=item $area->html

=item $area->etc

Returns the corresponding path.

=item $area->inc

Returns the list of directories to be pushed onto perl's @INC for the
current install area.

=item $area->initialized

Returns TRUE if this area has been initialized.  If C<autoinit> was
specified for the constructor, then this method might have the side
effect of actually initializing the database, in which case this
returns TRUE.

=item $area->initialize

Set up the database used to track packages for the install area if not
already set up.  This invokes sync_db() if the database was created.

Most methods will croak unless the install area has been initialized.
Exceptions are name(), readonly(), initialized(), packages() and the
directory accessors (like lib(), script(),...).

The C<autoinit> option can be specified for the constructor to make
the database be automatically set up during the first method call
that needs it.

=item $area->readonly

Returns TRUE if it is not possible to install or remove packages from
the area.  This is usually caused by the user not having permission
to modify the files of the area.

This also returns TRUE for unintialized install areas.

=item $area->install( packages => [\%pkg1, \%pkg2, ...] )

Install the given list of packages as one atomic operation.  The
function returns TRUE if all packages installed or FALSE if
installation failed.

Each package to be installed is described by a hash reference (or an
L<ActivePerl::PPM::Package> object) with the following elements:

=over

=item name => $name

The name of the package.  If a package with the given name is already
installed, then it will replaced with the new package.  This is the
only mandatory attribute.

=item version => $version

The version identifier for the given package.

=item author => $string

Who the current maintainer of the package is.  Should normally be on
the form "Givenname Lastname <user@example.com>".

=item abstract => $string

A short sentence describing the purpose of the package.

=item blib => $path

Pick up files to install from the given I<blib> style directory.  The
codebase directory of PPD packages is usually a tarball of this stuff.

=item files => \%hash

A hash describing files and directories to install.  The keys are
where to copy files from and the values are install locations.  The
install locations selects what type of directory to install into by
prefixing them with an dir identifier followed by a colon.  Example:

   files => {
      Foo => "archlib:Foo",
      "Bar.pm" => "lib:Bar.pm"
   }

This will install the "Foo" directory into the archlib area and the
"Bar.pm" module into the lib area.

=back

=item $area->uninstall( $name )

Removes the given package and its installed files.  Croaks if no such
package was installed in the first place.  Uninstalling a package
might break other packages that depended on features this package
provided.

=item $area->verify( %opts )

Verify that the files of the installed packages are still present and
unmodified.

In scalar context returns TRUE if all files where still found good.
In array context return key/value pairs suitable for assignment to a
hash.  The C<verified> value is the number of files checked.  The
C<missing>, C<modified>, C<wrong_mode> tally the files found to be
missing, modified or chmoded.

The following options are recognized:

=over

=item package => $name

Only verify the given package.

=item file_cb => \&sub

Function called back for each file visited.  The function is called
with 3 arguments; the file name, expected md5 checksum and expected
file mode.

=item badfile_cb => \&sub

Function called back each time a bad file is found.  The first
argument is what kind of badness (same as the status keys in the
return value), the second is the file name and the addtional info
varies depending on kind.

=back

=item $area->packages( @fields )

Without arguments returns the sorted list of names of packages
currently installed.  In scalar context returns the number of packages
installed, or C<undef> if database has not been initialized.

With arguments return a list of array references each one representing
an installed package.  The elements of each array are the fields
requested.  The list will be sorted by package name.  See
L<ActivePerl::PPM::Package> for what field names are available.

=item $area->package( $id )

=item $area->package( $name )

=item $area->package( $name, sloppy => 1 )

Return an package object (see L<ActivePerl::PPM::Package>) for the
given package.  Returns C<undef> if no such package is installed.

If no package match the specified name exactly and C<sloppy> is
specified then search again ignoring case and even search for features
provided that match name.  The method will croak if this extended
search end up matching multiple packages.

=item $area->package_id( $name )

Returns the internal identifier for the given package.  The package
name much match exactly; case matters.  Returns C<undef> if no such
package is installed.  This is the cheapest way to check if a package
is installed.

=item $area->package_id( $name, sloppy => 1 )

Find package even if the name does not match exactly.  The package
will be found if the name match without regard to case or if it
provide the given name as a feature.  Will croak if multiple packages
match.

=item $area->package_have( $name, $version )

Returns TRUE if the package with the given name and version number
is installed.

=item $area->feature_have( $feature )

If one of the installed packages provide the given feature, then the
feature version number is returned.  The method returns C<undef> if no
package provide the given feature.

=item $area->package_files( $id )

Returns the list of names for the files that belong to the given
package.  In scalar context return the number of files.

=item $area->file_owner( $path )

Return the $id if the package that owns the given file, or C<undef> if
the file is not tracked by this install area.

=item $area->package_packlist( $id )

Returns the F<.packlist> file for the given package.  See
L<ExtUtils::Packlist>.

=item $area->packlists

Returns the list of packages that have F<.packlist> files installed.
In scalar context return a hash reference; the keys are package names
and the values are full paths to the corresponding F<.packlist> file.
This will also pick up packages installed by other means that by PPM.
See L<ExtUtils::Packlist> for more information about these files.  PPM
does not use F<.packlist> files to track the files installed by the
packages it manage, but it keeps them in sync for other tools that
manage modules.

=item $area->sync_db( %opt )

Synchronize the state of the PPM database with what modules seems to
be installed in the directories of the current install area.  Packages
where all files are gone will also be deleted from the PPM database.

The following options are recognized:

=over

=item C<keep_package_version> => $bool

If TRUE don't try to update the package version from the version
number of the module with the same name as the pacakge.

=back

=back

=head1 SEE ALSO

L<ActivePerl::PPM::Package>, L<ExtUtils::Packlist>.

=head1 BUGS

none.
