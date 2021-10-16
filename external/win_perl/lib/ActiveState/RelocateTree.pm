package ActiveState::RelocateTree;
require Exporter;

use strict;
use Config;
use Cwd qw(abs_path getcwd);
use File::Basename qw(dirname basename);
use File::Copy ();
use File::Find;
use File::Path;
use File::Spec;

use vars qw(@ISA @EXPORT_OK $VERSION);
@ISA = qw(Exporter);
@EXPORT_OK = qw(relocate edit check move_tree spongedir rel2abs abs2rel);
$VERSION = '1.04';

my $modifier = $^O eq 'MSWin32' ? '(?i)' : '';

# This variable has to be built up, or this package itself will be relocated.
# That should never happen, since the unmodified path is needed by PPM.
# Scripts like reloc_perl provide their own default, which will of course get
# relocated as wanted.
sub spongedir {
    my %sponges = (
	ppm => '/tmp'.'/.ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZpErLZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZperl',
	thisperl => 'C:\cygwin\home\gecko\tmp\perl-os'.'bjwfdljxvgxuqemfdcixccagxrahfmzx',
    );
    return $sponges{lc$_[0]};
}

sub relocate {
    my %opt = (
#	to       => ??? -> you have to provide this one
	from     => $Config{prefix},

	quiet    => 0,
	verbose  => 0,
	filelist => undef,

	ranlib   => 1,
	textonly => 0,

	savebaks => 0,
	bak      => '.~1~',

	inplace  => 0,
	killorig => 0,
	usenlink => 1,
	@_,
    );
    $opt{search}  = $opt{from} unless exists $opt{search};
    $opt{replace} = $opt{to}   unless exists $opt{replace};
    $opt{inplace} = 1          if $opt{to} eq $opt{from};
    $opt{verbose} = 0          if $opt{quiet};

    unless ($opt{to}) {
	warn "No `to' path given to relocate(): cannot continue";
	return;
    }

    # Substitute '/' or '\\' characters with [/\\] if this is Windows. This
    # allows matching either slashes or backslashes.
    my $regexp;
    if ($^O eq 'MSWin32') {
	my @parts = map { quotemeta } split m#[/\\]#, $opt{search};
	$regexp = join '[/\\\\]', @parts;
    }
    else {
	$regexp = quotemeta($opt{search});
    }

    move_tree(@opt{qw(from to killorig verbose usenlink)})
	unless $opt{inplace};

    my (@bin, @text);
    {
	# On HP-UX with pfs_mount, nlink is always 2.
	local $File::Find::dont_use_nlink = !$opt{usenlink};
	find(sub {
	    return if -l;
	    if (-d) {
	        chmod 0755, $_;
		return;
	    }
	    return unless -f && -s;
	    if (-B) {
		return if $opt{textonly};
		push @bin, $File::Find::name if check($_, $regexp, 1);
	    }
	    else {
		push @text, $File::Find::name if check($_, $regexp, 0)
	    }
	}, resolve($opt{to}));
    }

    # show affected files
    print "Configuring Perl installation at $opt{to}\n"
	if !$opt{quiet} && (@bin || @text);

    if ($opt{filelist}) {
	open LOG, "> $opt{filelist}" or die "can't open $opt{filelist}: $!";
	print LOG "B $_\n" for @bin;
	print LOG "T $_\n" for @text;
	close LOG or die "can't close $opt{filelist}: $!";
    }
    if ($opt{verbose}) {
	print "Translating $opt{search} to $opt{replace}\n";
	print "editing binary file $_\n" for @bin;
	print "editing text file $_\n" for @text;
    }

    # edit files
    edit($regexp, @opt{qw(search replace bak)}, 0, @text);
    edit($regexp, @opt{qw(search replace bak)}, 1, @bin);

    # clobber backups
    unless ($opt{savebaks}) {
	print "cleaning out backups\n" if $opt{verbose};
	unlink "$_$opt{bak}" for (@text, @bin);
    }

    # run ranlib, where appropriate
    my $rl = $Config{ranlib};
    $rl = '' if $rl =~ /^:?\s*$/;
    if ($rl and $opt{ranlib}) {
	for (@bin) {
	    if (/\Q$Config{_a}\E$/o) {
		print "$rl $_\n" if $opt{verbose};
		system("$rl $_") == 0 or die "`$rl $_' failed: $?";
	    }
	}
    }
}

