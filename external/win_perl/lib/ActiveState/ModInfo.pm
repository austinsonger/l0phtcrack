package ActiveState::ModInfo;

use strict;

# XXX Setting our $VERSION here breaks the "eval $eval" in parse_version().
# our $VERSION = '1.02';

use base 'Exporter';
our @EXPORT_OK = qw(
    list_modules
    mod2fname fname2mod
    find_inc find_module
    open_inc open_module
    fixup_module_case
    parse_version
    module_version
);

use File::Find qw(find);

sub list_modules {
    my %opts = @_;
    my @inc = @{
	delete $opts{inc} || do {
	    require Config;
	    my $prefix = _canon_file($Config::Config{prefix});
	    [grep _canon_file($_) =~ /^\Q$prefix\E/o, @INC]
	}
    };
    my %inc = map { _canon_file($_) => 1 } @inc;

    my $namespace = delete $opts{namespace};
    my $namespace_re;
    $namespace_re = $namespace =~ /::\z/ ? qr/^\Q$namespace\E/ : qr/^\Q$namespace\E::/
	if $namespace;

    my $maxdepth = delete $opts{maxdepth};
    $maxdepth = 6 unless defined $maxdepth;
    my $allowdup = delete $opts{allowdup};

    if ($^W && %opts) {
	require Carp;
	Carp::carp("Unrecognized option '$_'") for sort keys %opts;
    }

    my @modules;
    my %seen;
    for my $dir (@inc) {
        next unless -d $dir;
	find(sub {
	    if ($File::Find::name ne $dir && $inc{_canon_file($File::Find::name)}) {
		#warn "Will not traverse $File::Find::name from $dir\n";
		$File::Find::prune = 1;
		return;
	    }
	    if (/^\w+\.pm$/ && -f $_) {
		my $fname = substr($File::Find::name, length($dir));
		$fname =~ s,^/,,;
		my $mod = fname2mod($fname) || die;
		return if $namespace && !($mod eq $namespace || $mod =~ $namespace_re);
		return if !$allowdup && $seen{$mod}++;
		push(@modules, $mod, $File::Find::name);
	    }
	    elsif (/^\w+$/ && -d $_) {
		my $n = substr($File::Find::name, length($dir));
		$n =~ s,^/,,;
		if ($n eq "auto" || $n eq "Tk/demos" || $n =~ /^\d/) {
		    $File::Find::prune = 1;
		    return;
		}
		if ($maxdepth && ($n =~ tr[/][] >= $maxdepth - 1)) {
		    #warn "Pruning $n in $dir";
		    $File::Find::prune = 1;
		    return;
		}
	    }
	    elsif ($_ ne ".") {
		#warn "Can't form a module name [$_]";
		$File::Find::prune = 1;
		return;
	    }
	}, $dir);
    }

    return @modules;
}

sub _canon_file {
    my $f = shift;
    if ($^O eq "MSWin32") {
	$f = Win32::GetLongPathName($f);
	$f = lc($f);
	$f =~ s,\\,/,g;
    }
    return $f;
}

sub mod2fname {
    my $mod = shift;
    $mod =~ s,::\z,,;
    $mod =~ s,::,/,g;
    $mod .= ".pm";
    return $mod;
}

sub fname2mod {
    my $name = shift;
    return undef unless $name =~ s/\.pm\z//;
    $name =~ s,[/\\],::,g;
    return $name;
}

sub find_inc {
    my $fname = shift;
    my $inc = shift || \@INC;
    for (@$inc) {
	my $f = "$_/$fname";
	$f =~ s,\\,/,g if $^O eq "MSWin32";
	return $f if -f $f;
    }
    return undef;
}

sub open_inc {
    my $fname = shift;
    my $inc = shift || \@INC;
    for (@$inc) {
	my $f = "$_/$fname";
	if (open(my $fh, "<", $f)) {
	    return $fh if -f $fh;
	}
    }
    return undef;
}

sub find_module {
    return find_inc(mod2fname(shift), @_);
}

sub open_module {
    return open_inc(mod2fname(shift), @_);
}

