package ActiveState::CPAN;

use strict;
use Carp ();
use ActiveState::Handy qw(file_content);
use ActiveState::CPAN::Utils qw($PKG_EXT);
require IO::Handle;  # for flush

our $VERSION = "1.01";

my $ASINTRA = gethostbyname("nas1.activestate.com");

# cache (file system, partial mirror)
# mirror
# backpan

# know about files that update (databases) and files that don't (uploaded stuff)

sub new {
    my $class = shift;
    my %opt = @_;

    my @mirrors = (
        'http://ppm.activestate.com/CPAN/',
        'http://cpan.perl.org/',

        # an then a few hardcoded places to look for local mirrors of CPAN
        (
            $^O eq "MSWin32" ? (
                "C:\\CPAN",
                "C:\\Perl\\CPAN",
            ) : (
                ($ASINTRA ? ('/data/cpan', '/net/nas1/cpan') : ()),
                '/opt/perl/CPAN/',
                '/usr/local/share/CPAN/',
                '/usr/local/CPAN/',
            )
        ),
    );

    my $opt_mirror = delete $opt{mirror};
    if ($opt_mirror) {
        unshift(@mirrors, $opt_mirror);
    }
    elsif ($ENV{CPAN_ROOT}) {
        unshift(@mirrors, $ENV{CPAN_ROOT});
    }

    my $local_mirror;
    my @remote_mirrors;
    for my $m (@mirrors) {
        $m .= "/" unless $m =~ m,/$,;
        if ($m =~ /^([a-zA-Z][a-zA-Z0-9.+\-]+):/ && lc($1) ne "file") {
            push(@remote_mirrors, $m);
        }
        elsif (!$local_mirror) {
            if ($m =~ s,^file:///,,) {
                $m = "/$m";
            }
            $local_mirror = $m if -f $m . "authors/01mailrc.txt.gz";
        }
    }

    my $cache = delete $opt{cache};
    if ($cache && !-d $cache) {
        require File::Path;
        $cache = undef unless File::Path::mkpath($cache);
    }
    if ($cache) {
        $cache .= "/" unless $cache =~ m,/$,;
    }

    my $backpans;
    if (exists $opt{backpan}) {
        $backpans = delete $opt{backpan} || [];
	$backpans = [$backpans] unless ref($backpans) eq "ARRAY";
        for my $m (@$backpans) {
	    $m .= "/" unless $m =~ m,/$,;
        }
    }
    else {
	$backpans = [
             "http://backpan.cpantesters.org/",
             "http://backpan.cpan.org/",
        ];
    }

    my $verbose = delete $opt{verbose};
    $verbose = 1 unless defined($verbose);

    if ($^W && %opt) {
        Carp::carp("Unrecognized option '$_'") for sort keys %opt;
    }

    my $self = bless {
        cache => $cache,
        remote_mirrors => \@remote_mirrors,
        local_mirror => $local_mirror,
        backpans => $backpans,
        verbose => $verbose,
    }, $class;
    return $self;
}

sub clear_cache {
    my $self = shift;
    return unless $self->{cache};
    require File::Path;
    if (opendir(my $dh, $self->{cache})) {
        while (defined(my $f = readdir($dh))) {
            next if $f eq "." || $f eq "..";
            File::Path::rmtree("$self->{cache}$f");
        }
    }
}

sub _cache_expire {
    my($self, $path) = @_;
    return 4 if $path =~ m,^indices/, || $path =~ m,^(?:authors|modules)/\d\d\D,;
    return undef if $path =~ m,^authors/id/, || $path =~ m,^src/perl[^/]+\z,;  # never expires
    return 24;
}

sub local_mirror {
    my $self = shift;
    return $self->{local_mirror};
}

sub author {
    my($self, $id) = @_;
    return $self->authors->{$id};
}

