function display(o)
% displays a gfMeshLevelSet object
  ll = gf_mesh_levelset_get(o, 'levelsets');
  s1 = sprintf(['gfMeshLevelSet object: ID=%u [%d bytes] , uses %d levelsets'],...
	       double(o.id),...
	       gf_mesh_levelset_get(o,'memsize'), numel(ll));
  
  m = gf_mesh_levelset_get(o, 'linked_mesh');
  s2 = sprintf(['  linked gfMesh object: dim=%d, nbpts=%d, nbcvs=%d'],...
	       gf_mesh_get(m,'dim'), ...
	       gf_mesh_get(m,'nbpts'), gf_mesh_get(m,'nbcvs'));
  disp(s1);disp(s2);
