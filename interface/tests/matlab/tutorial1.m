m = gf_mesh('cartesian',[0:.1:1],[0:.1:1]);
mf = gf_mesh_fem(m,1); % create a meshfem of for a field of dimension 1 (i.e. a scalar field)
gf_mesh_fem_set(mf,'fem',gf_fem('FEM_QK(2,2)'));

disp(gf_fem_get(gf_fem('FEM_QK(2,2)'), 'poly_str'));

mim=gf_mesh_im(m, gf_integ('IM_EXACT_PARALLELEPIPED(2)'));

border = gf_mesh_get(m,'outer faces');
gf_mesh_set(m, 'region', 42, border); % create the region (:#(B42

% the boundary edges appears in red
gf_plot_mesh(m, 'regions', [42], 'vertices','on','convexes','on'); 

b0=gf_mdbrick('generic elliptic',mim,mf)
gf_mdbrick_get(b0, 'param list')
b1=gf_mdbrick('dirichlet',b0,42,mf,'penalized')
R=gf_mesh_fem_get(mf, 'eval', {'(x-.5).^2 + (y-.5).^2 + x/5 - y/3'});
gf_mdbrick_set(b1, 'param', 'R', mf, R); 

mds=gf_mdstate(b1)
gf_mdbrick_get(b1, 'solve', mds)

U=gf_mdstate_get(mds, 'state');
gf_plot(mf, U, 'mesh','on');