// -*- c++ -*- (enables emacs c++ mode)
//===========================================================================
//
// Copyright (C) 2006-2008 Yves Renard, Julien Pommier.
//
// This file is a part of GETFEM++
//
// Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
// under  the  terms  of the  GNU  Lesser General Public License as published
// by  the  Free Software Foundation;  either version 2.1 of the License,  or
// (at your option) any later version.
// This program  is  distributed  in  the  hope  that it will be useful,  but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
// You  should  have received a copy of the GNU Lesser General Public License
// along  with  this program;  if not, write to the Free Software Foundation,
// Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
//===========================================================================

#include <map>
#include <getfemint_misc.h>
#include <getfemint_mesh.h>
#include <getfemint_convex_structure.h>
#include <getfemint_pgt.h>
#include <getfem/getfem_export.h>

using namespace getfemint;

static void check_empty_mesh(const getfem::mesh *pmesh)
{
  if (pmesh->dim() == bgeot::dim_type(-1) || pmesh->dim() == 0) {
    THROW_ERROR( "mesh object has an invalid dimension");
  }
}

static void
get_structure_or_geotrans_of_convexes(const getfem::mesh& m, 
				      mexargs_in& in, mexargs_out& out, 
				      id_type class_id)
{  
  dal::bit_vector cvlst;
  if (in.remaining()) cvlst = in.pop().to_bit_vector(&m.convex_index());
  else cvlst = m.convex_index();
  std::vector<id_type> ids; ids.reserve(cvlst.card());
  for (dal::bv_visitor cv(cvlst); !cv.finished(); ++cv) {
    if (class_id == CVSTRUCT_CLASS_ID)
      ids.push_back(ind_convex_structure(m.structure_of_convex(cv)));
    else ids.push_back(ind_pgt(m.trans_of_convex(cv)));
  }
  out.return_packed_obj_ids(ids, class_id);
}

/* apply geomtric transformation to edge lists */
static void
transform_edge_list(const getfem::mesh &m, unsigned N, const bgeot::edge_list &el, darray &w)
{
  bgeot::edge_list::const_iterator it;
  size_type cv = size_type(-1);
  getfem::mesh::ref_convex cv_ref;
  bgeot::pgeometric_trans pgt = NULL;
  size_type ecnt = 0;
  for (it = el.begin(); it != el.end(); it++, ecnt++) {
    if (cv != (*it).cv) {
      cv = (*it).cv;
      cv_ref = m.convex(cv);
      pgt = m.trans_of_convex(cv);
    }
    /* build the list of points on the edge, on the reference element */
    /* get local point numbers in the convex */
    bgeot::size_type iA = m.local_ind_of_convex_point(cv, (*it).i);
    bgeot::size_type iB = m.local_ind_of_convex_point(cv, (*it).j);

    if (iA >= cv_ref.nb_points() ||
	iB >= cv_ref.nb_points() || iA == iB) {
      THROW_INTERNAL_ERROR;
    }
    getfem::base_node A = pgt->convex_ref()->points()[iA];
    getfem::base_node B = pgt->convex_ref()->points()[iB];
    for (size_type i = 0; i < N; i++) {
      getfem::base_node pt;
      
      pt = A +  (B-A)*(double(i)/double(N-1));
      pt = pgt->transform(pt, m.points_of_convex(cv));
      std::copy(pt.begin(), pt.end(), &w(0,i, ecnt));
    }
  }
}

typedef dal::dynamic_tree_sorted<convex_face> mesh_faces_list;

static void
faces_from_pid(const getfem::mesh &m, mexargs_in &in, mexargs_out &out)
{
  mesh_faces_list lst;
  dal::bit_vector convex_tested, pts = in.pop().to_bit_vector(&m.points().index());

  for (dal::bv_visitor ip(pts); !ip.finished(); ++ip) {
    /* iterator over all convexes attached to point ip */
    bgeot::mesh_structure::ind_cv_ct::const_iterator 
      cvit = m.convex_to_point(ip).begin(),
      cvit_end = m.convex_to_point(ip).end();

    for ( ; cvit != cvit_end; cvit++) {
      size_type ic = *cvit;
      if (!convex_tested.is_in(ic)) {
	convex_tested.add(ic);
	/* loop over the convex faces */
	for (short_type f = 0; f < m.structure_of_convex(ic)->nb_faces(); f++) {
	  bgeot::mesh_structure::ind_pt_face_ct pt = m.ind_points_of_face_of_convex(ic, f);
	  bool ok = true;
	  for (unsigned ii=0; ii < pt.size(); ++ii) {
	    if (!pts.is_in(pt[ii])) { 
	      ok = false; break; 
	    }
	  }
	  if (ok) lst.add(convex_face(ic,f));
	}
      }
    }
  }
  iarray w = out.pop().create_iarray(2,unsigned(lst.size()));
  for (size_type j=0; j < lst.size(); j++) { 
    w(0,j) = int(lst[j].cv+config::base_index()); 
    w(1,j) = int(lst[j].f+config::base_index()); 
  }
}


