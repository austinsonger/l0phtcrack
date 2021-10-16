package ActivePerl::DocTools::Tree::HTML;

use strict;
use warnings;

use Config qw(%Config);
use Cwd qw(cwd);
use File::Basename qw(dirname);
use File::Path qw(mkpath);
use Pod::Find qw(pod_find);

use ActivePerl::DocTools::Pod qw(pod2html pod2html_remove_cache_files);

sub _relative_path {
    my($path, $prefix) = @_;
    $path =~ s,\\,/,g if $^O eq "MSWin32";
    $path =~ s,/\z,, unless $path =~ m,^([A-Za-z]:)?/\z,;

    if (defined $prefix && length $prefix) {
	$prefix =~ s,\\,/,g if $^O eq "MSWin32";
        $prefix =~ s,/\z,, unless $prefix =~ m,^([A-Za-z]:)?/\z,;

	my @path_parts   = split('/', $path);
	my @prefix_parts = split('/', $prefix);
	return $path if @path_parts < @prefix_parts;

	while (@prefix_parts) {
	    my $path_part   = shift(@path_parts);
	    my $prefix_part = shift(@prefix_parts);
	    if ($^O eq "MSWin32") {
		$_ = lc for $path_part, $prefix_part;
	    }
	    return $path unless $path_part eq $prefix_part;
	}

	$path = join('/', @path_parts) || ".";
    }
    return $path;
}

sub Update {
    my %args = @_;

    my $prefix  = $args{prefix}  || $Config{installprefix};
    my $htmldir = $args{htmldir} || $Config{installhtmldir} || "$prefix/html";
    my $podpath = $args{podpath} || [@Config{qw(privlib sitelib scriptdir)}];

    my $starting_cwd = cwd();
    unless (chdir($prefix)) {
	warn "Can't chdir to root of Perl installation: $!\n";
	return;
    }

    print "Building HTML tree at $htmldir, cwd is $prefix\n" if $args{verbose};

    my %pods = pod_find(@$podpath);
    @$podpath = map { _relative_path($_, $prefix) } @$podpath;

    foreach my $key (sort keys %pods) {
	my $in_file = _relative_path($key, $prefix);
	my $out_file = "$htmldir/$in_file";
	$out_file =~ s/\.[a-z]+\z|\z/.html/i;

	if ($args{force} || !-e $out_file || (stat $in_file)[9] > (stat $out_file)[9]) {
	    print "Making $out_file from $in_file => $pods{$key}\n"
		if $args{verbose};

	    unlink($out_file);
	    my $out_dir = dirname($out_file);
	    mkpath($out_dir);

	    my $depth = $in_file =~ tr,/,,;
	    pod2html(infile => $in_file, outfile => $out_file,
		     depth => $depth,
		     podroot => ".", podpath => $podpath,
		     index => 1,);
	}
	else {
	    print "Skipping $out_file\n" if $args{verbose};
	}
    }

    pod2html_remove_cache_files();
    chdir($starting_cwd) or die "Can't chdir back to '$starting_cwd': $!";
}

sub Update_blib {
    my %args  = @_;

    my $prefix      = $args{prefix}      || $Config{installprefix};
    my $htmldir     = $args{htmldir}     || $Config{installhtmldir} || "$prefix/html";

    my $installdirs = $args{installdirs} || 'site';
    my $instprefix  = { perl   => 'lib/',
                        site   => 'site/lib/',
                        vendor => 'site/lib/',
                      }->{$installdirs};

    my $starting_cwd = cwd();
    my $blib = File::Spec->catfile($starting_cwd, $args{blib} || 'blib');

    my $podpath = $args{podpath} || [$blib, @Config{qw(privlib sitelib scriptdir)}];

    print "Building HTML in $blib\n" if $args{verbose};

    unless (chdir($prefix)) {
	warn "Can't chdir to root of Perl installation: $!\n";
	return;
    }

    my %pods = pod_find($blib);
    @$podpath = map { _relative_path($_, $prefix) } @$podpath;

    foreach my $key (sort keys %pods) {
        my $in_file = $key;
        my $out_file = _relative_path($key, $blib);
        $out_file =~ s/\.[a-z]+\z|\z/.html/i;

        #Correct differences between blib/ layout and final layout
        $out_file =~ s[^script/][bin/];
        $out_file =~ s[^lib/][$instprefix];
        my $depth = $out_file =~ tr,/,,;

        $out_file = File::Spec->catfile($blib, 'html', $out_file);
        my $out_dir = dirname($out_file);
        mkpath($out_dir);

        print  "Making $out_file from $in_file => $pods{$key}\n"
            if $args{verbose};

	pod2html(infile => $in_file, outfile => $out_file,
		 depth => $depth,
		 podroot => ".", podpath => $podpath,
		 index => 1,);

        #We now fix links that point to our blib/html, since that's only
        #a temporary location
        open (HTMLFILE, "<$out_file") or die "Couldn't open $out_file: $!";
        open (TMPFILE, ">$out_file.tmp") or die "Couldn't open $out_file.tmp: $!";

        my $bhtml = File::Spec->catfile($blib, 'html', '');
        while (my $line = <HTMLFILE>) {
            $line =~ s/\Q$bhtml//g;
            print TMPFILE $line;
        }

        close (TMPFILE) || die;
        close (HTMLFILE);
        rename("$out_file.tmp", $out_file) || die;
    }

    pod2html_remove_cache_files();
    chdir($starting_cwd) or die "Can't chdir back to '$starting_cwd': $!";
}

1;