sub binary_pad {
    my($suffix,$pad_length) = @_;
    my $pad_char = "\0";
    $pad_char = ":" if $^O eq "aix" && index($suffix, ":") > 0;
    return $suffix . ($pad_char x $pad_length);
}

sub relocate2 {
    my $from = shift;
    my $to = shift;
    my $sponge = shift || $Config{prefix};
    my $reloc_txt = shift || "support/reloc.txt";
    my $inplace = $to eq $from;

    unless ($inplace) {
    # Find a suitable way to copy
    print "Copying files to $to...";
    # XXX there is a bug somewhere in perl that unsets $@ if
    # move_tree() below croaks.  Preloading Carp::Heavy avoids
    # this problem.  This problem can be reproduced by trying to
    # install to a non-writable location.  Without this hack the
    # installer will abort, but still report the install as
    # successful.
    require Carp::Heavy;

    move_tree($from, $to, 0, 0, 0);
    print "done\n";
    }

    # Relocate
    if (open(my $reloc, $reloc_txt)) {
	die "Can't relocate to a path longer than " . length($sponge) . " chars"
	    if length($to) > length($sponge);
	my $pad_length = length($sponge) - length($to);
	my $binary_pad = "\0" x $pad_length;

	print "Relocating...";
	my $count = 0;
	local $_;
	while (<$reloc>) {
	    chomp;
	    my($type, $f) = split(' ', $_, 2);
	    $f = "$to/$f";
	    #print "Relocating $f...\n";

	    # ensure that the file is writeable
	    my $old_mode = (stat $f)[2] & 07777;
	    my $mode = $old_mode | 0200;  # make writable
	    chmod($mode, $f) if $mode != $old_mode;

	    # update the file
	    open(my $fh, "+<", $f) || die "Can't open $f: $!";
	    binmode($fh);
	    my $content = do { local $/; <$fh> };

	    if ($type eq "B") {
		if ($^O eq "aix") {
		    1 while $content =~ s|\Q$sponge\E([^\0]*)|$to . binary_pad($1, $pad_length)|ge;
		}
		else {
		    1 while $content =~ s,\Q$sponge\E([^\0]*),$to$1$binary_pad,go;
		}
	    }
	    else {
		$content =~ s,\Q$sponge\E,$to,go;
		truncate($fh, length($content)) || die "Can't truncate '$f': $!";
	    }

	    seek($fh, 0, 0) || die "Can't reset file pos on '$f': $!";
	    print $fh $content;
	    close($fh) || die "Can't write back content to '$f': $!";

	    # make file unwritable again if it was before the update
	    chmod($old_mode, $f) if $mode != $old_mode;

	    $count++;
	}
	print "done ($count files relocated)\n";
    }
    else {
	die "Can't open '$reloc_txt': $!\n";
    }
}


sub check {
    my $file = shift;
    my $re   = shift;
    my $bin  = shift;

    # On Windows we also need to look for escaped backslashes, like
    #    foodir='c:\\perl\\foo';
    my $re2 = $re;
    if ($^O eq "MSWin32") {
        my $from = '[/\\\\]';
        $re2 =~ s[\Q$from\E][\\\\\\\\]g;
    }

    local (*F, $_);
    open F, "< $file" or die "Can't open `$file': $!";
    binmode F if $bin;
    my $mod = $modifier;
    while (<F>) {
	return 1 if /$mod$re/;
	return 1 if $^O eq "MSWin32" && /$mod$re2/;
    }
    return 0;
}