struct mesh_faces_by_pts_list_elt  {
  std::vector<size_type> ptid; // point numbers of faces
  int cnt; // number of convexes sharing that face
  int cv, f;
  inline bool operator < (const mesh_faces_by_pts_list_elt &e) const
  {
    if (ptid.size() < e.ptid.size()) return true;
    if (ptid.size() > e.ptid.size()) return false;
    return ptid < e.ptid;
  }

  mesh_faces_by_pts_list_elt(size_type cv_, size_type f_,
		    std::vector<size_type>& p) : cv(int(cv_)), f(int(f_)) {
    cnt = 0;
    if (p.size() == 0) THROW_INTERNAL_ERROR;
    std::sort(p.begin(), p.end());
//     cerr << "ajout cv=" << cv << " f=" << f << " pt=["; 
//     std::copy(p.begin(), p.end(), std::ostream_iterator<size_type,char>(cerr, ","));
//     cerr << "]\n";
    ptid = p; 
  }
  mesh_faces_by_pts_list_elt() {}
};
typedef dal::dynamic_tree_sorted<mesh_faces_by_pts_list_elt> mesh_faces_by_pts_list;


static void
outer_faces(const getfem::mesh &m, mexargs_in &in, mexargs_out &out)
{
  mesh_faces_by_pts_list lst;
  dal::bit_vector convex_tested;
  dal::bit_vector cvlst;

  if (in.remaining()) cvlst = in.pop().to_bit_vector(&m.convex_index());
  else cvlst = m.convex_index();
  
  for (dal::bv_visitor ic(cvlst); !ic.finished(); ++ic) {
    if (m.structure_of_convex(ic)->dim() == m.dim()) {
      for (short_type f = 0; f < m.structure_of_convex(ic)->nb_faces(); f++) {
	bgeot::mesh_structure::ind_pt_face_ct pt = m.ind_points_of_face_of_convex(ic, f);
	std::vector<size_type> p(pt.begin(), pt.end());
	size_type idx = lst.add_norepeat(mesh_faces_by_pts_list_elt(ic,f,p));
	//       cerr << " <-- idx = " << idx << endl;
	lst[idx].cnt++;
      }
    } else { /* les objets de dim inferieure sont considérés comme "exterieurs" 
		(c'ets plus pratique pour faire des dessins)
	      */
      bgeot::mesh_structure::ind_cv_ct pt = m.ind_points_of_convex(ic);		
      std::vector<size_type> p(pt.begin(), pt.end());
      size_type idx = lst.add_norepeat(mesh_faces_by_pts_list_elt(ic,size_type(-1),p));
      lst[idx].cnt++;
    }
  }
  size_type fcnt = 0;
  for (size_type i = 0; i < lst.size(); i++) {
    fcnt += (lst[i].cnt == 1 ? 1 : 0);
  }

  iarray w = out.pop().create_iarray(2,unsigned(fcnt));
  fcnt = 0;
  for (size_type j=0; j < lst.size(); j++) { 
    if (lst[j].cnt == 1) {
      w(0,fcnt) = lst[j].cv+config::base_index(); 
      w(1,fcnt) = lst[j].f+config::base_index(); 
      fcnt++;
    }
  }
}


static bgeot::base_node 
normal_of_face(const getfem::mesh& mesh, size_type cv, bgeot::dim_type f, size_type node) {
  if (!mesh.convex_index().is_in(cv)) THROW_BADARG("convex " << cv+1 << " not found in mesh");
  if (f >= mesh.structure_of_convex(cv)->nb_faces()) 
    THROW_BADARG("convex " << cv+1 << " has only " << 
		 mesh.structure_of_convex(cv)->nb_faces() << 
		 ": can't find face " << f+1);
  if (node >= mesh.structure_of_convex(cv)->nb_points_of_face(f))
    THROW_BADARG("invalid node number: " << node);
  bgeot::base_node N = mesh.normal_of_face_of_convex(cv, f, node);
  N /= gmm::vect_norm2(N);
  gmm::clean(N, 1e-14);
  return N;
}

