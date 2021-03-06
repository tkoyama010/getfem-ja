// Copyright (C) 1994, 1995, 1996, 1997, 1999, 2000, 2003, 2005, 2006,
// 2007, 2008 John W. Eaton
//
// This file is part of Octave.
//
// Octave is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or (at
// your option) any later version.
//
// Octave is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Octave; see the file COPYING. If not, see
// <http://www.gnu.org/licenses/>.

// -*- texinfo -*-
// @deftypefn {Function File} {} null (@var{a}, @var{tol})
// Return an orthonormal basis of the null space of @var{a}.
//
// The dimension of the null space is taken as the number of singular
// values of @var{a} not greater than @var{tol}. If the argument @var{tol}
// is missing, it is computed as
//
// @example
// max (size (@var{a})) * max (svd (@var{a})) * eps
// @end example
// @end deftypefn

// Author: KH <Kurt.Hornik@wu-wien.ac.at>
// Created: 24 December 1993.
// Adapted-By: jwe

function retval = null_space(A, tol)
[nargout,nargin] = argn();

if (isempty (A)) then
  retval = [];
else
  [U, S, V] = svd (A);

  [rows, cols] = size (A);

  [S_nr, S_nc] = size (S);

  if (S_nr == 1 | S_nc == 1) then
    s = S(1);
  else
    s = diag (S);
  end

  if (nargin == 1) then
    tol = max (size (A)) * s (1) * %eps;
  elseif (nargin ~= 2) then
    error('null(A,tol)');
  end

  _rank = sum (s > tol);

  if (_rank < cols) then
    retval = V (:, _rank+1:cols);
    retval(abs (retval) < %eps) = 0;
  else
    retval = zeros (cols, 0);
  end
end
endfunction 
