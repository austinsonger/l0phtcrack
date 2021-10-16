package ActivePerl::DocTools::TOC;

use strict;
use warnings;

use Config qw(%Config);
use File::Basename qw(fileparse);
use File::Find qw(find);

our $dirbase = $Config{installhtmldir} || "$Config{installprefix}/html";

#<<< No perltidy
my @core_pods = qw(
*Overview
  perl  perlintro  perltoc
-
*Tutorials
  perlreftut  perldsc  perllol
-
  perlrequick  perlretut
-
  perlboot  perltoot  perltooc  perlbot
-
  perlperf
-
  perlstyle
-
  perlcheat  perltrap  perldebtut
-
  perlfaq perlfaq1  perlfaq2  perlfaq3  perlfaq4  perlfaq5  perlfaq6
  perlfaq7  perlfaq8  perlfaq9
-
*Reference_Manual
  perlsyn  perldata  perlop  perlsub  perlfunc  perlopentut  perlpacktut
  perlpod  perlpodspec  perlpodstyle  perlrun  perldiag  perllexwarn  perldebug  perlvar
  perlre  perlrebackslash perlrecharclass perlreref  perlref  perlform  perlobj
  perltie  perldbmfilter
-
  perlipc  perlfork  perlnumber
-
  perlthrtut  perlothrtut
-
  perlport  perllocale  perluniintro  perlunicode  perlunifaq  perluniprops  perlunitut  perlebcdic
-
  perlsec
-
  perlmod  perlmodlib  perlmodstyle  perlmodinstall  perlnewmod perlpragma
-
  perlutil
-
  perllocal
-
  perlcompile
-
  perlfilter
-
  perlglossary
-
*Internals_and_C_Language_Interface
  perlembed  perldebguts  perl5db  perlxstut  perlxs  perlclib  perlguts  perlcall
  perlmroapi  perlreapi  perlreguts
-
  perlapi  perlintern  perliol  perlapio
-
  perlhack  perlsource  perlinterp  perlhacktut  perlhacktips  perlpolicy  perlgit  perlrepository
-
*Miscellaneous
  perlbook  perlcommunity  perltodo
-
  perlhist  perldelta
-
  perl5269delta  perl5268delta  perl5267delta perl5266delta  perl5265delta
  perl5264delta  perl5263delta  perl5262delta perl5261delta  perl5260delta
-
  perl5249delta  perl5248delta  perl5247delta perl5246delta  perl5245delta
  perl5244delta  perl5243delta  perl5242delta perl5241delta  perl5240delta
-
  perl5229delta  perl5228delta  perl5227delta perl5226delta  perl5225delta
  perl5224delta  perl5223delta  perl5222delta perl5221delta  perl5220delta
-
  perl5209delta  perl5208delta  perl5207delta perl5206delta  perl5205delta
  perl5204delta  perl5203delta  perl5202delta perl5201delta  perl5200delta
-
  perl5189delta  perl5188delta  perl5187delta perl5186delta  perl5185delta
  perl5184delta  perl5183delta  perl5182delta perl5181delta  perl5180delta
-
  perl5169delta  perl5168delta  perl5167delta perl5166delta  perl5165delta
  perl5164delta  perl5163delta  perl5162delta perl5161delta  perl5160delta
-
  perl5149delta  perl5148delta  perl5147delta perl5146delta  perl5145delta
  perl5144delta  perl5143delta  perl5142delta perl5141delta  perl5140delta
-
  perl5129delta  perl5128delta  perl5127delta perl5126delta  perl5125delta
  perl5124delta  perl5123delta  perl5122delta perl5121delta  perl5120delta
-
  perl5102delta  perl5101delta  perl5100delta
-
  perl589delta  perl588delta  perl587delta  perl586delta  perl585delta
  perl584delta  perl583delta  perl582delta  perl581delta  perl58delta
-
  perl561delta  perl56delta  perl5005delta  perl5004delta
-
  perlartistic  perlgpl
-
*Language-Specific
  perlcn  perljp  perlko  perltw
-
*Platform-Specific
  perlaix  perlamiga  perlapollo  perlbeos  perlbs2000  perlce  perlcygwin
  perldgux  perldos  perlepoc  perlfreebsd  perlhaiku  perlhpux  perlhurd  perlirix  perllinux
  perlmachten  perlmacos  perlmacosx  perlmint  perlmpeix  perlnetware perlopenbsd
  perlos2  perlos390  perlos400  perlplan9  perlqnx  perlriscos  perlsolaris  perlsymbian
  perltru64  perluts  perlvmesa  perlvms  perlvos  perlwin32
);
#>>>