/*MLABCOM

  FUNCTION [...] = gf_mesh_get(mesh M, [operation [, args]])

  General mesh inquiry function. All these functions accept also a
  mesh_fem argument instead of a mesh M (in that case, the mesh_fem
  linked mesh will be used). Note that when your mesh is recognized as
  a Matlab object , you can simply use "get(M, 'dim')" instead of
  "gf_mesh_get(M, 'dim')".

  @RDATTR MESH:GET('dim')
  @GET    MESH:GET('pts')
  @RDATTR MESH:GET('nbpts')
  @RDATTR MESH:GET('nbcvs')
  @GET    MESH:GET('pid')
  @GET    MESH:GET('cvid')
  @GET    MESH:GET('max pid')
  @GET    MESH:GET('max cvid')
  @GET    MESH:GET('pid from cvid')
  @GET    MESH:GET('pid from coords')
  @GET    MESH:GET('cvid from pid')
  @GET    MESH:GET('orphaned pid')
  @GET    MESH:GET('faces from pid')
  @GET    MESH:GET('faces from cvid')
  @GET    MESH:GET('outer faces')
  @GET    MESH:GET('edges')
  @GET    MESH:GET('curved edges')
  @GET    MESH:GET('triangulated surface')
  @GET    MESH:GET('normal of face')
  @GET    MESH:GET('normal of faces')
  @GET    MESH:GET('quality')
  @GET    MESH:GET('cvstruct')
  @GET    MESH:GET('geotrans')
  @GET    MESH:GET('regions')
  @GET    MESH:GET('region')
  @GET    MESH:GET('save')
  @GET    MESH:GET('char')
  @GET    MESH:GET('export to vtk')
  @GET    MESH:GET('export to dx')
  @GET    MESH:GET('memsize')

  $Id$
MLABCOM*/


