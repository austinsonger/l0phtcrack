package ActiveState::Run;

use strict;

our $VERSION = '1.02';

use base 'Exporter';
our @EXPORT_OK = qw(run run_ex shell_quote decode_status);

require Carp;

sub _echo_cmd {
    my $prefix = $ENV{AS_RUN_PREFIX};
    $prefix = "" unless defined $prefix;
    if (@_ == 1) {
        print "$prefix$_[0]\n";
    }
    else {
        print $prefix . shell_quote(@_) . "\n";
    }
}

sub run {
    my @cmds = @_;

    my $ignore_err = $cmds[0] =~ s/^-//;
    my $silent = $ENV{AS_RUN_SILENT};
    if ($cmds[0] =~ s/^@(-)?//) {
	$silent++;
	$ignore_err++ if $1;
    }
    _echo_cmd(@cmds) unless $silent;

    system(@cmds) == 0 || $ignore_err || do {
	my $msg = "Command";
	if ($? == -1) {
	    my $cmd = $cmds[0];
	    $cmd =~ s/\s.*// if @cmds == 1;
	    $msg .= qq( "$cmd" failed: $!);
	}
	else {
            $msg .= " " . decode_status();
	}
	$msg .= ":\n  @cmds\n  stopped";

        Carp::croak($msg);
    };
    return $?;
}

sub run_ex {
    my(%opt) = @_;

    my $exe = delete $opt{exe};
    my $cmd = delete $opt{cmd} || $exe ||
        Carp::croak("Mandatory argument 'cmd' not passed to run_ex");
    $cmd = [$cmd] unless ref($cmd) eq "ARRAY";

    my $cwd = delete $opt{cwd};
    my $env = delete $opt{env};
    my $env_hide = delete $opt{env_hide};

    my $stdin = delete $opt{stdin};
    my $output = delete $opt{output};
    my $tee = delete $opt{tee};
    my $nice = delete $opt{nice};
    my $new_group = delete $opt{new_group};

    my $silent = delete $opt{silent};
    my $ignore_err = delete $opt{ignore_err};

    my $limit_time = delete $opt{limit_time} || delete $opt{timeout} || 0;
    my $limit_output = delete $opt{limit_output} || 0;
    $limit_output = $tee = 0 unless $output;

    my $MB = 1024 * 1024;
    $limit_output *= $MB;

    my %ulimit;
    for my $limit (qw(cpu fsize vmem data stack core)) {
        my $v = delete $opt{"limit_$limit"};
        next unless defined $v;
        $v *= $MB unless $limit eq "cpu";
        $ulimit{$limit} = $v;
    }

    if (%opt && $^W) {
        Carp::carp("Unrecognized option '$_' passed to run_ex") for sort keys %opt;
    }

    _echo_cmd(@$cmd) unless $silent;

    my $tail_f_pos = 0;
    my $tail_f = sub {
	if (open(my $fh, "<", $output)) {
	    if (-s $fh < $tail_f_pos) {
		$tail_f_pos = 0;  # restart
	    }
	    if (-s _ > $tail_f_pos) {
		seek($fh, $tail_f_pos, 0) if $tail_f_pos;
		local $_;
		while (<$fh>) {
		    print;
		}
		$tail_f_pos = tell($fh);
	    }
	    close($fh);
	}
    };

    my $kill_reason;
    my $should_kill = sub {
	my $before = shift;
	if ($limit_output && -s $output > $limit_output) {
	    my $limit = ($limit_output >= 1024 * 1024)
		? sprintf "%.1f MB", $limit_output / (1024 * 1024)
		: "$limit_output bytes";
	    $kill_reason = "Too much output (limit is $limit)";
	    return 1;
	}
        if ($limit_time && (Time::HiRes::time() - $before) > $limit_time) {
	    $kill_reason = "Timeout (max run time is ${limit_time}s)";
	    return 1;
	}
	return 0;
    };

    if ($^O eq "MSWin32") {
	my $output_fh;
	if ($output) {
	    open($output_fh, ">", $output) || die "Can't open '$output': $!";
	}
	unless ($stdin) {
	    open($stdin, "<", "nul") || die "Can't open 'nul': $!";
	}

        if ($env || $env_hide) {
            Carp::carp("Environment manipulation for run_ex() not implemented on Windows yet");
        }

        # Use Win32::Job to control the process and all its descendents.
        require Win32::Job;
        my $job = Win32::Job->new;
        my $pid = $job->spawn(
            $exe, scalar(shell_quote(@$cmd)), {
                ($cwd ? (cwd => $cwd) : ()),
		stdin  => $stdin,
                stdout => $output_fh || *STDOUT,
                stderr => $output_fh || *STDERR,
                new_group => $new_group,
            }
	);

	if ($nice) {
	    require Win32::Process;
	    Win32::Process::Open(my $proc, $pid, 0);
	    # BELOW_NORMAL_PRIORITY_CLASS (not defined by Win32::Process)
	    $proc->SetPriorityClass(0x00004000);
	}

	if ($limit_output || $tee) {
            require Time::HiRes;
	    my $before = Time::HiRes::time();
	    $job->watch(sub {
		$tail_f->() if $tee;
		return $should_kill->($before);
            }, 1);
        }
	else {
	    $job->run($limit_time, 0);
	}
	$tail_f->() if $tee;

        $? = ($job->status->{$pid}{exitcode} & 0xff) << 8;
    }
    else {
        # unix
        my $pid = fork;
        die "Can't fork: $!" unless defined $pid;
        if ($pid) {
            if ($limit_time || $limit_output || $tee) {
                require POSIX;
                require Time::HiRes;
                my $before = Time::HiRes::time();
                my $killed_softly;
                my $killed_hard;
                while (1) {
                    select(undef, undef, undef, 0.5);
                    #print "Checking on $pid...\n";
                    $tail_f->() if $tee;
                    last if waitpid($pid, POSIX::WNOHANG()) == $pid;
                    if ($killed_hard) {
                        if (++$killed_hard > 5) {
                            # give up the wait
                            die "Process $pid refuse to die";
                        }
                    }
                    elsif ($killed_softly) {
                        _echo_cmd("kill", "-9", $pid) unless $silent;
                        kill +($new_group ? -9 : 9), $pid;
                        $killed_hard++;
                    }
                    else {
                        if ($should_kill->($before)) {
                            _echo_cmd("kill", "-15", $pid) unless $silent;
                            kill +($new_group ? -15 : 15), $pid;
                            $killed_softly++;
                        }
                    }
                }
            }
            else {
                waitpid($pid, 0);
            }
        }
        else {
            local $SIG{__DIE__};
            eval {
                if ($stdin) {
                    open(STDIN, "<&", $stdin)
                        unless fileno(STDIN) != fileno($stdin);
                }
                else {
                    open(STDIN, "<", "/dev/null");
                }

                if ($output) {
                    open(STDOUT, ">", $output) || die "Can't open '$output': $!";
                    open(STDERR, ">&STDOUT") || die "Can't redirect STDERR: $!";
                }

                if ($env_hide) {
                    for my $k (@$env_hide) {
                        delete $ENV{$k};
                    }
                }

                if ($env) {
                    while (my($k, $v) = each %$env) {
                        $ENV{$k} = $v;
                    }
                }

                if ($new_group) {
                    require POSIX;
                    POSIX::setsid() || die "Can't start new group: $!";
                }

                if (%ulimit || $nice) {
                    require BSD::Resource;
                    BSD::Resource::setpriority() if $nice;
                    while (my($k, $v) = each %ulimit) {
                        no strict 'refs';
                        my $limit = &{"BSD::Resource::RLIMIT_" . uc($k)}();
                        BSD::Resource::setrlimit($limit, $v, $v);
                    }
                }

                if ($cwd) {
                    chdir($cwd) || die "Can't chdir '$cwd': $!";
                    $ENV{PWD} = $cwd if exists $ENV{PWD};
                }

                if ($exe) {
                    exec $exe @$cmd;
                }
                else {
                    exec @$cmd;
                }
            };
            print STDERR $@ || "exec failed: $!\n";
            eval {
                # want to avoid perl destructors from running in child
                require POSIX;
                POSIX::_exit(1);
            };
            exit 1;  # if POSIX fails to load
        }
    }

    if ($? && !$ignore_err) {
        Carp::croak(
	    ($kill_reason ? "$kill_reason\n" : "") .
	    ($exe || $cmd->[0]) .
	    " " . decode_status()
	)
    }

    return $?;
}

