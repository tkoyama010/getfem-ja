function [m,b]=gfMeshFem(a,varargin)
% gfMeshFem class constructor (see the function gf_mesh_fem for argument
% list)
  
  this_class = 'gfMeshFem';
  if (nargin==0) error('can''t create an empty mesh_fem reference'); end;
  if (isa(a,this_class)),
    m=a;
  else
    
    if (isstruct(a) & isfield(a,'id') & isfield(a,'cid'))
      cname = gf_workspace('class name',a);
    else
      cname = class(a);
    end;
    if (strcmp(cname, this_class))
      m = a;
    elseif (strcmp(cname, 'gfMesh') |...
	    (ischar(a))),
      m = gf_mesh_fem(a,varargin{:});
    else
      error(['can''t create a ' this_class ' object from a ' cname ...
	     ' object']);
    end;
    m.txt = '';
    m = class(m, this_class);
  end;
  