sub authors {
    my $self = shift;
    return $self->{authors} ||= do {
        my $fh = $self->open("authors/01mailrc.txt.gz");
        my %authors;
        while (<$fh>) {
            if (/^alias\s+(\S+)\s+\"([^\"]+)\"/) {
                $authors{$1} = $2;
            }
        }
        close($fh);
        \%authors;
    };
}

sub packages {
    my $self = shift;
    my $iter = $self->packages_iter(@_);
    my @pkgs;
    while (my $path = $iter->()) {
        push(@pkgs, scalar(ActiveState::CPAN::Utils::distname_info($path)));
    }
    return @pkgs;
}

sub packages_iter {
    my($self, %opt) = @_;

    my $indexed = delete $opt{indexed};
    my $recent = delete $opt{recent};

    if ($^W && %opt) {
        Carp::carp("Unrecognized option '$_'") for sort keys %opt;
    }

    my $iter;
    if ($indexed && $recent) {
        # Hmm, this whole thing is a bit expensive

        # First extract and store the indexed packages
        my $next = $self->packages_iter(indexed => 1);
        my %indexed;
        keys %indexed = 20000;  # preextend
        while (my $f = $next->()) {
            $indexed{$f} = undef;
        }
        # warn scalar(keys %indexed), " indexed packages\n";

        # To sort them by date we need to look that field
        my @mtime_path;
        $next = $self->files_iter(package => 1);
        while (my($path, undef, $mtime) = $next->()) {
            push(@mtime_path, "$mtime $path") if exists $indexed{$path};
        }

        if (@mtime_path != keys %indexed) {
            warn "Can't look up mtime for ", (keys(%indexed) - @mtime_path), " indexed packages"
        }

        # sort and eliminate the time stamp
        @mtime_path = sort @mtime_path;
        substr($_, 0, 16, "") for @mtime_path;

        $iter = sub { pop(@mtime_path); };
    }
    elsif ($indexed) {
        my $fh = $self->open("modules/02packages.details.txt.gz");
        my %seen;
        $iter = sub {
            while (defined(my $line = readline($fh))) {
                if ($line =~ m,^([a-zA-Z_]\w*(?:::\w+)*)\s+(\S+)\s+([A-Z]/\S+),) {
                    my $pkg = $3;
                    next if $seen{$pkg}++;
                    next unless $pkg =~ $PKG_EXT;
                    return "authors/id/" . $pkg;
                }
            }
            return;
        };
    }
    else {
        $iter = $self->files_iter(
            package => 1,
            ($recent ? (order_by => "mtime desc") : ())
        );
    }

    return sub {
        my $path = $iter->() || return;
        return $path unless wantarray;
        return ActiveState::CPAN::Utils::distname_info($path);
    }
}

sub package_info {
    shift;
    goto &ActiveState::CPAN::Utils::distname_info;
}

sub modules_iter {
    my $self = shift;
    my $fh = $self->open("modules/02packages.details.txt.gz");

    # skip header
    while (defined(my $line = readline($fh))) {
        last if $line =~ /^$/;
    }

    return sub {
        while (defined(my $line = readline($fh))) {
            my($module, $version, $package) = split(' ', $line);
            return $module unless wantarray;
            $version = undef if $version eq "undef";
            $package = "authors/id/" . $package;
            return ($module, $version, $package);
        }
        return;
    };
}

