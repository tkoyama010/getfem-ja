function varargout=get(obj,varargin)
% gfSlice/get.m
% see gf_slice_get for more help.
if (nargout),
 [varargout{1:nargout}]=gf_slice_get(obj, varargin{:});
else
 gf_slice_get(obj,varargin{:});
 if (exist('ans','var') == 1), varargout{1}=ans;
end;
end
% autogenerated by gen_mfiles