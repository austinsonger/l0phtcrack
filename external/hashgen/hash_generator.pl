#!/usr/bin/perl
#__________________________________________________________
# Author:     phillips321 contact through phillips321.co.uk
# License:    CC BY-SA 3.0
# Use:        Hash generator
# Released:   www.phillips321.co.uk
# Thanks to:  atom for the origional script which i have used
# Dependencies:
#	see header below
# ToDo:
#	Make code neater
# ChangeLog:
#	v0.2 - thanks to atom
#	v0.1 - first release

use strict;
use warnings;
use Digest::MD4 qw (md4 md4_hex);
use Digest::MD5 qw (md5 md5_hex);
use Digest::SHA qw (sha1 sha1_hex sha256_hex sha512_hex);
use Crypt::MySQL qw (password41);
use Crypt::PasswdMD5;
use Crypt::PBKDF2;
use Crypt::CBC;
use MIME::Base64;
use Authen::Passphrase::PHPass;
use Authen::Passphrase::LANManager;
use Encode;

my @modes = (0, 11, 21, 100, 101, 111, 112, 121, 122, 131, 132, 300, 400, 500, 900, 1000, 1100, 1400, 1500, 1600, 1700, 1722, 2100, 2400, 2600, 2611, 2711, 2811, 3000, 3100);