sub files_iter {
    my($self, %opt) = @_;

    my $order = lc(delete $opt{order_by} || "path");
    if ($order ne "path") {
        my @files;
        my $iter = $self->files_iter(%opt);
        while (my @f = $iter->()) {
            push(@files, \@f);
        }
        #print scalar(@files), " files buffered\n";
        my $desc = $order =~ s/\s+desc\s*$//;
        if ($order eq "path") {
            # already sorted
        }
        elsif ($order eq "size") {
            @files = sort { $a->[1] <=> $b->[1] } @files;
        }
        elsif ($order eq "mtime") {
            @files = sort { $a->[2] cmp $b->[2] } @files;
        }
        else {
            Carp::croak("Unrecognized sort field '$order'");
        }
        return $desc ?
            sub {
                return @files ? pop(@files)->[0] : undef unless wantarray;
                return @{shift(@files)};
            } :
            sub {
                return @files ? shift(@files)->[0] : undef unless wantarray;
                return @{shift(@files)};
            };
    }

    my $matching = delete $opt{matching};
    my $package = delete $opt{package};
    my $author = delete $opt{author};
    if ($author) {
        my $a = substr($author, 0, 1);
        my $aa = substr($author, 0, 2);
        $author = qr,^authors/id/$a/$aa/$author/,;
    }

    if ($^W && %opt) {
        Carp::carp("Unrecognized option '$_'") for sort keys %opt;
    }

    my $fh = $self->open("indices/find-ls.gz");
    return sub {
        while (defined(my $line = readline($fh))) {
            my(undef, undef, $mode, undef, undef, undef, $size, $mtime, $path) =
                split(' ', $line);
            next if $mode eq "l";
            next if $mode =~ /^d/;
            next if $path =~ /~\z/;
            next if $matching && $path !~ $matching;
            next if $author && $path !~ $author;
            next if $package && ($path !~ m,^authors/id/, || $path !~ $PKG_EXT);
            return $path unless wantarray;
            $mtime =~ s/\./T/; # ISO time
            return ($path, $size, $mtime);
        }
        close($fh);
        return;
    };
}

sub files {
    my $self = shift;
    my @files;
    my $iter = $self->files_iter(@_);
    while (my $f = $iter->()) {
        push(@files, $f);
    }
    return @files;
}

sub get {
    my($self, $path) = @_;
    if (my $file = $self->get_file($path)) {
        if ($file =~ /\.gz/) {
            require Compress::Zlib;
            return Compress::Zlib::memGunzip(file_content($file));
        }
        else {
            return file_content($file);
        }
    }
    unless ($self->{cache}) {
        if (my $res = $self->_get_remote($path)) {
            if ($path =~ /\.gz$/) {
                require Compress::Zlib;
                return Compress::Zlib::memGunzip($res->content);
            }
            else {
                return $res->content;
            }
        }
    }
    return undef;
}

sub get_file {
    my($self, $path) = @_;

    if ($self->{cache}) {
        my $file = $self->{cache} . $path;
        if (-f $file) {
            my $exp = "$file.expire";
            return $file if !-f $exp || time < (stat _)[9];
        }
    }

    if (my $dir = $self->{local_mirror}) {
        my $file = $dir . $path;
        return $file if -f $file;
    }

    if ($self->{cache}) {
        my $file = $self->{cache} . $path;
        (my $dir = $file) =~ s,[^/]+$,,;
        unless (-d $dir) {
            require File::Path;
            File::Path::mkpath($dir) || die "Can't mkpath $dir: $!";
        }
        if ($self->_get_remote($path, $file)) {
            if (my $h = $self->_cache_expire($path)) {
                my $exp = "$file.expire";
                file_content($exp, "");
                my $t = time + $h * 3600;
                utime $t, $t, $exp;
            }
            return $file;
        }
    }

    return undef;
}