sub shell_quote {
    my @copy;
    for (defined(wantarray) ? (@copy = @_) : @_) {
	if ($^O eq "MSWin32") {
	    s/(\\*)\"/$1$1\\\"/g;
	    $_ = qq("$_") if /\s/ || $_ eq "";
	}
	else {
	    if ($_ eq "" || /[^\w\.\-\/]/) {
		s/([\\\$\"\`])/\\$1/g;
		$_ = qq("$_");
	    }
	}
    }
    wantarray ? @copy : join(" ", @copy);
}

sub decode_status {
    my $rc = shift || $?;

    my $exit_status = ($rc & 0xff00) >> 8;
    my $signal = $rc & 0x7f;
    my $dumped_core = $rc & 0x80;
    my $ifstopped = ($rc & 0xff) == 0x7f;
    my $ifexited = $signal == 0;
    my $ifsignaled = !$ifstopped && !$ifexited;

    return (WIFEXITED   => $ifexited,
            $ifexited ? (WEXITSTATUS => $exit_status) : (),
            WIFSIGNALED => $ifsignaled,
            $ifsignaled ? (WTERMSIG    => $signal) : (),
            WIFSTOPPED  => $ifstopped,
            $ifstopped ? (WSTOPSIG    => $exit_status) : (),
            WCOREDUMP   => $dumped_core) if wantarray;

    my $msg = "";
    $msg .= " exits with $exit_status" if $ifexited and $exit_status;
    $msg .= " killed by signal $signal" if $ifsignaled;
    $msg .= " stopped by signal $exit_status" if $ifstopped;
    $msg .= " (core dumped)" if $dumped_core;
    $msg =~ s/^\s//;
    return $msg;
}

1;

=head1 NAME

ActiveState::Run - Collection of small utility functions

=head1 SYNOPSIS

 use ActiveState::Run qw(run);
 run("ls -l");

=head1 DESCRIPTION

This module provides a collection of small utility functions for
running external programs.

The following functions are provided:

=over 4

=item decode_status( )

=item decode_status( $rc )

Will decode the given return code (defaults to $?) and return the 
exit value, the signal it was killed with, and if it dumped core.

In scalar context, it will return a string explaining what happened, or 
an empty string if no error occured.

  my $foo = `ls`;
  my $err = decode_status;
  die "ls failed: $err" if $err;

In array context, it will return a list of key/value pairs containing:

=over 4

=item WIFEXITED

True when the status code indicates normal termination.

=item WEXITSTATUS

If WIFEXITED, this will contain the low-order 8 bits of the status
value the child passed to exit or returned from main.

=item WIFSIGNALED

Non-zero if process was terminated by a signal.

=item WTERMSIG

If WIFSIGNALED, the terminating signal.

=item WIFSTOPPED

Non-zero if the process was stopped.

=item WSTOPSIG

If WIFSTOPPED, the signal that stopped the process.

=item WCOREDUMP

Nonzero if the process dumped core.

=back

Example:

  my $foo = `ls`;
  my %err = decode_status;
  die "ls dumped core" if $err{WCOREDUMP};

=item run( $cmd, @args )

Works like the builtin system() but will by default print commands to
stdout before it execute them and raise an exception (die) if the
command fails (returns non-zero status).  Like for the command
specifications for make(1), you can prefix the command with "@" to
suppress the echo and with "-" to suppress the status check.

The environment variables AS_RUN_SILENT and AS_RUN_PREFIX influence
printing as well, see L<"ENVIRONMENT">.

=item run_ex( %opt )

The extended version of the run function with many additional ways to
control the how the command runs, but otherwise it acts as run().

The following options are recognized:

=over

=item cmd => $cmd

=item cmd => [$cmd, @args]

Specify the command line to run.  It does not support the '@' and '-'
prefixes that the run() command allow.  This option is not optional :)

=item exe => $path

Only use this if you want to override what executable actually runs.
Can be used to lie about what program actually runs, as this allow
argv[0] to be different than the actual command.

=item cwd => $path

Make this the current directory of the process.  By default, the
process shares the parent's current directory.

=item env => \%hash

Override the environment for the process.

=item env_hide => \@keys

List of environment variables that will not be passed to the kid.  You
might pass this as C<< [keys %ENV] >> if you want the process to start
out with an environment that only consist of what you passed in with the
C<env> option.

=item stdin => $filehandle

Make the new process run with stdin from the given filehandle.  If not
given the process will be started with F</dev/null> as its input.  If
you want the process to inherit the input of the parent you need to
pass C<*STDIN> explictly.

=item output => $path

Redirect the combined STDOUT and STDERR to the given file.  The
command will croak if the file can't be opened.  If not specified,
then the STDOUT and STDERR of the process is simply inherited from the
parent.

=item silent => $bool

If TRUE don't echo commands as they are executed.

=item ignore_err => $bool

If TRUE don't croak if the command exits with a non-zero status.

=item nice => $bool

Be nice!

=item tee => $bool

If TRUE send the output captured in the C<output> file to the current STDOUT
as well.  No effect unless C<output> is specified.

=item new_group => $bool

If TRUE start a new process group for the process.

=item limit_time => $seconds

Kill the process (or the process group if C<new_group> was specified)
if it runs for longer than the specified number of seconds.

=item limit_output => $megabytes

Kill the process (or the process group if C<new_group> was specified)
if it output file grows bigger than the specified number of mega
bytes.  No effect unless C<output> was specified.

=item limit_cpu => $seconds

=item limit_XXX => $megabytes

Other limits might also be passed which will set how much resources the process
is allowed to use.  The unit for all size limits are megabytes.  See
C<BSD::Resource> for allowed values.  Also consult the C<ulimit> command in you
shell.

=back

=item shell_quote( @args )

Will quote the arguments provided so that they can be passed to the
command shell without interpretation by the shell.  This is useful
with run() when you can't provide separate @args, e.g.:

   run(shell_quote("rm", "-f", @files) . " >dev/null");

In list context it returns the same number of values as arguments
passed in.  Only those arg values that need quoting will be quoted.

In scalar context it will return a single string with all the quoted
@args separated by space.

In void context it will attempt inline modification of the @args
passed.

=back

=head1 ENVIRONMENT

If the AS_RUN_SILENT environment variable is TRUE, then printing of
the command about to run for run() is suppressed.

If the AS_RUN_PREFIX environment variable is set, then the printed
command is prefixed with the given string.  If AS_RUN_SILENT is TRUE,
then this value is ignored.

=head1 BUGS

none.

=cut
