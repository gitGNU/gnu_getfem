% Copyright (C) 2005-2017 Julien Pommier.
%
% This file is a part of GetFEM++
%
% GetFEM++  is  free software;  you  can  redistribute  it  and/or modify it
% under  the  terms  of the  GNU  Lesser General Public License as published
% by  the  Free Software Foundation;  either version 3 of the License,  or
% (at your option) any later version along with the GCC Runtime Library
% Exception either version 3.1 or (at your option) any later version.
% This program  is  distributed  in  the  hope  that it will be useful,  but
% WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
% or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License and GCC Runtime Library Exception for more details.
% You  should  have received a copy of the GNU Lesser General Public License
% along  with  this program;  if not, write to the Free Software Foundation,
% Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

errcnt=0;
t = 'check_integ [integration methods]            ';
try
  check_integ;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_fem [finite element methods]           ';
try
  check_fem;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_geotrans [geometric transformations]   ';
try
  check_geotrans;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_workspace [objects management]         ';
try
  check_workspace;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_mesh_fem [mesh_fem manipulations]      ';
try
  check_mesh_fem;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_asm [assembly routines]                ';
try
  check_asm;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_oo [pseudo object oriented interface]  ';
try
  check_oo;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_slices [mesh slicing functions]        ';
try
  check_slices;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_spmat [sparse matrix functions]        ';
try
  check_spmat;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'check_plasticity [model brick complex generic assembly] ';
try
  check_plasticity;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ' : FAILURE']);
end;

t = 'demo_laplacian [model use for solving a Poisson problem] ';
try
  automatic_var654 = 1;
  demo_laplacian;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'demo_laplacian_DG [model use for solving a Poisson problem] ';
try
  automatic_var654 = 1;
  demo_laplacian_DG;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'demo_periodic_laplacian [model use for solving a Poisson problem]        ';
try
  automatic_var654 = 1;
  demo_periodic_laplacian;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;

t = 'demo_refine [adaptative refinement for an elastostatic problem] ';
try
  automatic_var654 = 1;
  demo_refine;
  disp(['== ' t ': SUCCESS']);
catch
  errcnt=errcnt+1; disp(['== ' t ': FAILURE']);
end;


if (errcnt),
  disp(sprintf('\n\n== %d/11 tests FAILED\n', errcnt));
else
  disp(sprintf('\n\n== All tests succeeded\n'));
end;
disp('end of check_all..');