sub edit {
    my $re   = shift;
    my $from = shift;
    my $dest = shift;
    my $bak  = shift;
    my $bin  = shift;
    return unless @_; # prevent reading from STDIN

    my $mod  = $modifier;
    my ($term, $pad);

    # On Windows we also need to look for escaped backslashes, like
    #    foodir='c:\\perl\\foo';
    my $re2 = $re;
    if ($^O eq "MSWin32") {
        my $from = '[/\\\\]';
        $re2 =~ s[\Q$from\E][\\\\\\\\]g;
    }

    if ($bin) {
	my $d = length $dest;
	my $f = length $from;

	# If we're installing into a shorter path, we have to NUL pad the
	# replacement string. But if we're installing into a longer path, we
	# have to NUL-pad the search string.
	if ($d <= $f) {
	    $term = '([^\0]*\0)';
	    $pad  = "\0" x ($f - $d);
	}
	else {
	    $term = "([^\0]*\\0)\\0{" . ($d - $f) . "}";
	    $pad  = '';
	}
    }
    else {
	$term = '()';
	$pad  = '';
    }

    local ($_, *INPUT, *OUTPUT);
    my $old = select(STDOUT);
    for my $file (@_) {
	rename($file, "$file$bak")
	    or do { warn "can't rename $file: $!" if $^W; next };
	open(INPUT,   "< $file$bak")
	    or do { warn "can't open $file$bak: $!" if $^W; next };
	open(OUTPUT,  "> $file")
	    or do { warn "can't write $file: $!" if $^W; next };
	chmod((stat "$file$bak")[2] & 07777, $file);
	binmode(INPUT), binmode(OUTPUT) if $bin;
	select(OUTPUT);
	if ($^O eq 'MSWin32') {
	    while (<INPUT>) {
	        if (m[($mod$re$term)]) {
		    # if the string to be modified has backslashes anywhere
		    # in it and has no forward slashes, make the replacement
		    # string backslashed too
		    my $match = $1;
		    my $d = $dest;
		    if ($match =~ m[\\] and $match !~ m[/]) {
		        $d =~ s[/][\\]g;
		    }
		    s[$mod$re$term][$d$1$pad]g;
		}
	        if (m[($mod$re2$term)]) {
		    (my $d = $dest) =~ s[/][\\\\]g;
		    s[$mod$re2$term][$d$1$pad]g;
		}
		print;
	    }
	}
	else {
	    while (<INPUT>) {
		s[$mod$re$term][$dest$1$pad]g;
		print;
	    }
	}
	close(OUTPUT);
	close(INPUT);
    }
    select($old);

# Unfortunately, this doesn't work in 5.005_03. Oh, how I wish it would just
# die once and for all!
#    local ($_, *ARGV, *ARGVOUT);
#    local $^I = $bak;
#    @ARGV = @_;
#    binmode(ARGV), binmode(ARGVOUT) if $bin;
#    while (<>) {
#	s[$mod\Q$from\E$term][$dest$1$pad]g;
#	print;
#	close ARGV if eof;
#    }
}

