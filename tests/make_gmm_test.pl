
# -*- perl -*-
eval 'exec perl -S $0 "$@"'
  if 0;

# � ajouter : - les sous-matrices
#             - Gerer l'interface Lapack, SuperLU et QD.

if ($ARGV[0] eq "") { $nb_iter = 1; } else { $nb_iter = int($ARGV[0]); }
if ($nb_iter == 0) { $nb_iter = 1; }
$srcdir = $ENV{srcdir};
if ($srcdir eq "") { $srcdir="../../tests"; print "WARNING : no srcdir\n"; }
$inc_dir = "$srcdir/../src";

print "Gmm tests : Making $nb_iter execution(s) of each test\n";

for ($iter = 0; $iter < $nb_iter; ++$iter) {

  $tests_to_be_done =  `ls $srcdir/gmm_test*.C`;

  while ($tests_to_be_done) {
    ($org_name, $tests_to_be_done) = split('\s', $tests_to_be_done, 2);

    print "\nTesting $org_name\n";
    $d = $org_name;
    do { ($b, $d) = split('/', $d, 2); } while ($d);
    $dest_name = "auto_$b";
    ($root_name, $d) = split('.C', $dest_name, 2);
    $size_max = 30.0;

    open(DATAF, $org_name) or die "Open input file impossible : $!\n";
    open(TMPF, ">$dest_name") or die "Open output file impossible : $!\n";

    print TMPF "#include<gmm.h>\n\n\n";

    $reading_param = 1;
    $nb_param = 0;

    while (($li = <DATAF> )&& ($reading_param)) {
      chomp($li);
      if ($li=~/ENDPARAM/) { $reading_param = 0; }
      elsif ($li=~/DENSE_VECTOR_PARAM/) { $param[$nb_param++] = 1; }
      elsif ($li=~/VECTOR_PARAM/) { $param[$nb_param++] = 2; }
      elsif ($li=~/RECTANGULAR_MATRIX_PARAM/) { $param[$nb_param++] = 3; }
      elsif ($li=~/SQUARED_MATRIX_PARAM/) { $param[$nb_param++] = 4; }
      else { die "Error in parameter list"; }
    }

    $TYPES[0] = "float";
    $TYPES[1] = "double";
    $TYPES[2] = "std::complex<float> ";
    $TYPES[3] = "std::complex<double> ";
    $NB_TYPES = 4.0;
    $TYPE = $TYPES[int($NB_TYPES * rand)];

    $VECTOR_TYPES[0] = "std::vector<$TYPE> ";
    $VECTOR_TYPES[1] = "std::vector<$TYPE> ";
    $VECTOR_TYPES[2] = "gmm::rsvector<$TYPE> ";
    $VECTOR_TYPES[3] = "gmm::wsvector<$TYPE> ";
    $NB_VECTOR_TYPES = 4.0;

    $MATRIX_TYPES[0] = "gmm::dense_matrix<$TYPE> ";
    $MATRIX_TYPES[1] = "gmm::dense_matrix<$TYPE> ";
    $MATRIX_TYPES[2] = "gmm::row_matrix<std::vector<$TYPE> > ";
    $MATRIX_TYPES[3] = "gmm::col_matrix<std::vector<$TYPE> > ";
    $MATRIX_TYPES[4] = "gmm::row_matrix<gmm::rsvector<$TYPE> > ";
    $MATRIX_TYPES[5] = "gmm::col_matrix<gmm::rsvector<$TYPE> > ";
    $MATRIX_TYPES[6] = "gmm::row_matrix<gmm::wsvector<$TYPE> > ";
    $MATRIX_TYPES[7] = "gmm::col_matrix<gmm::wsvector<$TYPE> > ";
    $NB_MATRIX_TYPES = 8.0;

    while ($li = <DATAF>) { print TMPF $li; }
    $sizep = int($size_max*rand);
    print "Parameters for the test:\n";
    print TMPF "\n\n\n";
    print TMPF "int main(void) {\n\n";
    print TMPF "  try {\n\n";
    for ($j = 0; $j < $nb_param; ++$j) {
      $a = rand;
      $sizepp = $sizep + int(50.0*rand);
      $step = $sizep; if ($step == 0) { ++$step; }
      $step = int(1.0*int($sizepp/$step - 1)*rand) + 1;

      if (($param[$j] == 1) || ($param[$j] == 2)) { # vectors
	$lt = $VECTOR_TYPES[0];
	if ($param[$j] == 2) {
	  $lt = $VECTOR_TYPES[int($NB_VECTOR_TYPES * rand)];
	}
	if ($a < 0.1) {
	  $li = "    $lt param$j($sizepp);";
	  $c = int(1.0*($sizepp-$sizep+1)*rand);
	  $param_name[$j]
	    = "gmm::sub_vector(param$j, gmm::sub_interval($c, $sizep))";
	}
	elsif ($a < 0.2) {
	  $li = "    $lt param$j($sizepp);";
	  $c = int(1.0*($sizepp-($sizep*$step+1))*rand);
	  $param_name[$j]
	    = "gmm::sub_vector(param$j, gmm::sub_slice($c, $sizep, $step))";
	}
	elsif ($a < 0.3) {
	  $li = "    $lt param$j($sizepp);";
	  do {
	    $sub_index[0] = int(2.0*rand);
	    for ($k = 1; $k < $sizep; ++$k)
	      { $sub_index[$k] = $sub_index[$k-1] + int(1.0*$step*rand)+1; }
	  } while ($sub_index[$sizep-1] >  $sizepp);
	  $li= "$li\n    gmm::size_type param_tab$j [$sizep] = {$sub_index[0]";
	  for ($k = 1; $k < $sizep; ++$k) { $li = "$li , $sub_index[$k]"; }
	  $li = "$li};";
	  $param_name[$j] = "gmm::sub_vector(param$j,".
	    " gmm::sub_index(&param_tab$j [0], &param_tab$j [$sizep]))";
	}
	else {
	  $li = "    $lt param$j($sizep);";
	  $param_name[$j] = "param$j";
	}
	print TMPF "$li\n    gmm::fill_random(param$j);\n";
      }
      elsif ($param[$j] == 3) { # rectangular matrices
	$s = int($size_max*rand);
	$lt = $MATRIX_TYPES[int($NB_MATRIX_TYPES * rand)];
	$li = "    $lt param$j($sizep, $s);";
	print TMPF "$li\n    gmm::fill_random(param$j);\n";
	$sizep = $s; $param_name[$j] = "param$j";
      }
      elsif ($param[$j] == 4) { # squared matrices
	$lt = $MATRIX_TYPES[int($NB_MATRIX_TYPES * rand)];
	$li = "    $lt param$j($sizep, $sizep);";
	print TMPF "$li\n    gmm::fill_random(param$j);\n";
	$param_name[$j] = "param$j";
      }
      print "$li ($param_name[$j])\n";
    }
    print TMPF "    \n\n    test_procedure($param_name[0]";
    for ($j = 1; $j < $nb_param; ++$j) { print TMPF ", $param_name[$j]"; }
    print TMPF ");\n\n";
    print TMPF "  }\n";
    print TMPF "  DAL_STANDARD_CATCH_ERROR;\n";
    print TMPF "  return 0;\n";
    print TMPF "}\n";

    close(DATAF);
    close(TMPF);

    `rm -f $root_name`;
    print `make $root_name CPPFLAGS=\"-I$inc_dir -I../src\"`;
    if ($? != 0) {
      print "\n******************************************************\n";
      print "* Compilation error, please submit this bug to\n";
      print "* Yves.Renard\@gmm.insa-tlse.fr, with the file\n";
      print "* $dest_name\n";
      print "* produced in directory \"tests\".\n";
      print "******************************************************\n";
      exit(1);
    }
    print `./$root_name`;
    if ($? != 0) {
      print "\n******************************************************\n";
      print "* Execution error, please submit this bug to\n";
      print "* Yves.Renard\@gmm.insa-tlse.fr, with the file\n";
      print "* $dest_name\n";
      print "* produced in directory \"tests\".\n";
      print "******************************************************\n";
      exit(1);
    }
    `rm -f $dest_name`;

  }

}