sub unpack {
    my($self, $path, $dir) = @_;
    unless ($dir) {
        $dir = $path;
        $dir =~ s/$PKG_EXT//;
        $dir =~ s,.*/,,;
        die "Assert" unless $dir;
    };
    my $file = $self->get_file($path);
    die "Can't obtain file reference for $path" unless $file;
    mkdir($dir, 0755) || die "Can't mkdir $dir: $!";
    eval {
        print "Unpacking $file..." if $self->{verbose};
        if ($file =~ /\.(?:tar\.gz|tgz)\z/) {
            require Archive::Tar;
            require Cwd;

            my $tar = Archive::Tar->new;
            $tar->read($file, 1);
            my $cwd = Cwd::cwd();
            chdir($dir) or die "Can't chdir into $dir: $!";
            if ($^O eq "MSWin32") {
                # XXX The code in Archive::Tar that converts symlinks to ordinary copies
                # seems to be broken, so don't extract symlinks for now.
                # Also man pages containing "::" in the filename are invalid on Windows.
                $tar->extract(
                    grep { !$_->is_symlink && $_->full_path !~ /:/ } $tar->get_files
	        );
            }
            else {
                $tar->extract();
            }
            chdir($cwd) or die "Can't chdir into $cwd: $!";;
        }
        elsif ($file =~ /\.zip/) {
            require Archive::Zip;
            require Cwd;
            my $zip = Archive::Zip->new;
            $zip->read(Cwd::abs_path($file)) == Archive::Zip::AZ_OK() or die "Can't read zip file $file";
            my $cwd = Cwd::cwd();
            chdir($dir) or die "Can't chdir into $dir: $!";
            $zip->extractTree() == Archive::Zip::AZ_OK() or die "Can't unzip $file into $dir";
            chdir($cwd) or die "Can't chdir into $cwd: $!";
        }
        else {
            die "Don't know how to unpack $file, stopped";
        }
        print "done\n" if $self->{verbose};

        opendir(my $dh, $dir) || die "Can't opendir $dir: $!";
        my @f = grep !/^\.\.?\z/, readdir($dh);
        closedir($dh);
        if (@f == 1) {
            my $d2 = "$dir/$f[0]";
            if (opendir($dh, $d2)) {
                # move content up one level
                while (defined(my $f = readdir($dh))) {
                    next if $f =~ /^\.\.?\z/;
                    rename("$d2/$f", "$dir/$f") || die "Can't move $d2/$f up: $!";
                }
                closedir($dh);
                rmdir($d2) || die "Can't remove $d2: $!";
            }
        }
    };

    if ($@) {
        my $err = $@;
        require File::Path;
        File::Path::rmtree($dir);
        die $err;
    }
    return $dir;
}

sub _user_agent {
    my $self = shift;
    return $self->{user_agent} ||= do {
        require LWP::UserAgent;
        return ActiveState::CPAN::UA->new($self);
    };
}

sub _get_remote {
    my($self, $path, $save_as) = @_;

    my $ua = $self->_user_agent;
    unless ($self->{local_mirror}) {
        for my $base (@{$self->{remote_mirrors}}) {
            my $uri = $base . $path;
            my $res = $save_as ? $ua->save($uri, $save_as) : $ua->get($uri);
            return $res if $res->is_success;
            last if $res->code == 404;
        }
    }

    for my $base (@{$self->{backpans}}) {
	my $uri = $base . $path;
	my $res = $save_as ? $ua->save($uri, $save_as) : $ua->get($uri);
	return $res if $res->is_success;
	last if $res->code == 404;
    }

    return undef;
}

sub open {
    my($self, $path) = @_;
    if (my $file = $self->get_file($path)) {
        if ($file =~ /\.gz$/) {
            open(my $fh, "gzip -cd $file|") || die "Can't gunzip $file: $!";
            return $fh;
        }
        else {
            open(my $fh, "<", $file) || die "Can't open $file: $!";
            return $fh;
        }
    }
    unless ($self->{cache}) {
        if (my $res = $self->_get_remote($path)) {
            my $content = $res->content;
            if ($path =~ /\.gz$/) {
                require Compress::Zlib;
                $content = Compress::Zlib::memGunzip($content);
            }
            open(my $fh, "<", \$content) || die "Can't open in-memory-buffer";
            return $fh;
        }
    }
    die "Can't open $path: No such file or directory";
}

package ActiveState::CPAN::UA;

our @ISA = qw(LWP::UserAgent);

use Time::HiRes qw(time);