sub move_tree {
    my ($from, $to, $kill, $verbose, $usenlink) = @_;
    $from = rel2abs(resolve($from));
    $to   = rel2abs($to);

    # On HP-UX with pfs_mount, nlink is always 2.
    local $File::Find::dont_use_nlink = !$usenlink;
    my @dir_utime_stack;
    find({
	bydepth => 1,
	wanted => sub {
	    my $src = abs2rel($File::Find::name, $from);
	    my $targ;

	    if (-l) {
		# Resolve the source link. If it points inside the source tree,
		# build a similar one which points to the same relative location
		# in the destination tree. Else, copy it if it points to a file,
		# else die.
		my $resolved = resolve($File::Find::name);
		if ($resolved =~ /^$modifier\Q$from\E/) {

		    # The symlink we create:
		    my $link = File::Spec->catfile($to, $src);
		    my $dir = dirname($link); # where
		    unless (-d $dir) {
			mkpath($dir, 0, 0777)
			    || die "Can't mkpath $dir: $!";
		    }

		    # What should be the content of the link?
		    my $rel = abs2rel($resolved, $from);
		    my $dest = abs2rel(File::Spec->catfile($to, $rel), $dir);
		    symlink($dest, $link)
			|| die "Can't create symlink at '$link': $!";
		    return;
		}
		elsif (-d) {
		    die "move_tree: symlink '$File::Find::name' points to a " .
			"directory '$resolved' outside of '$from'.\n";
		}
	    }

	    if (-f) {
		$targ = File::Spec->catfile($to, $src);
		my $dir = dirname($targ);
		unless (-d $dir) {
		    mkpath($dir, 0, 0777)
			|| die "Can't mkpath $dir: $!";
		}
		File::Copy::syscopy($File::Find::name, $targ)
		    || die "Can't copy to '$targ': $!";
	    }
	    elsif (-d) {
		$targ = File::Spec->catdir($to, $src);
		unless (-d $targ) {
		    mkpath($targ, 0, 0777)
			|| die "Can't mkpath $targ: $!";
		}
	    }
	    else {
		die "Can't copy '$File::Find::name': unsupported file type.\n";
	    }

	    # Preserve file/directory attributes. We expect chown() to fail if
	    # the group is bad, so don't warn about it. This is the same
	    # policy as GNU tar, which silently sets the user/group of
	    # extracted files to the current user's primary uid/gid if the
	    # initial chmod fails.
	    my @stat = stat($File::Find::name)
		or die "Can't stat $File::Find::name: $!";
	    my $uid = $> ? -1 : $stat[4];
	    my $gid = $stat[5];
	    chown($uid, $gid, $targ); # ignore chmod failures.
	    chmod($stat[2] & 07777, $targ)
		|| die "Can't chmod $File::Find::name: $!";
	    # Windows does not allow the changing of file times if the file is readonly.
	    utime($stat[8], $stat[9], $targ)
		|| $^O eq "MSWin32" || die "Can't set atime/mtime of $File::Find::name: $!";
	},
	no_chdir => 1,
    }, $from);
    if ($kill) {
	print "deleting $from\n" if $verbose;
	rmtree($from, 0, 0);
    }
}

{
    my $rel2abs_test = UNIVERSAL::can("File::Spec", "rel2abs");
    my $abs2rel_test = (
	UNIVERSAL::can("File::Spec", "abs2rel") && $^O ne 'MSWin32'
    );
    my $symlink_test = eval { symlink('', ''); 1 };

    sub resolve {
	my $l = shift;
	return $l unless $symlink_test;
	return $l unless -l $l;
	my $d = dirname($l);
	my $v = readlink($l);
	return rel2abs($v, $d);
    }

    sub rel2abs {
	my ($rel, $relto) = @_;
	my ($base, $rest);
	if ($rel2abs_test) {
	    $base = File::Spec->rel2abs(@_);
	    $rest = '';
	}
	else {
	    # Support for 5.005:
	    return $rel if File::Spec->file_name_is_absolute($rel);
	    if    (!defined $relto) { $relto = getcwd(); }
	    elsif (!File::Spec->file_name_is_absolute($relto)) {
		$relto = rel2abs($relto);
	    }
	    ($base, $rest) = (File::Spec->catdir($relto, $rel), '');
	}
	until (-d $base) {
	    $rest = File::Spec->catdir(basename($base), $rest);
	    $base = dirname($base);
	}
	return File::Spec->catdir(abs_path($base), $rest) if $base and $rest;
	return abs_path($base) if $base and not $rest;
	die "can't absolutize $rel against $relto\n";
    }

    sub abs2rel {
	return File::Spec->abs2rel(@_) if $abs2rel_test;

	# Support for 5.005:
	my $abs  = shift;
	my $from = shift;
	(my $rel  = $abs) =~ s#$modifier^\Q$from\E[\\/]?##;
	return $rel;
    }
}

1;

__END__

=head1 NAME

ActiveState::RelocateTree - copy tree substituting paths at the same time

=head1 SYNOPSIS

   use ActiveState::RelocateTree qw(relocate);
   relocate(from => 'C:\Perl', to => 'D:\lang\perl');

=head1 DESCRIPTION

When a perl installation is copied into a new location, some of its
files need to be modified accordingly.  The
C<ActiveState::RelocateTree> module provide functions that helps you
do this.

The following functions are provided.  None of them are exported by
default.

=over

=item relocate( %options )

This is the main entry point that applications will use.  The
following options are recognized:

=over 4

=item C<to>

The tree which must be transformed. Unless the C<inplace> option is
true, it will copy the tree at C<from> to C<to> before transforming
it. This option is the only one required.  The other options have
reasonable defaults, so in most cases this is the only option you need
to provide.

