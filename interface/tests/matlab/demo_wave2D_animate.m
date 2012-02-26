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


dt=2*pi/20;
t=0:dt:2*pi-dt/2;
%mov = avifile('example.avi');
for i=1:length(t),  
  disp(sprintf('theta=%1.3f', t(i)));
  gf_plot(mfu,imag(U(:)'*exp(1i*t(i))),'refine',28,'contour',0); 
  axis([-11 11 -11 11]); caxis([-1 1]);
  print(gcf,'-dpng','-r150',sprintf('wave%02d.png',i));
  %F = getframe(gca);
  %mov = addframe(mov,F);
end;
%mov = close(mov);
