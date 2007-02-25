function display(b)
% displays a gfMdBrick object
  s = sprintf(['gfMdBrick object: ID=%u [%d bytes], dim=%d, '...
	       'nb_dof=%d, nb_constraints=%d, nb_mixed=%d,\n'...
	       '  is_complex=%d,is_linear=%d,is_coercive=%d,'...
	       'is_symmetric=%d'],double(b.id),...
	       gf_mdbrick_get(b,'memsize'), gf_mdbrick_get(b, 'dim'),... 
	       gf_mdbrick_get(b, 'nbdof'), gf_mdbrick_get(b, 'nb_constraints'), ...
	       numel(gf_mdbrick_get(b, 'mixed_variables')), gf_mdbrick_get(b, 'is_complex'),...
	       gf_mdbrick_get(b, 'is_linear'), gf_mdbrick_get(b, 'is_coercive'), ...
	       gf_mdbrick_get(b, 'is_symmetric'));
	       
  disp(s);
  disp(sprintf(' subclass: %s', gf_mdbrick_get(b, 'subclass'))); 
  l = gf_mdbrick_get(b, 'param_list');
  if (numel(l)),
    s = ''; for il=1:numel(l), s = [s  ' '  l{il}]; end;
    disp(sprintf(' parameters:     %s', s)); 
  end;		     