sub parse_version {
    my $parsefile = shift;
    my $result;

    local $/ = "\n";
    local $_;

    open(my $fh, "<", $parsefile) or die "Could not open '$parsefile': $!";
    my $inpod = 0;
    while (<$fh>) {
        $inpod = /^=(?!cut)/ ? 1 : /^=cut/ ? 0 : $inpod;
        next if $inpod || /^\s*#/;
        chop;
        next unless /(?<!\\)([\$*])(([\w\:\']*)\bVERSION)\b.*\=/;
        my $eval = qq{
            package ExtUtils::MakeMaker::_version;
            no strict;

            local $1$2;
            \$$2=undef; do {
                $_
            }; \$$2
        };
        local $^W = 0;
        $result = eval($eval);
        #warn "Could not eval '$eval' in $parsefile: $@" if $@;
        last;
    }
    close($fh);

    return $result;
}

sub module_version {
    my $filename = shift;
    return unless $filename =~ /(\w+)\.pm$/;
    my $basename = $1;
    open(my $fh, "<", $filename) || return;

    # scan the module
    local $_;
    my $pkg;
    my $vers;
    my $inpod = 0;
    while (<$fh>) {
        $inpod = /^=(?!cut)/ ? 1 : /^=cut/ ? 0 : $inpod;
        next if $inpod || /^\s*#/;
        if (/^\s*package\s*([\w:]+)/) {
            $pkg = $1;
        }
        elsif (/([\$*])(([\w\:\']*)\bVERSION)\b.*?\=(.*)/) {
            # Borrowed from ExtUtils::MM_Unix
            my $eval = qq{
                 package ExtUtils::MakeMaker::_version;
                 no strict;
                 local $1$2;
                 \$$2=undef; do {
                      $_
                 }; \$$2
            };

            my $val = $4;
            if ($3) {
                $pkg = $3;
                $pkg =~ s/::$//;
            }

            unless ($pkg && $basename eq (split /::/, $pkg)[-1]) {
                undef($pkg);
                next;
            }

            # Avoid losing trailing '0's on numeric assignments like
            # "$VERSION = 1.10"
            if ($val =~ /^\s*(\d+(\.\d*)?)\s*;/) {
                $vers = $1;
                last;
            }

            # let's try to run the $eval
            local $^W;
            $vers = eval($eval);
            $vers =~ s/\s+$//;
            last;
        }
    }
    close($fh);

    return unless $pkg && $vers;

    if ($vers =~ m/^[\x00-\x0F]/ && length($vers) <= 4) {
        # assume v-string
        $vers = sprintf "%vd", $vers;
    }

    return ($pkg, $vers);
}

sub fixup_module_case {
    my($module, %opts) = @_;
    my @mod = split(/::/, $module);
    my @inc;
    if (my $opt_inc = delete $opts{inc}) {
	@inc = @$opt_inc;
    }
    else {
	@inc = @INC;
    }
    my $prefix_only = delete $opts{prefix_only};

    if ($^W && %opts) {
	require Carp;
	Carp::carp("Unrecognized option '$_'") for sort keys %opts;
    }

    return $module unless @inc;
    my @dirs = map [$_ => ""], @inc;

    while (@mod) {
	if (@mod == 1 && !$prefix_only) {
	    my %candidates;
	    for my $d (@dirs) {
		#print "Try $d...\n";
		if (opendir(my $dh, $d->[0])) {
		    while (my $f = readdir($dh)) {
			next unless $f =~ s/\.pm//;
			next unless  lc($mod[0]) eq lc($f);
			$candidates{"$d->[1]::$f"}++;
		    }
		}
	    }
	    if (keys(%candidates) == 1) {
		my($m) = keys %candidates;
		$m =~ s/^:://;
		return $m;
	    }
	    return $module;
	}
	else {
	    my @candidates;
	    for my $d (@dirs) {
		if (opendir(my $dh, $d->[0])) {
		    while (my $f = readdir($dh)) {
			next unless  lc($mod[0]) eq lc($f);
			my $dir = "$d->[0]/$f";
			next unless -d $dir;
			push(@candidates, [$dir, "$d->[1]::$f"]);
		    }
		}
	    }
	    return $module unless @candidates;

	    shift(@mod);
	    @dirs = @candidates;
	}
    }

    my %candidates;
    for my $d (@dirs) {
	$candidates{$d->[1]}++;
    }

    die "Assert" unless keys(%candidates);
    return $module if keys(%candidates) > 1;

    my($m) = keys %candidates;
    $m =~ s/^:://;
    return $m;
}

1;

__END__

=head1 NAME

ActiveState::ModInfo - Queries about installed perl modules

=head1 SYNOPSIS

 use ActiveState::ModInfo qw(list_modules find_module);

=head1 DESCRIPTION

The following functions are provided:

=over

=item $path = find_inc( $fname )

=item $path = find_inc( $fname, \@inc )

Returns the full path to the given $fname, or C<undef> if not found.

=item $path = find_module( $mod )

=item $path = find_module( $mod, \@inc )

Returns the full path to the given module, or C<undef> if not found.

=item $mod = fixup_module_case( $mod, %opts )

Will for instance change "html::parser" into "HTML::Parser".  If
multiple mappings are possible return $mod unchanged.  The $mod is
also returned unchanged if the module can't be located.

The following options are recognized:

=over

=item C<inc> => \@list

A list of directories to search for modules.  If not provided it
defaults to all the @INC.

=item C<prefix_only> => $bool

If true assume $mod to just be a namespace.  Will just look for
matching directory names, not an actual F<.pm> file.

=back

=item $mod = fname2mod( $fname )

Convert an fname to a module module name.  The function might return
C<undef> if the given $fname does not represent a perl module.

=item %modules = list_modules( %opt )

This function will locate all modules (.pm files) and return a list of
module-name/file-location pairs.

The following options are recognized:

=over

=item C<inc> => \@list

A list of directories to search for modules.  If not provided it
defaults to all the @INC entries that point inside the perl
installation directory ($Config{prefix}).

=item C<maxdepth> => $n

When to give up when traversing directories, i.e. how many "::"s to
allow in the module name.  The default is 6.  A value of 0 indicate no
limit.

=item C<allowdup> => $bool

If true return all occurrences or any given module.  If this option is
false or not provided, only the first occurrence of any given module
when traversing C<inc> will be returned.  This should also be the
module that perl would pick up if @INC as the given C<inc>.

=item C<namespace> => $ns

Only modules in the given namespace is returned.  If $ns ends with
"::" only submodules of the given namespace is returned.

=back

=item $fname = mod2fname( $mod )

Convert a module name to an fname.

=item $fh = open_inc( $fname )

=item $fh = open_inc( $fname, \@inc )

Returns an opened file handle for the given fname, or C<undef> if not
found.  Slightly more efficient than using the standard open()
function on the path returned by find_inc(), but otherwise just the
same.  The file is opened in read-only mode.

=item $fh = open_module( $mod )

=item $fh = open_module( $mod, \@inc )

Returns an opened file handle for the given module, or C<undef> if not found.

=item $vers = parse_version( $filename )

Return the $VERSION of a module using the official ExtUtils::MakeMaker
algorithm.  This is a slightly modified copy of the MakeMaker
function.  The main difference is that it returns a real C<undef> if
no version number is found and do it without producing any warning.

=item ($module, $vers) = module_version( $filename )

Return the module name and its version number from a file.

=back

=head1 BUGS

none.