=item C<from>

The path from which to copy the perl tree. Defaults to C<$Config{prefix}>, the
home of the currently executing perl interpreter.

=item C<search>

This is the path which will be searched for and replaced in C<to>. This
defaults to the value of C<from>.

=item C<replace>

The replacement value for C<search>. This defaults to the value of C<to>.

=item C<inplace>

If the tree at C<to> already exists and you just want to transform it in-situ,
use this option. It skips the copying step and just transforms the tree.
If C<from> equals C<to>, it is set to true and cannot be unset. Otherwise it
defaults to false.

=item C<killorig>

If you're really moving the tree, this option will remove C<from> after
copying and transforming C<to>. Use with care! Defaults to false.

=item C<bak>

While relocating the tree, relocate() creates a backup file for each file
being edited. This option allows you to specify the extension of backup files.
Defaults to C<.~1~>.

=item C<savebaks>

Normally relocate() deletes the backup files before returning. C<savebaks>
skips that step, leaving the backup files alone. Defaults to false (backups
are deleted).

=item C<textonly>

Normally relocate() edits both text and binary files. Text files are replaced
using a normal search-and-replace algorithm, but binary files are NULL-padded
so that all offsets remain the same. By default, C<textonly> is false, i.e.
relocate() operates on both text and binary files.

=item C<ranlib>

If C<ranlib> is true, relocate() will call C<ranlib> on binary files which
look like library files (have the C<$Config{_a}> extension). Defaults to true.

=item C<verbose>

If C<verbose> is true, relocate() emits warning messages as it performs
certain operations. This may be useful for debugging, or for command-line
tools, where user feedback is a good thing.

=item C<quiet>

Normally, relocate() prints out some status messages even with C<verbose>
disabled. If C<quiet> is true, all messages (except error messages) are
temporarily silenced. This option overrides C<verbose>, so there isn't much
point calling relocate() with both C<quiet> and C<verbose> set. By default,
C<quiet> is false.

=item C<filelist>

If specified, relocate() will write a list of the files modified to
C<filelist>, one filename per line.  The lines are prefixed with "B "
for binary files and "T " for text files.

=back

=item move_tree( $from, $to )

=item move_tree( $from, $to, $delete_after, $verbose )

This function will copy the directory tree at $from to the location
$to.

If $delete_after is TRUE, then tree at $from will be removed after the
copy completes. If $verbose is TRUE, then print a message when
deleting the $from tree.

=item check( $file, $regexp, $is_binary )

Returns TRUE if there are occurrences of $regexp in $file. It is used
by relocate() to search for files which should be edited.  If
$is_binary is TRUE, then read the file in binmode.

=item edit( $regexp, $from, $dest, $bak, $are_binary, @files )

edit() is designed to rip though a set of files, efficiently replacing $from
with $dest. It operates on the whole set of files, which all need to be of the
same type (binary or text). It accepts the following parameters:

=over 4

=item $regexp

The regular expression to search for. Matching text will be replaced with
$dest.

=item $from

The path to search for and replace. If $are_binary is true, this is used to
calculate the amount of NUL-padding required to preserve the length of strings.
It is not used otherwise.

=item $dest

The replacement string. If $are_binary is true and $dest is shorter than
$from, then it inserts a NULL-pad to preserve the original length of the
strings.

=item $bak

The extension to use when storing backup files.

=item $are_binary

A boolean: if true, the files are edited with binary semantics: the
filehandles are set to binmode, and strings are NULL-padded. Otherwise
a plain-old substitution occurs.

=item @files

A list of files to edit.

=back

=item spongedir( $name )

Returns the spongedir associated with a particular product.  The $name
is the spongedir you're interested in. It's case-insensitive.  The
following spongedirs are known:

=over 10

=item ppm

The sponge directory to be used in PPM packages.

=item thisperl

The original directory in which this copy of Perl was built. This allows
relocate() to detect when a replacement path will not fit into the binary.

=back

=back

=head1 SEE ALSO

L<reloc_perl>

=head1 COPYRIGHT

Copyright 2002 ActiveState Software Inc.  All Rights Reserved.

=cut