# List of methods to override
for my $method (
    qw(
    header footer
    before_pods      pod      after_pods      pod_separator
    before_scripts   script   after_scripts
    before_pragmas   pragma   after_pragmas
    before_libraries library  after_libraries
    library_indent_open library_indent_close library_indent_same
    library_container)
    ) {
    no strict "refs";
    *$method = sub {
        die "The subroutine $method() must be overriden by the child class";
    };
}

sub new {
    my ( $class, $options ) = @_;
    my $self = $options ? $options : {};
    _BuildHashes($self);
    return bless( $self, $class );
}

sub _cmp_part {
    my ( $a, $b ) = @_;

    # If both module names have the same string prefix followed
    # by a number, then sort the names by the number numerically.
    my ( $prefix, $number ) = $a =~ /^([a-z]+)(\d+)$/i;
    if ( $prefix && $b =~ /^$prefix(\d+)$/ ) {
        return $number <=> $1;
    }

    # Otherwise compare names alphabetically
    return $a cmp $b;
}

sub _cmp_module_names {
    my @a = split /::/, $a;
    my @b = split /::/, $b;

    # Compare each part of the name separately, so we use the
    # "string prefix with number suffix" sorting rule at each level.
    while ( scalar(@a) && scalar(@b) ) {
        my $cmp = _cmp_part( shift @a, shift @b );
        return $cmp if $cmp;
    }
    return -1 unless @a;
    return 1  unless @b;
    return 0;
}

# generic structure for the website, HTML help, RDF
sub TOC {
    my ($self) = @_;

    # generic header stuff
    my $output = $self->boilerplate;
    $output .= $self->header;

    # core pods
    $output .= $self->before_pods;

    my %unused_pods = %{ $self->{pods} };
    foreach my $file (@core_pods) {
        next if $file =~ /^\*/;
        if ( $file eq '-' ) {
            $output .= $self->pod_separator;
        }
        elsif ( delete $unused_pods{"Pod::$file"} || delete $unused_pods{"pods::$file"} ) {
            $output .= $self->pod($file);
        }
        else {
            warn "Couldn't find pod for $file" if $self->{verbose};
        }
    }

    if ( $self->{verbose} ) {
        warn "Unused Pod: $_" for sort keys %unused_pods;
    }

    $output .= $self->after_pods;

    # scripts
    $output .= $self->before_scripts;
    $output .= $self->script($_) for sort { uc($a) cmp uc($b) } keys %{ $self->{scripts} };
    $output .= $self->after_scripts;

    # pragmas
    $output .= $self->before_pragmas;
    $output .= $self->pragma($_) for sort keys %{ $self->{pragmas} };
    $output .= $self->after_pragmas;

    # libraries
    $output .= $self->before_libraries;

    my $depth = 0;
    foreach my $file ( sort _cmp_module_names keys %{ $self->{files} } ) {
        my $showfile   = $file;
        my $file_depth = 0;
        my $depth_changed;

        # cuts $showfile down to its last part, i.e. Foo::Baz::Bar --> Bar
        # and counts the number of times, to get indent. --> 2
        $file_depth++ while $showfile =~ s/.*?::(.*)/$1/;

        while ( $file_depth > $depth ) {
            $output .= $self->library_indent_open;
            $depth++;
            $depth_changed = 1;
        }
        while ( $file_depth < $depth ) {
            $output .= $self->library_indent_close;
            $depth--;
            $depth_changed = 1;
        }

        $output .= $self->library_indent_same unless $depth_changed;

        if ( $self->{files}->{$file} ) {
            $output .= $self->library( $file, $showfile, $depth );
        }
        else {
            # assume this is a containing item like a folder or something
            $output .= $self->library_container( $file, $showfile, $depth );
        }
    }

    $output .= $self->after_libraries;
    $output .= $self->footer;

    return $output;
}

