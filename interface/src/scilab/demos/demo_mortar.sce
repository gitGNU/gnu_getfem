// do a partition of the mesh into two disjoint areas, and then
// solve the linear elasticity problem with a mortar join on 
// the interface between the two areas

gf_workspace('clear all'); 

NX     = 9;
radius = 0.3;
xc     = .5;
yc     = .5;

m = gf_mesh('cartesian', 0:1/NX:1, 0:1/NX:1);
[pid,idx] = gf_mesh_get(m, 'pid_from_cvid');
P = gf_mesh_get(m,'pts');

areap=[];
for cv=1:(length(idx)-1)
  areap(cv) = 1;
  for i=idx(cv):(idx(cv+1)-1)
    if (norm(P(:,pid(i)) - [xc;yc]) > radius) then
      areap(cv)=0;
      break;
    end
  end
end

mfu  = gf_mesh_fem(m, 2); gf_mesh_fem_set(mfu, 'fem', gf_fem('FEM_QK(2,2)'));
mfd  = gf_mesh_fem(m, 1); gf_mesh_fem_set(mfd, 'fem', gf_fem('FEM_QK(2,1)'));
mfm  = gf_mesh_fem(m, 2); gf_mesh_fem_set(mfm, 'fem', gf_fem('FEM_QK(2,2)'));
mfdu = gf_mesh_fem(m);    gf_mesh_fem_set(mfdu,'fem', gf_fem('FEM_QK_DISCONTINUOUS(2,2)'));
mim  = gf_mesh_im(m, 5);

gf_mesh_fem_set(mfu, 'dof_partition', areap);

b_in     = gf_mesh_get(m, 'outer faces', find(areap==1));
b_out    = gf_mesh_get(m, 'outer faces', find(areap==0));
b_border = gf_mesh_get(m, 'outer faces');
b_out    = setdiff(b_out', b_border', 'rows')';

fleft  = gf_mesh_get(m,'faces from pid',find(abs(P(1,:))<1e-6));
fright = gf_mesh_get(m,'faces from pid',find(abs(P(1,:) - 1)<1e-6));

// assign boundary numbers
gf_mesh_set(m,'region',1,fleft);
gf_mesh_set(m,'region',2,fright);

MORTAR_BOUNDARY_IN  = 40;
MORTAR_BOUNDARY_OUT = 41;
gf_mesh_set(m, 'region', MORTAR_BOUNDARY_IN, b_in);
gf_mesh_set(m, 'region', MORTAR_BOUNDARY_OUT, b_out);

gf_plot_mesh(m,'boundaries',40);
disp('This is the mortar interface (press a key to continue)'); pause;

indm = gf_mesh_fem_get(mfm, 'basic dof on region', MORTAR_BOUNDARY_OUT);
expr = 'M(#1,#2)+=comp(vBase(#1).vBase(#2))(:,i,:,i)';
M    =   gf_asm('boundary', MORTAR_BOUNDARY_IN , expr, mim, mfm, mfu);
M    = M-gf_asm('boundary', MORTAR_BOUNDARY_OUT, expr, mim, mfm, mfu);
M    = M(indm, :);

md = gf_model('real');
gf_model_set(md, 'add fem variable', 'u', mfu);
gf_model_set(md, 'add initialized data', 'lambda', [1]);
gf_model_set(md, 'add initialized data', 'mu', [1]);
gf_model_set(md, 'add isotropic linearized elasticity brick', mim, 'u', 'lambda', 'mu');
gf_model_set(md, 'add Dirichlet condition with multipliers', mim, 'u', mfu, 1);
F = gf_mesh_fem_get(mfd, 'eval', {0; 'y+2'});
gf_model_set(md, 'add initialized fem data', 'VolumicData', mfd, F);
gf_model_set(md, 'add source term brick', mim, 'u', 'VolumicData');
gf_model_set(md, 'add variable', 'mult_spec', length(indm));
gf_model_set(md, 'add constraint with multipliers', 'u', 'mult_spec', M, zeros(length(indm),1));

gf_model_get(md, 'solve');
U = gf_model_get(md, 'variable', 'u');

VM = gf_model_get(md, 'compute isotropic linearized Von Mises or Tresca', 'u', 'lambda', 'mu', mfdu);

gf_plot(mfdu,VM,'deformed_mesh','on', 'deformation',U,	'deformation_mf',mfu,'refine', 4, 'deformation_scale',0.1); 
// caxis([0 500]);