sub new {
    my($class, $mgr) = @_;
    my $self = $class->SUPER::new(
        agent => "ActiveState::CPAN/$ActiveState::CPAN::VERSION ",
        env_proxy => 1,
        keep_alive => 1,
    );
    $self->{cpan_verbose} = $mgr->{verbose};
    return $self;
}

sub save {
    my($self, $uri, $file) = @_;
    my $part = "$file.part";
    my $res = $self->get($uri, ":content_file" => $part);
    if ($res->is_success) {
        my $len = $res->content_length;
        if (defined($len) && $len != -s $part) {
            $res->code(550);
            $res->message("Incomplete download");
            unlink($part);
            return $res;
        }
        rename($part, $file) || die "Can't replace $file: $!";
        if (my $mtime = $res->last_modified) {
            utime($mtime, $mtime, $file);
        }
    }
    return $res;
}

sub progress {
    my($self, $status, $res) = @_;
    return unless $self->{cpan_verbose};
    if ($status eq "begin") {
        # XXX Should fix LWP to pass in the request here!!!
        print "==> ";
        $self->{cpan_start} = time;
        $self->{cpan_lastp} = "";
    }
    elsif ($status eq "end") {
        my $req = $res->request;
        print "\r", $req->method, " ", $req->uri, " ==> ", $res->status_line;
        my $t = time - $self->{cpan_start};
        require ActiveState::Duration;
        print " (", ActiveState::Duration::dur_format_sm($t), ")\n";
    }
    elsif ($status eq "tick") {
        print "#";
    }
    else {
        my $p = sprintf "%4.0f%%", $status * 100;
        return if $p eq $self->{cpan_lastp};
        print "$p\b\b\b\b\b";
        $self->{cpan_lastp} = $p;
    }
    STDOUT->flush;
}

1;

=head1 NAME

ActiveState::CPAN - Get information and files from CPAN

=head1 SYNOPSIS

 use ActiveState::CPAN ();
 my $cpan = ActiveState::CPAN->new;

=head1 DESCRIPTION

ActiveState::CPAN provides an interface for fetching files off CPAN
and for extracting information from the various meta and index files.
The following methods are provided:

=over

=item $cpan = ActiveState::CPAN->new( %options )

This constructs a new ActiveState::CPAN object.  The following options are recognized:

=over

=item mirror => $url_or_path

Give the URL of the CPAN mirror to fetch files from.  The module works
best with a local CPAN mirror and this option might also be given as a
path to the local mirror.  If not provided, then the CPAN_ROOT
environment variable is consulted, and finally a set of hardcoded URLs
are used.

=item cache => $path

The cache is a directory containing a partial mirror of CPAN.  If
files are requested from remote mirrors or backpan they will be stored
in the cache and served back from here the next time they are
requested.

You need to specify a cache if you rely on $cpan->get_file() to return
file system path names for all CPAN paths.

=item backpan => $url

Give the URL of the backpan server to use to fetch files that have
expired from CPAN.  The default is "http://backpan.cpantesters.org/".

An explict C<undef> can be passed to disable the fallback
on Backpan.

=item verbose => $bool

If TRUE print trace messages to STDOUT about operations are taken,
like downloads from remote servers.  Default is TRUE.

=back

=item $cpan->clear_cache

This will delete all the files in the cache directory.  Use with care.
This is a noop if you did not pass the C<cache> option to the
constructor.

=item $cpan->local_mirror

Returns the file system path to the local mirror used.  Returns
C<undef> if there is no local mirror.

=item $cpan->author( $author_id )

Returns the email alias for the given CPAN author.  The alias is on the form:

    Gisle Aas <gisle@aas.no>

=item $cpan->authors

Returns a reference to a hash that maps author ids to email aliases.

=item $cpan->packages( %opt )

This returns the list of packages on CPAN.  The packages are returned
as a reference to a hash with the following keys:

=over

=item name

This is the bare name of the package.  It's a string like "libwww-perl".