my %rename = (
    lwptut  => "LWP::lwptut",
    lwpcook => "LWP::lwpcook",
    Roadmap => "DBI::Roadmap",

    # already in pods/
    perlfilter  => undef,
    perlpod     => undef,
    perlpodspec => undef,

    # move to core Perl docs section
    perl5db =>
        ( $^O eq "darwin" || $^O eq "MSWin32" && Win32::BuildNumber() >= 821 ? "pods::" : "Pod::" )
        . "perl5db",
    perllocal =>
        ( $^O eq "darwin" || $^O eq "MSWin32" && Win32::BuildNumber() >= 821 ? "pods::" : "Pod::" )
        . "perllocal",

    # because podlators from CPAN will install into Pod::perlpodstyle
    "Pod::perlpodstyle" =>
        ( $^O eq "darwin" || $^O eq "MSWin32" && Win32::BuildNumber() >= 821 ? "pods::" : "Pod::" )
        . "perlpodstyle",

    # useless docs
    "Exporter::Heavy"   => undef,
    "JSON::PP::Boolean" => undef,    # also /^JSON::PP5/
    "JSON::XS::Boolean" => undef,    # also /^JSON::PP5/
    "Pod::PerlEz"       => undef,    # linked from ActivePerl Components
    TASKS               => undef,    # from DBI
    "YAML::XS::LibYAML" => undef,
);

sub _BuildHashes {
    my $self = shift;

    die "htmldir not found at: $dirbase" unless -d $dirbase;

    my @checkdirs = qw(bin lib site/lib);
    my ( %files, %pragmas, %pods, %scripts );

    my $Process = sub {
        return if -d;
        my $parsefile = $_;

        my ( $filename, $dir, $suffix ) = fileparse( $parsefile, '\.html' );

        if ( $suffix !~ m#\.html# ) { return; }

        my $TOCdir = $dir;

        $filename =~ s/(.*)\..*/$1/;

        #    print "$TOCdir";
        my $ver = $Config{version};
        my $an  = $Config{archname};
        if ( $TOCdir =~ s#^.*?(bin/)(\Q$an\E/)?(.*)$#$3# ) {
            return if $filename eq "ppm3-bin";
            return if $] >= 5.008 && $filename eq "ppm3";
            return if $] < 5.008 && $filename eq "ppm";

            $scripts{"$TOCdir$filename"} = "bin/$filename.html";
            return 1;
        }
        $TOCdir =~ s#^.*?(lib/site_perl/\Q$ver\E/|lib/\Q$ver\E/|lib/)(\Q$an\E/)?(.*)$#$3#;
        $TOCdir =~ s#/#::#g;
        $TOCdir =~ s#^pod::#Pod::#;    #Pod dir is uppercase in Win32

        #    print " changed to: $TOCdir\n";
        $dir =~ s#.*?/((site/)?lib.*?)/$#$1#;    #looks ugly to get around warning

        if ( $files{"$TOCdir/$filename.html"} ) {
            warn "$parsefile: REPEATED!\n";
        }
        $files{"$TOCdir$filename"} = "$dir/$filename.html";

        #    print "adding $parsefile as " . $files{"$TOCdir/$filename.html"} . "\n";
        #    print "\%files{$TOCdir$filename.html}: " . $files{"$TOCdir$filename.html"} . "\n";

        return 1;
    };

    foreach my $dir (@checkdirs) {
        next unless -d "$dirbase/$dir";
        find( { wanted => $Process, no_chdir => 1 }, "$dirbase/$dir" );
    }

    foreach my $file ( keys %files ) {
        if ( exists $rename{$file} ) {
            my $entry = delete $files{$file};
            $file = $rename{$file};
            next unless defined $file;
            $files{$file} = $entry;
        }

        if ( $file =~ /^(Pod|pods)::(a2p|perldoc)$/ ) {
            $scripts{$2} = delete $files{$file};
        }
        elsif ( $file =~ /^(Pod|pods)::perl/ ) {
            $pods{$file} = delete $files{$file};
        }
        elsif ($file eq "dmake"
            || $file eq "MinGW"
            || 0 ) {
            $scripts{$file} = delete $files{$file};
        }
        elsif ($file =~ /^(Pod|pods)::active/
            || $file =~ /^ASRemote/
            || $file =~ /^PPM/
            || $file =~ /^JSON::PP5/
            || 0 ) {
            delete $files{$file};
        }
        elsif ( $file =~ /^[a-z]/ ) {
            $pragmas{$file} = delete $files{$file};
        }
    }

    foreach my $file ( sort { uc($b) cmp uc($a) } keys %files ) {
        my $prefix = $file;
        while ( $prefix =~ s/(.*)?::(.*)/$1/ ) {
            unless ( defined( $files{$prefix} ) ) {
                $files{$prefix} = '';
                warn "Added topic: $prefix\n" if $self->{verbose};
            }
            warn "$prefix from $file\n" if $self->{verbose};
        }
    }

    $self->{files}   = \%files;
    $self->{pods}    = \%pods;
    $self->{pragmas} = \%pragmas;
    $self->{scripts} = \%scripts;
}

1;
