# This is CPAN.pm's systemwide configuration file provided for
# ActivePerl. This file provides defaults for users, and the values
# can be changed in a per-user configuration file. The user-config
# file is being looked for as ~/.cpan/CPAN/MyConfig.pm.

my $CPAN_HOME = "$ENV{HOME}/.cpan";
$CPAN_HOME = do {require Config; "$Config::Config{prefix}/cpan"}
    if $^O eq "MSWin32";

my $SHELL = $ENV{SHELL};
$SHELL ||= $ENV{COMSPEC} if $^O eq "MSWin32";

my $PAGER = $ENV{PAGER} || "more";

$CPAN::Config = {
  'auto_commit' => 0,
  'build_cache' => "10",
  'build_dir' => "$CPAN_HOME/build",
  'cache_metadata' => 1,
  'colorize_output' => 1,
  'colorize_print' => "green",
  'colorize_warn' => "red",
  'cpan_home' => $CPAN_HOME,
  'ftp' => '',
  'ftp_proxy' => '',
  'getcwd' => '',
  'gpg' => '',
  'gzip' => '',
  'histfile' => "$CPAN_HOME/histfile",
  'histsize' => 100,
  'http_proxy' => '',
  'inactivity_timeout' => 0,
  'index_expire' => 1,
  'inhibit_startup_message' => 0,
  'keep_source_where' => "$CPAN_HOME/sources",
  'load_module_verbosity' => 'none',
  'lynx' => '',
  'make' => '',
  'make_arg' => '',
  'make_install_arg' => '',
  'makepl_arg' => 'INSTALLDIRS=site',
  'mbuild_arg' => '',
  'mbuild_install_arg' => '',
  'mbuildpl_arg' => '--installdirs=site',
  'no_proxy' => '',
  'pager' => $PAGER,
  'prerequisites_policy' => 'follow',
  'scan_cache' => 'atstart',
  'shell' => $SHELL,
  'tar' => '',
  'term_is_latin' => 1,
  'unzip' => '',
  'urllist' => ['http://ppm.activestate.com/CPAN', 'http://cpan.perl.org'],
  'wget' => '',
  'yaml_module' => q[YAML::XS],
};

if ($^O eq "MSWin32") {
    $ENV{TERM} = "dumb";
    $CPAN::Config->{colorize_output} = eval { require Win32::Console::ANSI };
}
else {
    $CPAN::Config->{mbuild_install_build_command} = './Build install';
}

1;