if (scalar @ARGV < 1){die ("usage: $0 [mode] ['password' or stdin]
* Hash types:
    0 = MD5
   11 = Joomla
   21 = osCommerce, xt:Commerce
  100 = SHA1
  101 = nsldap, SHA-1(Base64), Netscape LDAP SHA
  111 = nsldaps, SSHA-1(Base64), Netscape LDAP SSHA
  112 = Oracle 11g
  121 = SMF > v1.1
  122 = OSX v10.4, v10.5, v10.6
  131 = MSSQL(2000)
  132 = MSSQL(2005)
  300 = MySQL
  400 = phpass, MD5(Wordpress), MD5(phpBB3)
  500 = md5crypt, MD5(Unix), FreeBSD MD5, Cisco-IOS MD5
  900 = MD4
 1000 = NTLM
 1100 = Domain Cached Credentials, mscash
 1400 = SHA256
 1500 = descrypt, DES(Unix), Traditional DES
 1600 = md5apr1, MD5(APR), Apache MD5
 1700 = SHA512
 1722 = OSX v10.7
 2100 = Domain Cached Credentials2, mscash2
 2400 = Cisco-PIX MD5
 2600 = Double MD5
 2611 = vBulletin < v3.8.5
 2711 = vBulletin > v3.8.5
 2811 = IPB2+, MyBB1.2+
 3000 = LM
 3100 = Oracle 7-10g, DES(Oracle)
");}

my $mode = shift @ARGV;
my $password = shift @ARGV // <STDIN>;

single ($mode, $password);
sub single{ #mode #password
  my $mode = shift;
  my $password = shift;
  if (defined $mode)
  {
    @modes = ($mode);
  }
  for (my $j = 0; $j < scalar @modes; $j++)
  {
    my $mode = $modes[$j];
    if ($mode == 0 || $mode == 100 || $mode == 101 || $mode == 300 || $mode == 600 || $mode == 900 || $mode == 1000 || $mode == 1400 || $mode == 1700 || $mode == 2400 || $mode == 2600)
    {rnd ($mode, 0, $password);}
    elsif ($mode == 11){rnd ($mode, 32, $password);}
    elsif ($mode == 21){rnd ($mode, 2, $password);}
    elsif ($mode == 111){rnd ($mode, 8, $password);}
    elsif ($mode == 112){rnd ($mode, 20, $password);}
    elsif ($mode == 121){my $salt_len = get_random_num (1, 15);rnd ($mode, $salt_len, $password);}
    elsif ($mode == 122){rnd ($mode, 8, $password);}
    elsif ($mode == 131){rnd ($mode, 8, $password);}
    elsif ($mode == 132){rnd ($mode, 8, $password);}
    elsif ($mode == 400){rnd ($mode, 8, $password);}
    elsif ($mode == 500){rnd ($mode, 8, $password);}
    elsif ($mode == 1100){my $salt_len = get_random_num (1, 15);rnd ($mode, $salt_len, $password);}
    elsif ($mode == 1500){rnd ($mode, 2, $password);}
    elsif ($mode == 1600){rnd ($mode, 8, $password);}
    elsif ($mode == 1722){rnd ($mode, 8, $password);}
    elsif ($mode == 2100){my $salt_len = get_random_num (1, 15);rnd ($mode, $salt_len, $password);}
    elsif ($mode == 2611){rnd ($mode, 3, $password);}
    elsif ($mode == 2711){rnd ($mode, 30, $password);}
    elsif ($mode == 2811){rnd ($mode, 5, $password);}
    elsif ($mode == 3000){rnd ($mode, 0, $password);}
    elsif ($mode == 3100){rnd ($mode, 10, $password);}
  }
}

exit;

sub gen_hash{ #mode #password #salt
  my $mode = shift;
  my $word_buf = shift;
  my $salt_buf = shift;
  my $tmp_hash;
  my $hash_buf;
  if ($mode == 0){$hash_buf = md5_hex ($word_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 11){$hash_buf = md5_hex ($word_buf . $salt_buf);$tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 21){$hash_buf = md5_hex ($salt_buf . $word_buf);$tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 100){$hash_buf = sha1_hex ($word_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 101){
    my $ctx = Digest::SHA1->new;
    $ctx->add ($word_buf);
    my $hash_buf = encode_base64 ($ctx->digest);
    chomp ($hash_buf);
    $tmp_hash = sprintf ("{SHA}%s", $hash_buf);}
  if ($mode == 111){ 
    my $ctx = Digest::SHA1->new;
    $ctx->add ($word_buf);
    $ctx->add ($salt_buf);
    my $hash_buf = encode_base64 ($ctx->digest . $salt_buf);
    chomp ($hash_buf);
    $tmp_hash = sprintf ("{SSHA}%s", $hash_buf);}
  if ($mode == 112){
    my $salt_buf_bin = "";
    for (my $pos = 0; $pos < length ($salt_buf); $pos += 2)
    {
      my $num = substr ($salt_buf, $pos, 2);
      $salt_buf_bin .= chr (hex ($num));
    }
    $hash_buf = sha1_hex ($word_buf . $salt_buf_bin);
    $tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 121){$hash_buf = sha1_hex ($salt_buf . $word_buf);$tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 122){
    my $salt_buf_bin = "";
    for (my $pos = 0; $pos < length ($salt_buf); $pos += 2)
    {
      my $num = substr ($salt_buf, $pos, 2);
      $salt_buf_bin .= chr (hex ($num));
    }
    $hash_buf = sha1_hex ($salt_buf_bin . $word_buf);
    $tmp_hash = sprintf ("%s%s", $salt_buf, $hash_buf);}
  if ($mode == 131){
    my $salt_buf_bin = "";
    for (my $pos = 0; $pos < length ($salt_buf); $pos += 2)
    {
      my $num = substr ($salt_buf, $pos, 2);
      $salt_buf_bin .= chr (hex ($num));
    }
    $hash_buf = sha1_hex (encode ("UTF-16LE", $word_buf) . $salt_buf_bin);
    $tmp_hash = sprintf ("0x0100%s%s%s", $salt_buf, "0" x 40, $hash_buf);}
  if ($mode == 132){
    my $salt_buf_bin = "";
    for (my $pos = 0; $pos < length ($salt_buf); $pos += 2)
    {
      my $num = substr ($salt_buf, $pos, 2);
      $salt_buf_bin .= chr (hex ($num));
    }
    $hash_buf = sha1_hex (encode ("UTF-16LE", $word_buf) . $salt_buf_bin);
    $tmp_hash = sprintf ("0x0100%s%s", $salt_buf, $hash_buf);}
  if ($mode == 300){$hash_buf = substr (password41 ($word_buf), 1);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 400){
    my $ppr = Authen::Passphrase::PHPass->new
    (
      cost => 11,
      salt => $salt_buf,
      passphrase => $word_buf,
    );
    $hash_buf = $ppr->as_rfc2307;
    $tmp_hash = sprintf ("%s", substr ($hash_buf, 7));}
  if ($mode == 500){$hash_buf = unix_md5_crypt ($word_buf, $salt_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 900){$hash_buf = md4_hex ($word_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 1000){$hash_buf = md4_hex (encode ("UTF-16LE", $word_buf));$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 1100){
    $hash_buf = md4_hex (md4 (encode ("UTF-16LE", $word_buf)) . encode ("UTF-16LE", lc ($salt_buf))),
    $tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 1400){$hash_buf = sha256_hex ($word_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 1500){$hash_buf = crypt ($word_buf, $salt_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 1600){$hash_buf = apache_md5_crypt ($word_buf, $salt_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 1700){$hash_buf = sha512_hex ($word_buf);$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 1722){
    my $salt_buf_bin = "";
    for (my $pos = 0; $pos < length ($salt_buf); $pos += 2){my $num = substr ($salt_buf, $pos, 2);$salt_buf_bin .= chr (hex ($num));}
    $hash_buf = sha512_hex ($salt_buf_bin . $word_buf);
    $tmp_hash = sprintf ("%s%s", $salt_buf, $hash_buf);}
  if ($mode == 2100){
    my $salt = encode ("UTF-16LE", lc ($salt_buf));
    my $pbkdf2 = Crypt::PBKDF2->new
    (
      hash_class => 'HMACSHA1',
      iterations => 10240,
      output_len => 16,
      salt_len   => length ($salt),
    );
    my $hash_buf = unpack ("H*", $pbkdf2->PBKDF2 ($salt, md4 (md4 (encode ("UTF-16LE", $word_buf)) . $salt)));
    $tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);
  }
  if ($mode == 2400){$tmp_hash = sprintf ("%s", pseudo_base64 (Digest::MD5::md5 ($word_buf . "\0" x (16 - length ($word_buf)))));}
  if ($mode == 2600){$hash_buf = md5_hex (md5_hex ($word_buf));$tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 2611){$hash_buf = md5_hex (md5_hex ($word_buf) . $salt_buf);$tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 2711){$hash_buf = md5_hex (md5_hex ($word_buf) . $salt_buf);$tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 2811){$hash_buf = md5_hex (md5_hex ($salt_buf) . md5_hex ($word_buf));$tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  if ($mode == 3000){
    my $ppr = Authen::Passphrase::LANManager->new ("passphrase" => $word_buf);
    $hash_buf = $ppr->hash_hex;
    $tmp_hash = sprintf ("%s", $hash_buf);}
  if ($mode == 3100){$hash_buf = oracle_hash ($salt_buf, $word_buf);$tmp_hash = sprintf ("%s:%s", $hash_buf, $salt_buf);}
  return ($tmp_hash);
}
sub rnd{ #mode #saltlength #password
  my $mode = shift;
  my $salt_len = shift;
  my $password = shift;
  my $word_buf = $password;
  my @salt_arr;
  for (my $i = 0; $i < $salt_len; $i++){my $c = get_random_chr (0x30, 0x39);push (@salt_arr, $c);}
  my $salt_buf = join ("", @salt_arr); 
  my $tmp_hash = gen_hash ($mode, $word_buf, $salt_buf);
  printf ("$tmp_hash\n");
}

sub get_random_num{ #min #max
  my $min = shift;
  my $max = shift;
  return int ((rand ($max - $min)) + $min);}
sub get_random_chr{return chr get_random_num (@_);}
sub pseudo_base64{
    my $itoa64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    my $md5 = shift;
    my $s64 = "";
    for my $i (0..3) {
        my $v = unpack "V", substr($md5, $i*4, 4);
        for (1..4) {$s64 .= substr($itoa64, $v & 0x3f, 1);$v >>= 6;}
    }
    return $s64;}
sub oracle_hash{
  my ($username, $password) = @_;
  my $userpass = pack('n*', unpack('C*', uc($username.$password)));
  $userpass .= pack('C', 0) while (length($userpass) % 8);
  my $key = pack('H*', "0123456789ABCDEF");
  my $iv = pack('H*', "0000000000000000");
  my $c = new Crypt::CBC(
    -literal_key => 1,
    -cipher => "DES",
    -key => $key,
    -iv => $iv,
    -header => "none"
  );
  my $key2 = substr($c->encrypt($userpass), length($userpass)-8, 8);
  my $c2 = new Crypt::CBC(
    -literal_key => 1,
    -cipher => "DES",
    -key => $key2,
    -iv => $iv,
    -header => "none"
  );
  my $hash = substr($c2->encrypt($userpass), length($userpass)-8, 8);
  return uc(unpack('H*', $hash));}
