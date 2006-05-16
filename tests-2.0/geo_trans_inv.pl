$bin_dir = "$ENV{srcdir}/../bin";
$tmp = `$bin_dir/createmp geo.param`;

sub catch { `rm -f $tmp`; exit(1); }
$SIG{INT} = 'catch';

open(TMPF, ">$tmp") or die "Open file impossible : $!\n";
print TMPF "N = 2;\n";
print TMPF "LX = 1.0;\n";
print TMPF "LY = 1.0;\n";
print TMPF "LZ = 1.0;\n";
print TMPF "MESH_TYPE = 0;\n";
print TMPF "NX = 10;\n";
print TMPF "NB_POINTS = 100;\n";
print TMPF "BASE = 10;\n";
print TMPF "\n\n";
close(TMPF);

$er = 0;
open F, "./geo_trans_inv $tmp 2>&1 |" or die;
while (<F>) {
  # print $_;
  if ($_ =~ /error has been detected/)
  {
    $er = 1;
    print "=============================================================\n";
    print $_, <F>;
  }
}
close(F);
if ($?) { `rm -f $tmp`; exit(1); }
if ($er == 1) { `rm -f $tmp`; exit(1); }
`rm -f $tmp`;