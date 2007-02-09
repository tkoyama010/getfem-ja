function display(mim)
% displays a mesh_im object
  disp(sprintf(['gfMeshIm object: ID=%u [%d bytes]\n'...
		'  linked gfMesh object: dim=%d, nbpts=%d, nbcvs=%d'],double(mim.id),...
	       gf_mesh_im_get(mim,'memsize'), ...
	       gf_mesh_get(mim,'dim'), ...
	       gf_mesh_get(mim,'nbpts'), gf_mesh_get(mim,'nbcvs')));
