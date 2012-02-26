% Copyright (C) 2005-2012 Julien Pommier.
%
% This file is a part of GETFEM++
%
% Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
% under  the  terms  of the  GNU  Lesser General Public License as published
% by  the  Free Software Foundation;  either version 2.1 of the License,  or
% (at your option) any later version.
% This program  is  distributed  in  the  hope  that it will be useful,  but
% WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
% or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License for more details.
% You  should  have received a copy of the GNU Lesser General Public License
% along  with  this program;  if not, write to the Free Software Foundation,
% Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.


function check_workspace(iverbose,idebug)
  global gverbose;
  global gdebug;  
  if (nargin >= 1),
    gverbose = iverbose;
    if (nargin == 2),
      gdebug = idebug;
    else 
      gdebug = 0; end;
  else 
    gverbose = 0;
  end;
  gf_workspace('clear all');
  gf_workspace('stats');
  gf_workspace('push');
  m=gf_mesh('empty',1);
  mf=gf_mesh_fem(m);
  gf_workspace('stats');
  gf_workspace('pop');
  gf_workspace('push','foo');
  m=gf_mesh('empty',2);
  mf=gf_mesh_fem(m);
  gf_workspace('keep',mf);
  gf_workspace('pop');
  gf_workspace('stats');
  gf_delete(mf);
  asserterr('gf_delete(mf)');
  gf_workspace('stats');
  gf_workspace('clear all');
