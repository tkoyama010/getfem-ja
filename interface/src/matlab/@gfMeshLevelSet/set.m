function varargout=set(obj,varargin)
% gfMeshLevelSet/set.m
% see gf_mesh_levelset_set for more help.
if (nargout),
 [varargout{1:nargout}]=gf_mesh_levelset_set(obj, varargin{:});
else
 gf_mesh_levelset_set(obj,varargin{:});
 if (exist('ans','var') == 1), varargout{1}=ans;
end;
end
% autogenerated by gen_mfiles