void gf_mesh_get(getfemint::mexargs_in& in, getfemint::mexargs_out& out)
{
  if (in.narg() < 2) {
    THROW_BADARG( "Wrong number of input arguments");
  }

  const getfem::mesh *pmesh = in.pop().to_const_mesh();
  std::string cmd                  = in.pop().to_string();
  if (check_cmd(cmd, "dim", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MESH:GET('dim')
      Get the dimension of the mesh (2 for a 2D mesh, etc).
      @*/
    out.pop().from_integer(pmesh->dim());
  } else if (check_cmd(cmd, "nbpts", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MESH:GET('nbpts')
      Get the number of points of the mesh.
      @*/
    out.pop().from_integer(int(pmesh->nb_points()));
  } else if (check_cmd(cmd, "nbcvs", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MESH:GET('nbcvs')
      Get the number of convexes of the mesh.
      @*/
    out.pop().from_integer(int(pmesh->nb_convex()));
  } else if (check_cmd(cmd, "pts", in, out, 0, 1, 0, 1)) {
    /*@GET [@dmat PT]=MESH:GET('pts' [, @ivec PIDLST])
      Return the list of point coordinates of the mesh.\\\\
      
      Each column of the returned matrix contains the coordinates of
      one point.  If the optional argument PIDLST was given, only the
      points whose #id is listed in this vector are
      returned. Otherwise, the returned matrix will have
      MESH:GET('max_pid') columns, which might be greater than
      MESH:GET('nbpts') (if some points of the mesh have been
      destroyed and no call to MESH:SET('optimize structure') have
      been issued).  The columns corresponding to deleted points
      will be filled with NaN. You can use MESH:GET('pid') to filter
      such invalid points. @*/
    double nan = get_NaN();
    if (!in.remaining()) {
      dal::bit_vector bv = pmesh->points().index();
      darray w = out.pop().create_darray(pmesh->dim(), unsigned(bv.last_true()+1));
      for (size_type j = 0; j < bv.last_true()+1; j++) {
	for (size_type i = 0; i < pmesh->dim(); i++) {
	  w(i,j) = (bv.is_in(j)) ? (pmesh->points()[j])[i] : nan;
	}
      }
    } else {
      dal::bit_vector pids = in.pop().to_bit_vector();
      darray w = out.pop().create_darray(pmesh->dim(), unsigned(pids.card()));
      size_type cnt=0;
      for (dal::bv_visitor j(pids); !j.finished(); ++j) {
	if (!pmesh->points().index().is_in(j))
	  THROW_ERROR("point " << j+config::base_index() << " is not part of the mesh");
	for (size_type i = 0; i < pmesh->dim(); i++) {
	  w(i,cnt) = (pmesh->points()[j])[i];
	}
	cnt++;
      }
    }
  } else if (check_cmd(cmd, "pid", in, out, 0, 0, 0, 1)) {
    /*@GET [@ivec PID]=MESH:GET('pid')
      Return the list of points #id of the mesh.\\\\

      Note that their numbering is not supposed to be contiguous from
      @MATLAB{1 to MESH:GET('nbpts')}@PYTHON{0 to MESH:GET('nbpts')-1}, 
      especially if you destroyed some convexes. You
      can use MESH:SET('optimize_structure') to enforce a contiguous
      numbering. @MATLAB{PID is a row vector.}
    @*/

    out.pop().from_bit_vector(pmesh->points().index());
  } else if (check_cmd(cmd, "cvid", in, out, 0, 0, 0, 1)) {
    /*@GET [CVID] = MESH:GET('cvid')
      Return the list of all convex #id.\\\\

      Note that their numbering is not supposed to be contiguous from
      @MATLAB{1 to MESH:GET('nbcvs')}@PYTHON{0 to MESH:GET('nbcvs')-1}, 
      especially if some points have been removed from the mesh. You
      can use MESH:SET('optimize_structure') to enforce a contiguous
      numbering. @MATLAB{CVID is a row vector.}
      @*/
    out.pop().from_bit_vector(pmesh->convex_index());
  } else if (check_cmd(cmd, "max pid", in, out, 0, 0, 0, 1)) {
    /*@GET MESH:GET('max pid')
      Return the maximum #id of all points in the mesh (see 'max cvid').
      @*/
    int i = pmesh->points().index().card() ? int(pmesh->points().index().last_true()) : -1;
    out.pop().from_integer(i + config::base_index());
  } else if (check_cmd(cmd, "max cvid", in, out, 0, 0, 0, 1)) {
    /*@GET MESH:GET('max cvid')
      Return the maximum #id of all convexes in the mesh (see 'max pid').
      @*/
    int i = pmesh->convex_index().card()? int(pmesh->convex_index().last_true()) : -1;
    out.pop().from_integer(i + config::base_index());
  } else if (check_cmd(cmd, "pid from cvid", in, out, 0, 1, 0, 2)) {
    /*@GET  [PID,IDX]=MESH:GET('pid from cvid' [,CVLST])
      Return the points attached to each convex of the mesh.\\\\

      If CVLST is omitted, all the convexes will be considered (equivalent to CVLST = @MESH:GET('max cvid')).
      IDX is a @MATLAB{row }vector, length(IDX) = length(CVLIST)+1. PID is a @MATLAB{row }vector containing the concatenated list of points of each convex in
      cvlst. Each entry of IDX is the position of the corresponding convex
      point list in PID. Hence, for example, the list of points of the
      second convex is @MATLAB{PID(IDX(2):IDX(3)-1)}@PYTHON{PID[IDX(2):IDX(3)]}.\\

      If CVLST contains convex #id which do not exist in the mesh, their
      point list will be empty.
      @*/
    dal::bit_vector cvlst;
    if (in.remaining()) cvlst = in.pop().to_bit_vector();
    else cvlst.add(0, pmesh->convex_index().last_true()+1);
    
    size_type pcnt = 0;
    /* phase one: count the total number of pids */
    for (dal::bv_visitor cv(cvlst); !cv.finished(); ++cv) {
      if (pmesh->convex_index().is_in(cv)) {
	pcnt += pmesh->structure_of_convex(cv)->nb_points();
      }
    }
    /* phase two: allocation */
    iarray pid = out.pop().create_iarray_h(unsigned(pcnt));
    bool fill_idx = out.remaining();
    iarray idx;
    if (fill_idx) idx = out.pop().create_iarray_h(unsigned(cvlst.card() + 1));

    pcnt = 0;
    size_type cvcnt = 0;
    /* phase three: build the list */
    for (dal::bv_visitor cv(cvlst); !cv.finished(); ++cv) {
      if (fill_idx) idx[cvcnt] = int(pcnt + config::base_index());
      if (pmesh->convex_index().is_in(cv)) {
	for (bgeot::mesh_structure::ind_cv_ct::const_iterator pit = 
	       pmesh->ind_points_of_convex(cv).begin();
	     pit != pmesh->ind_points_of_convex(cv).end(); ++pit) {
	  pid[pcnt++] = int((*pit) + config::base_index());
	}
      }
      cvcnt++;
    }
    if (fill_idx) idx[idx.size()-1] = int(pcnt+config::base_index()); /* for the last convex */
  } else if (check_cmd(cmd, "edges", in, out, 0, 2, 0, 2)) {
    /*@GET [E,C]=MESH:GET('edges' [, CVLST][,'merge'])
      [OBSOLETE FUNCTION! will be removed in a future release]\\\\

      Return the list of edges of mesh M for the convexes listed in the
      row vector CVLST. E is a 2 x nb_edges matrix containing point
      indices. If CVLST is omitted, then the edges of all convexes are
      returned. If CVLST has two rows then the first row is supposed to
      contain convex numbers, and the second face numbers, of which the
      edges will be returned.  If 'merge' is indicated, all common
      edges of convexes are merged in a single edge.  If the optional
      output argument C is specified, it will contain the convex number
      associated with each edge.@*/
    bgeot::edge_list el;
    build_edge_list(*pmesh, el, in);
    iarray w   = out.pop().create_iarray(2, unsigned(el.size()));
    /* copy the edge list to the matlab array */
    for (size_type j = 0; j < el.size(); j++) { 
      w(0,j) = int(el[j].i + config::base_index());
      w(1,j) = int(el[j].j + config::base_index());
    }
    if (out.remaining()) {
      iarray cv = out.pop().create_iarray_h(unsigned(el.size()));
      for (size_type j = 0; j < el.size(); j++) { 
	cv[j] = int(el[j].cv+config::base_index());
      }
    }
  } else if (check_cmd(cmd, "curved edges", in, out, 1, 2, 0, 2)) {    
    /*@GET [E,C]=MESH:GET('curved edges', @int N [, CVLST])
      [OBSOLETE FUNCTION! will be removed in a future release]\\\\

      More sophisticated version of MESH:GET('edges') designed for
      curved elements. This one will return N (N>=2) points of the
      (curved) edges. With N==2, this is equivalent to
      MESH:GET('edges'). Since the points are no more always part of
      the mesh, their coordinates are returned instead of points
      number, in the array E which is a [ mesh_dim x 2 x nb_edges ]
      array.  If the optional output argument C is specified, it will
      contain the convex number associated with each edge.
      @*/
    bgeot::edge_list el;
    size_type N = in.pop().to_integer(2,10000);
    build_edge_list(*pmesh, el, in);
    darray w   = out.pop().create_darray(pmesh->dim(), unsigned(N),
					 unsigned(el.size()));
    transform_edge_list(*pmesh, unsigned(N), el, w);
    if (out.remaining()) {
      iarray cv = out.pop().create_iarray_h(unsigned(el.size()));
      for (size_type j = 0; j < el.size(); j++) { 
	cv[j] = int(el[j].cv + config::base_index());
      }
    }
  } else if (check_cmd(cmd, "pid from coords", in, out, 1, 1, 0, 1)) {
    /*@GET [@ivec PIDLST]=MESH:GET('pid from coords', @mat PT)
      Search point #id whose coordinates are listed in PT.\\\\

      PT is an array containing a list of point coordinates. On return, PIDLST is a @MATLAB{row }vector containing points #id for each point found, and -1 
      -1 for those which where not found in the mesh.
      @*/
    check_empty_mesh(pmesh);
    darray v   = in.pop().to_darray(pmesh->dim(), -1);
    iarray w   = out.pop().create_iarray_h(v.getn());
    for (unsigned j=0; j < v.getn(); j++) {
      getfem::base_node P = v.col_to_bn(j);
      getfem::size_type id = size_type(-1);
      if (!is_NaN(P[0])) id = pmesh->search_point(P);
      if (id == getfem::size_type(-1)) w[j] = -1;
      else w[j] = int(id + config::base_index());
    }
  } else if (check_cmd(cmd, "orphaned pid", in, out, 0, 0, 0, 1)) {
    /*@GET [@ivec PIDLST]=MESH:GET('orphaned pid')
      Returns the list of mesh nodes which are not linked to a convex.
      @*/
    dal::bit_vector bv = pmesh->points().index();
    for (dal::bv_visitor cv(pmesh->convex_index()); !cv.finished(); ++cv) {
      for (unsigned i=0; i < pmesh->nb_points_of_convex(cv); ++i)
        bv.sup(pmesh->ind_points_of_convex(cv)[i]);
    }
    out.pop().from_bit_vector(bv);
  } else if (check_cmd(cmd, "cvid from pid", in, out, 1, 1, 0, 1)) {
    /*@GET [@ivec CVLST]=MESH:GET('cvid from pid', @ivec PIDLST)
      Returns the convex #ids that share the point #ids given in PIDLST@MATLAB{ in a row vector (possibly empty)}.
      @*/
    check_empty_mesh(pmesh);
    dal::bit_vector pts = in.pop().to_bit_vector(&pmesh->points().index());
    dal::bit_vector cvchecked;

    std::vector<size_type> cvlst;

    /* loop over the points */
    for (dal::bv_visitor ip(pts); !ip.finished(); ++ip) {
      /* iterators over the convexes attached to point ip */
      bgeot::mesh_structure::ind_cv_ct::const_iterator 
	cvit = pmesh->convex_to_point(ip).begin(),
	cvit_end = pmesh->convex_to_point(ip).end();    

      for ( ; cvit != cvit_end; cvit++) {
	size_type ic = *cvit;

	//	cerr << "cv = " << ic+1 << endl;

	if (!cvchecked.is_in(ic)) {
	  bool ok = true;
	  
	  bgeot::mesh_structure::ind_cv_ct cvpt = pmesh->ind_points_of_convex(ic);
	  /* check that each point of the convex is in the list */
	  for (unsigned ii=0; ii < cvpt.size(); ++ii) {
	    if (!pts.is_in(cvpt[ii])) {
	      ok = false; break;
	    }
	  }
	  if (ok) cvlst.push_back(ic);
	  cvchecked.add(ic);
	}
      }
    }
    iarray w = out.pop().create_iarray_h(unsigned(cvlst.size()));
    for (size_type j=0; j < cvlst.size(); j++)
      w[j] = int(cvlst[j]+config::base_index());
  } else if (check_cmd(cmd, "faces from pid", in, out, 1, 1, 0, 1)) {
    /*@GET [@imat CVFLST]=MESH:GET('faces from pid', @ivec PIDLST)
      Return the convex faces whose vertex #ids are in PIDLST.\\\\
  
      For a convex face to be returned, EACH of its points have to be
      listed in PIDLST. On output, the first row of CVFLST contains
      the convex number, and the second row contains the face number
      (local number in the convex). @*/
    check_empty_mesh(pmesh);
    faces_from_pid(*pmesh, in, out);
  } else if (check_cmd(cmd, "outer faces", in, out, 0, 1, 0, 1)) {
    /*@GET [CVFLST]=MESH:GET('outer faces' [, CVLST])
      Return the faces which are not shared by two convexes.

      If CVLST is not given, it basically returns the mesh boundary. If CVLST is given, it returns the boundary of the convex set whose #ids are listed in CVLST.
      @*/
    check_empty_mesh(pmesh);
    outer_faces(*pmesh, in, out);
  } else if (check_cmd(cmd, "faces from cvid", in, out, 0, 2, 0, 1)) {
    /*@GET [@imat CVFLST]=MESH:GET('faces from cvid', @ivec CVLST,[ 'merge'])
      Return a list of convexes faces from a list of convex #id.\\\\
      
      CVFLST is a two-rows matrix, the first row lists convex #ids,
      and the second lists face numbers. The optional argument 'merge'
      merges faces shared by two convexes of CVLST.
      @*/
    check_empty_mesh(pmesh);
    dal::bit_vector bv;
    if (in.remaining()) bv = in.pop().to_bit_vector(&pmesh->convex_index());
    else bv = pmesh->convex_index();
    bool merge = false; 
    if (in.remaining()) {
      std::string s = in.pop().to_string();
      if (cmd_strmatch(s, "merge")) merge = true; else bad_cmd(s);
    }
    getfem::mesh_region flst;
    //getfem::convex_face_ct flst;
    size_type cnt = 0;
    for (dal::bv_visitor cv(bv); !cv.finished(); ++cv, ++cnt) {
      for (short_type f=0; f < pmesh->structure_of_convex(cv)->nb_faces(); ++f) {
	bool add = true;
	if (!merge) {
	  bgeot::mesh_structure::ind_set neighbours;
	  pmesh->neighbours_of_convex(cv, f, neighbours);
	  for (bgeot::mesh_structure::ind_set::const_iterator it = neighbours.begin();
	       it != neighbours.end(); ++it) {
	    if (*it < cv) { add = false; break; }
	  }
	}
	if (add) flst.add(cv,f); //push_back(getfem::convex_face(cv,f));
      }
    }
    /*iarray w = out.pop().create_iarray(2,flst.size());
      for (size_type i=0; i < flst.size(); ++i) { 
      w(0,i) = flst[i].cv + config::base_index(); 
      w(1,i) = flst[i].f  + config::base_index(); 
      }*/
    out.pop().from_mesh_region(flst);
  } else if (check_cmd(cmd, "triangulated surface", in, out, 1, 2, 0, 1)) {
    /*@GET [@mat T]=MESH:GET('triangulated surface', @int Nrefine [,CVLIST])
      [OBSOLETE FUNCTION! will be removed in a future release]

      Similar function to MESH:GET('curved edges') : split (if
      necessary, i.e. if the geometric transformation if non-linear)
      each face into sub-triangles and return their coordinates in T
      (see also ::COMPUTE('eval on P1 tri mesh')) @*/
    int Nrefine = in.pop().to_integer(1, 1000);
    std::vector<convex_face> cvf;
    if (in.remaining() && !in.front().is_string()) {
      iarray v = in.pop().to_iarray(-1, -1);
      build_convex_face_lst(*pmesh, cvf, &v);
    } else build_convex_face_lst(*pmesh, cvf, 0);
    eval_on_triangulated_surface(pmesh, Nrefine, cvf, out, NULL, darray());
  } else if (check_cmd(cmd, "normal of face", in, out, 2, 3, 0, 1)) {
    /*@GET [@mat N]=MESH:GET('normal of face', @int CV, @int F[, @int FPTNUM])
      Evaluates the normal of convex CV, face F at the
      FPTNUMth point of the face.

      If FPTNUM is not specified, then the normal is evaluated at each
      geometrical node of the face.
      @*/
    size_type cv = in.pop().to_convex_number(*pmesh);
    size_type f  = in.pop().to_face_number(pmesh->structure_of_convex(cv)->nb_faces());
    size_type node = 0; 
    if (in.remaining()) node = in.pop().to_integer(config::base_index(),10000)-config::base_index(); 
    bgeot::base_node N = normal_of_face(*pmesh, cv, dim_type(f), node);
    out.pop().from_dcvector(N);
  } else if (check_cmd(cmd, "normal of faces", in, out, 1, 1, 0, 1)) {
    /*@GET [@mat N] = MESH:GET('normal of faces', @imat CVFLST)
      Evaluates (at face centers) the normals of convexes.
      
      CVFLST is supposed to contain convex numbers in its first row
      and convex face number in its second row. @*/
    iarray v            = in.pop().to_iarray(2,-1);
    darray w            = out.pop().create_darray(pmesh->dim(), v.getn());    
    for (size_type j=0; j < v.getn(); j++) {
      size_type cv = v(0,j) - config::base_index();
      size_type f  = v(1,j) - config::base_index();
      bgeot::base_node N = normal_of_face(*pmesh, cv, dim_type(f), 0);
      for (size_type i=0; i < pmesh->dim(); ++i) w(i,j)=N[i];
    }
  } else if (check_cmd(cmd, "quality", in, out, 0, 1, 0, 1)) {
    /*@GET [Q]=MESH:GET('quality' [,@ivec CVLST])
      Return an estimation of the quality of each convex (0 <= Q <= 1).
      @*/
    dal::bit_vector bv; 
    if (in.remaining()) bv = in.pop().to_bit_vector(&pmesh->convex_index());
    else bv = pmesh->convex_index();
    darray w = out.pop().create_darray_h(unsigned(bv.card()));
    size_type cnt = 0;
    for (dal::bv_visitor cv(bv); !cv.finished(); ++cv, ++cnt) 
      w[cnt] = pmesh->convex_quality_estimate(cv);
  } else if (check_cmd(cmd, "convex area", in, out, 0, 1, 0, 1)) {
    /*@GET [Q]=MESH:GET('convex area' [,@ivec CVLST])
      Return an estimation of the area of each convex.
      @*/
    dal::bit_vector bv; 
    if (in.remaining()) bv = in.pop().to_bit_vector(&pmesh->convex_index());
    else bv = pmesh->convex_index();
    darray w = out.pop().create_darray_h(unsigned(bv.card()));
    size_type cnt = 0;
    for (dal::bv_visitor cv(bv); !cv.finished(); ++cv, ++cnt) 
      w[cnt] = pmesh->convex_area_estimate(cv);
  } else if (check_cmd(cmd, "cvstruct", in, out, 0, 1, 0, 2)) {
    /*@GET [CVS, @ivec CV2STRUC]=MESH:GET('cvstruct',[@ivec CVLST])
      Return an array of the convex structures.
      
      If CVLST is not given, all convexes are considered. Each convex
      structure is listed once in CVS, and CV2STRUC maps the convexes
      indice in CVLST to the indice of its structure in CVS. @*/
    get_structure_or_geotrans_of_convexes(*pmesh, in, out, CVSTRUCT_CLASS_ID);
  } else if (check_cmd(cmd, "geotrans", in, out, 0, 1, 0, 2)) {
    /*@GET [GT,GT2STRUCT]=MESH:GET('geotrans',[CVLST])
      Returns an array of the geometric transformations.
      
      See also MESH:GET('cvstruct').
      @*/
    get_structure_or_geotrans_of_convexes(*pmesh, in, out, GEOTRANS_CLASS_ID);
  } else if (check_cmd(cmd, "boundaries", in, out, 0, 0, 0, 1) ||
	     check_cmd(cmd, "regions", in, out, 0, 0, 0, 1)) {
    /*@GET BLST = MESH:GET('regions')
      Return the list of valid regions stored in the mesh.
      @*/
    iarray w = out.pop().create_iarray_h(unsigned(pmesh->regions_index().card()));
    size_type i=0; 
    for (dal::bv_visitor k(pmesh->regions_index()); !k.finished(); ++k, ++i) {
      w[i] = int(k);
    }
    if (i != w.size()) THROW_INTERNAL_ERROR;
  } else if (check_cmd(cmd, "boundary", in, out, 1, 1, 0, 1) ||
	     check_cmd(cmd, "region", in, out, 1, 1, 0, 1)) {
    /*@GET CVFLST = MESH:GET('region', @int rnum)
      Return the list of convexes/faces on the region 'rnum'. 

      On output, the first row of I contains the convex numbers, and
      the second row contains the face numbers (and
      @MATLAB{0}@PYTHON{-1} when the whole convex is in the region).      
      @*/
    int bnum = in.pop().to_integer(1,100000);
    std::vector<unsigned> cvlst;
    std::vector<unsigned> facelst;
    for (getfem::mr_visitor i(pmesh->region(bnum)); !i.finished(); ++i) {
      cvlst.push_back(unsigned(i.cv()));
      facelst.push_back(i.f());
    }
    iarray w = out.pop().create_iarray(2, unsigned(cvlst.size()));
    for (size_type j=0; j < cvlst.size(); j++) { 
      w(0,j) = cvlst[j]+config::base_index(); 
      w(1,j) = facelst[j]+config::base_index(); 
    }
  } else if (check_cmd(cmd, "save", in, out, 1, 1, 0, 0)) {
    /*@GET MESH:GET('save', @string FILENAME)
      Save the mesh object to an ascii file.
      
      This mesh can be restored with MESH:INIT('load', FILENAME). 
      @*/
    std::string fname = in.pop().to_string();
    pmesh->write_to_file(fname);
  } else if (check_cmd(cmd, "char", in, out, 0, 0, 0, 1)) {
    /*@GET S=MESH:GET('char')
      Output a string description of the mesh.
      @*/
    std::stringstream s;
    pmesh->write_to_file(s);
    out.pop().from_string(s.str().c_str());
  } else if (check_cmd(cmd, "export to vtk", in, out, 1, 3, 0, 1)) {
    /*@GET MESH:GET('export to vtk', @str filename, ... [,'ascii'][,'quality']) 
      Exports a mesh to a VTK file . 

      If 'quality' is specified, an estimation of the quality of
      each convex will be written to the file. 
      See also MESHFEM:GET('export to vtk') , SLICE:GET('export to vtk').
      @*/
    bool write_q = false, ascii = false;
    std::string fname = in.pop().to_string();
    while (in.remaining() && in.front().is_string()) {
      std::string cmd2 = in.pop().to_string();
      if (cmd_strmatch(cmd2, "ascii"))
        ascii = true;
      else if (cmd_strmatch(cmd2, "quality"))
        write_q = true;
      else THROW_BADARG("expecting 'ascii' or 'quality', got " << cmd2);
    }
    getfem::vtk_export exp(fname, ascii);
    exp.exporting(*pmesh);
    exp.write_mesh(); if (write_q) exp.write_mesh_quality(*pmesh);
  } else if (check_cmd(cmd, "export to dx", in, out, 1, 3, 0, 1)) {
    /*@GET MESH:GET('export to dx', @str filename, ... [,'ascii'][,'append'][,'as',@str name,[,'serie',@str serie_name]][,'edges']) 
      Exports a mesh to an OpenDX file. 

      See also MESHFEM:GET('export to dx') , SLICE:GET('export to dx').
      @*/
    bool ascii = false, append = false, edges = false;
    std::string fname = in.pop().to_string();
    std::string mesh_name;
    std::string serie_name;
    while (in.remaining() && in.front().is_string()) {
      std::string cmd2 = in.pop().to_string();
      if (cmd_strmatch(cmd2, "ascii"))
        ascii = true;
      else if (cmd_strmatch(cmd2, "edges"))
        edges = true;
      else if (cmd_strmatch(cmd2, "append"))
        append = true;
      else if (cmd_strmatch(cmd2, "as") && in.remaining())
	mesh_name = in.pop().to_string();
      else if (cmd_strmatch(cmd2, "serie") && in.remaining() && mesh_name.size())
	serie_name = in.pop().to_string();
      else THROW_BADARG("expecting 'ascii' or 'append', 'serie', or 'as' got " << cmd2);
    }
    getfem::dx_export exp(fname, ascii, append);
    exp.exporting(*pmesh, mesh_name);
    exp.write_mesh(); 
    if (edges) exp.exporting_mesh_edges();
    if (serie_name.size()) exp.serie_add_object(serie_name,mesh_name);
  } else if (check_cmd(cmd, "memsize", in, out, 0, 0, 0, 1)) {
    /*@GET MESH:GET('memsize')
      Return the amount of memory (in bytes) used by the mesh.
      @*/
    out.pop().from_integer(int(pmesh->memsize()));
  } else bad_cmd(cmd);
}