=item version

This is the version number of the package.  It's a string like "5.812".

=item maturity

The maturity of the distribution. This will be either "released" or "developer".

=item author

This is the CPAN author id owning the package.  It's a string like "GAAS".

=item extension

This is the file suffix of the package file.  It's a string like "tar.gz"

=item path

This is the CPAN relative path of the package file.  It's a string
like "authors/id/G/GA/GAAS/libwww-perl-5.812.tar.gz".

=back

The passed in options determine what packages are returned.  The
recognized options are:

=over

=item indexed => 1

If passed with a TRUE value only list packages with indexed
modules (as determined by the CPAN indexer).

=item recent => 1

The most recently uploaded packages are returned first.

=back

=item $cpan->packages_iter( %opt )

This returns an iterator that returns the packages on CPAN.  The
iterator returns the name, version, maturity, author, extension and
path of the package.  In scalar context the path is returned.

The recognized options are the same as for packages() described above.

=item $cpan->package_info( $path )

Returns a hash reference like the ones that package() returns.  In
list context it returns separate values like package iterator does.

=item $cpan->modules_iter()

This returns an iterator that returns the indexed modules on CPAN. The
iterator returns module name, module version and the CPAN relative
package path.  In scalar context the module name is returned.

=item $cpan->files( %opt )

This returns the list of path names of the files on CPAN.  Symlinks
are not returned.  The options passed in can be used to select what
path names are returned.  The recognized options are:

=over

=item matching => qr/.../

Only list path names that match the given regular expression.

=item package => 1

If passed with a TRUE value only list package files, also called CPAN
distributions.  These have normally names that end in F<.tar.gz>.

=item author => $author_id

Only list files uploaded by the given CPAN author.  The $author_id is
a string like "GAAS".

=item order_by => $field

=item order_by => "$field desc"

Return the files sorted by the given field, which can be one of
"path", "size", "mtime".  Append the string " desc" to the field name
to sort in descending order.  For example:

   order_by => "mtime desc"

will return the most recently uploaded files first.

=back

=item $cpan->files_iter( %opt )

This returns an iterator that returns the files on CPAN. Symlinks are
not returned.  The iterator returns the path name, the size and the
last modified timestamp.  In scalar context only the path name is returned.

The recognized options are the same as for files() described above.

The timestamp is in ISO 8601 compact format: YYYYMMDDThhmmss (with a literal "T").

=item $cpan->get( $path )

Returns the full content of the given file or undef if the file can't
be found.  Compressed files are automatically uncompressed.

=item $cpan->get_file( $path )

Returns the file sytem path that corresponds to the given CPAN path.
This will either be the path to a local CPAN mirror or a path within
the cache.  If there is no cache configured, then this function might
return C<undef>.

=item $cpan->open( $path )

Opens the given CPAN file and returns a file handle to it.  Croaks if
the file can't be found or opened.  Compressed files are automatically
uncompressed.

=item $cpan->unpack( $path )

=item $cpan->unpack( $path, $dir )

Will unpack a CPAN package to the given directory.  If $dir isn't
provided it defaults to the basename of $path.  It returns the path to
the unpacked directory ($dir) and croaks if it gets into trouble.

=back

=head2 Iterators

The methods with names that end with C<_iter> return iterators.
Iterators are functions that return the next element in a sequence
each time they are called, and return nothing once the sequence is
exhausted.  Example usage:

    my $next = $cpan->files_iter(author => "GAAS", package => 1);
    while (my($path, $size, $mtime) = $next->()) {
        print "$path $size $mtime\n";
    }

More information about iterators at L<Iterator::Simple>.  This module
also contains some utilities for wrapping and combining iterators.

=head1 ENVIRONMENT

If the CPAN_ROOT environment variable is set it will be used as the
primary mirror.  It can be an URL or the name of a directory.

=head1 BUGS

none.

=cut
