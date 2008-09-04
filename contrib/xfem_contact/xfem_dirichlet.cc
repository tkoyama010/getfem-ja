// -*- c++ -*- (enables emacs c++ mode)
//===========================================================================
//
// Copyright (C) 2002-2008 Yves Renard, Julien Pommier.
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

/**
 * Goal : scalar Dirichlet problem with Xfem.
 *
 * Research program.
 */

#include "getfem/getfem_assembling.h" /* import assembly methods      */
#include "getfem/getfem_export.h"     /* export functions             */
#include "getfem/getfem_derivatives.h"
#include "getfem/getfem_regular_meshes.h"
#include "getfem/getfem_model_solvers.h"
#include "getfem/getfem_mesh_im_level_set.h"
#include "getfem/getfem_partial_mesh_fem.h"
#include "getfem/getfem_Coulomb_friction.h"
#include "getfem/getfem_import.h"
#include "getfem/getfem_inter_element.h"
#include "gmm/gmm.h"

/* some Getfem++ types that we will be using */
using bgeot::base_small_vector; /* special class for small (dim<16) vectors */
using bgeot::base_vector;
using bgeot::base_node;  /* geometrical nodes(derived from base_small_vector)*/
using bgeot::scalar_type; /* = double */
using bgeot::short_type;  /* = short */
using bgeot::size_type;   /* = unsigned long */
using bgeot::base_matrix; /* small dense matrix. */

/* definition of some matrix/vector types. These ones are built
 * using the predefined types in Gmm++
 */
typedef getfem::modeling_standard_sparse_vector sparse_vector;
typedef getfem::modeling_standard_sparse_matrix sparse_matrix;
typedef getfem::modeling_standard_plain_vector  plain_vector;

typedef gmm::row_matrix<sparse_vector> sparse_row_matrix;

/* 
 * Exact solution 
 */
double Radius;
int u_version;
double u_alpha = 1.5;
double u_B = 20;
double u_n = 7.0;
double dtheta = M_PI/36;

double u_exact(const base_node &p) {
  double R = Radius, r=gmm::vect_norm2(p), T=atan2(p[1], p[0])+dtheta;
  switch (u_version) {
    case 0: {
      double sum = std::accumulate(p.begin(), p.end(), double(0));
      
      return 5.0 * sin(sum) * (r*r - Radius*Radius);
    }
    case 1: {
      double A=u_alpha, n=u_n;
      return (R*R - r*r *(1+A*(1.0 + sin(n*T))));
    }
    case 2: {
      double A=u_alpha, n=u_n, B=u_B;
      return (R*R - r*r *(1+A*(1.0 + sin(n*T)))) * cos(B*r);      
    }
    case 3: {
      double A=u_alpha, n=u_n;
      return 5*(R*R*R*R - r*r*r*r*(1+A*(1.0 + sin(n*T))));      
    }
  }
  GMM_ASSERT1(false, "Invalid exact solution");
}

double g_exact(const base_node &p) {
  // value of the normal derivative. Due to the pb idem than the opposite of
  // norm of the gradient.
  double R = Radius, r=gmm::vect_norm2(p);
  switch (u_version) {
  case 0: {
    double sum=std::accumulate(p.begin(), p.end(), double(0)), norm_sqr = r*r;
    if (norm_sqr < 1e-10) norm_sqr = 1e-10;
    return 5.0 * (sum * cos(sum) * (norm_sqr - R*R)
		  + 2.0 * norm_sqr * sin(sum)) / sqrt(norm_sqr);
  }
  case 1: {
    double A=u_alpha, T=atan2(p[1], p[0])+dtheta, n=u_n;
    return - sqrt(r*r*pow(2.0*sin(T)+2.0*sin(T)*A+2.0*sin(T)*A*sin(n*T)+cos(T)*A*cos(n*T)*n,2.0)+r*r*pow(-2.0*cos(T)-2.0*cos(T)*A-2.0*cos(T)*A*sin(n*T)+sin(T)*A*cos(n*T)*n,2.0));
  } 
  case 2: {
    double A=u_alpha, T=atan2(p[1], p[0])+dtheta, n=u_n,B=u_B;
    if (gmm::abs(r) < 1e-10) r = 1e-10;
    return -(4.0*r*cos(B*r)+8.0*r*cos(B*r)*A+8.0*r*cos(B*r)*A*A
	     +2.0*sin(B*r)*B*R*R-2.0*sin(B*r)*B*r*r
	     +r*A*A*pow(cos(n*T),2.0)*n*n*cos(B*r)+8.0*r*cos(B*r)*A*A*sin(n*T)
	     -4.0*sin(B*r)*B*r*r*A*sin(n*T)
	     +2.0*sin(B*r)*B*r*r*A*A*pow(cos(n*T),2.0)
	     -4.0*r*cos(B*r)*A*A*pow(cos(n*T),2.0)+8.0*r*cos(B*r)*A*sin(n*T)
	     -4.0*sin(B*r)*B*r*r*A*A*sin(n*T)-4.0*sin(B*r)*B*r*r*A*A
	     -4.0*sin(B*r)*B*r*r*A+2.0*sin(B*r)*B*R*R*A
	     +2.0*sin(B*r)*B*R*R*A*sin(n*T))
      / sqrt(A*A*pow(cos(n*T),2.0)*n*n +4.0+8.0*A+8.0*A*sin(n*T)
	     +8.0*A*A+8.0*A*A*sin(n*T)-4.0*A*A*pow(cos(n*T),2.0));
  }
  case 3: {
    double A=u_alpha, T=atan2(p[1], p[0])+dtheta, n=u_n;
    return -5*r*r*r*sqrt(16.0 + 32.0*A*A + 32.0*A*sin(n*T) + 32.0*A
			+ 32.0*A*A*sin(n*T) - 16*gmm::sqr(A*cos(n*T))
		       + gmm::sqr(A*cos(n*T)*n));
  }   
  }
  return 0;
}

double rhs(const base_node &p) {
  double R = Radius, r=gmm::vect_norm2(p);
  switch (u_version) {
  case 0: {
    double sum = std::accumulate(p.begin(), p.end(), double(0));
    double norm_sqr = r*r, N = double(gmm::vect_size(p));
    return 5.0 * (N * sin(sum) * (norm_sqr - R*R-2.0)
		  - 4.0 * sum * cos(sum));
  }
  case 1: {
    double A=u_alpha, T=atan2(p[1], p[0])+dtheta, n=u_n;
    return -(-4.0-4.0*A-4.0*A*sin(n*T)+A*sin(n*T)*n*n);
  }
  case 2: {
    double A=u_alpha, T=atan2(p[1], p[0])+dtheta, n=u_n, B=u_B;
    if (gmm::abs(r) < 1e-10) r = 1e-10;
    return (4.0*r*cos(B*r)*A*sin(n*T)-5.0*sin(B*r)*B*r*r*A+4.0*r*cos(B*r)
	    +4.0*r*cos(B*r)*A+sin(B*r)*B*R*R-5.0*sin(B*r)*B*r*r
	    -5.0*sin(B*r)*B*r*r*A*sin(n*T)-r*r*r*cos(B*r)*B*B
	    -r*A*sin(n*T)*n*n*cos(B*r)+r*cos(B*r)*B*B*R*R
	    -r*r*r*cos(B*r)*B*B*A-r*r*r*cos(B*r)*B*B*A*sin(n*T))/r;
  }
  case 3: {
    double A=u_alpha, T=atan2(p[1], p[0])+dtheta, n=u_n;
    return -5*r*r*(-16.0-16.0*A*sin(n*T)-16.0*A+A*sin(n*T)*n*n);
  }
  }
  return 0;
}

double ls_value(const base_node &p) {
  double R = Radius, r=gmm::vect_norm2(p);
  switch (u_version) {
  case 0: return gmm::vect_norm2_sqr(p) - R*R;
  case 1: case 2: {
    double A=u_alpha, T=atan2(p[1],p[0])+dtheta, n=u_n;
    return -(R*R - r*r*(1+A*(1.0 + sin(n*T)))) / 15.0;
  }
  case 3: {
    double A=u_alpha, T=atan2(p[1], p[0])+dtheta, n=u_n;
    return -(R*R*R*R - r*r*r*r*(1+A*(1.0 + sin(n*T)))) / 4.0;      
  } 
  }
  return 0;
}

/*
 * Test procedure
 */

void test_mim(getfem::mesh_im_level_set &mim, getfem::mesh_fem &mf_rhs,
	      bool bound) {
  if (!u_version) {
    unsigned N =  mim.linked_mesh().dim();
    size_type nbdof = mf_rhs.nb_dof();
    plain_vector V(nbdof), W(1);
    std::fill(V.begin(), V.end(), 1.0);
    
    getfem::generic_assembly assem("u=data(#1); V()+=comp(Base(#1))(i).u(i);");
    assem.push_mi(mim); assem.push_mf(mf_rhs); assem.push_data(V);
    assem.push_vec(W);
    assem.assembly(getfem::mesh_region::all_convexes());
    double exact(0), R2 = Radius*Radius, R3 = R2*Radius;
    switch (N) {
      case 1: exact = bound ? 1.0 : 2.0*Radius; break;
      case 2: exact = bound ? Radius*M_PI : R2*M_PI; break;
      case 3: exact = bound ? 2.0*M_PI*R2 : 4.0*M_PI*R3/3.0; break;
      default: assert(N <= 3);
    }
    if (bound) cout << "Boundary length: "; else cout << "Area: ";
    cout << W[0] << " should be " << exact << endl;
    assert(gmm::abs(exact-W[0])/exact < 0.01); 
  }
}

/*
 * Assembly of stabilization terms
 */


template<typename VECT1> class level_set_unit_normal 
  : public getfem::nonlinear_elem_term {
  const getfem::mesh_fem &mf;
  const VECT1 &U;
  size_type N;
  base_matrix gradU;
  bgeot::base_vector coeff;
  bgeot::multi_index sizes_;
public:
  level_set_unit_normal(const getfem::mesh_fem &mf_, const VECT1 &U_) 
    : mf(mf_), U(U_), N(mf_.linked_mesh().dim()), gradU(1, N)
  { sizes_.resize(1); sizes_[0] = N; }
  const bgeot::multi_index &sizes() const {  return sizes_; }
  virtual void compute(getfem::fem_interpolation_context& ctx,
		       bgeot::base_tensor &t) {
    size_type cv = ctx.convex_num();
    coeff.resize(mf.nb_dof_of_element(cv));
    gmm::copy(gmm::sub_vector(U, gmm::sub_index(mf.ind_dof_of_element(cv))),
	      coeff);
    ctx.pf()->interpolation_grad(ctx, coeff, gradU, 1);
    scalar_type norm = gmm::vect_norm2(gmm::mat_row(gradU, 0));
    for (size_type i = 0; i < N; ++i) t[i] = gradU(0, i) / norm;
    // cout << "point " << ctx.xreal() << " norm = " << t << endl;
  }
};


template<class MAT>
void asm_stabilization_mixed_term
(const MAT &RM_, const getfem::mesh_im &mim, const getfem::mesh_fem &mf, 
 const getfem::mesh_fem &mf_mult, getfem::level_set &ls,
 const getfem::mesh_region &rg = getfem::mesh_region::all_convexes()) {
  MAT &RM = const_cast<MAT &>(RM_);

  level_set_unit_normal<std::vector<scalar_type> >
    nterm(ls.get_mesh_fem(), ls.values());

  getfem::generic_assembly assem("t=comp(Base(#2).Grad(#1).NonLin(#3));"
				 "M(#2, #1)+= t(:,:,i,i)");
  assem.push_mi(mim);
  assem.push_mf(mf);
  assem.push_mf(mf_mult);
  assem.push_mf(ls.get_mesh_fem());
  assem.push_mat(RM);
  assem.push_nonlinear_term(&nterm);
  assem.assembly(rg);
}


template<class MAT>
void asm_stabilization_symm_term
(const MAT &RM_, const getfem::mesh_im &mim, const getfem::mesh_fem &mf, 
 getfem::level_set &ls,
 const getfem::mesh_region &rg = getfem::mesh_region::all_convexes()) {
  MAT &RM = const_cast<MAT &>(RM_);

  level_set_unit_normal<std::vector<scalar_type> >
    nterm(ls.get_mesh_fem(), ls.values());

  getfem::generic_assembly
    assem("t=comp(Grad(#1).NonLin(#2).Grad(#1).NonLin(#2));"
	  "M(#1, #1)+= sym(t(:,i,i,:,j,j))");
  assem.push_mi(mim);
  assem.push_mf(mf);
  assem.push_mf(ls.get_mesh_fem());
  assem.push_mat(RM);
  assem.push_nonlinear_term(&nterm);
  assem.assembly(rg);
}

/*
 * Elementary extrapolation matrices
 */

void compute_mass_matrix_extra_element
(base_matrix &M, const getfem::mesh_im &mim, const getfem::mesh_fem &mf,
 size_type cv1, size_type cv2) {

  getfem::pfem pf1_old = 0;
  static getfem::pfem_precomp pfp1 = 0;
  static getfem::papprox_integration pai1_old = 0;
  bgeot::geotrans_inv_convex gic;
  bgeot::base_tensor t1, t2;
  getfem::base_matrix G1, G2;
  
  const getfem::mesh &m(mf.linked_mesh());
  
  GMM_ASSERT1(mf.convex_index().is_in(cv1) && mim.convex_index().is_in(cv1) &&
	      mf.convex_index().is_in(cv2) && mim.convex_index().is_in(cv2),
	      "Bad element");
    
  bgeot::pgeometric_trans pgt1 = m.trans_of_convex(cv1);
  getfem::pintegration_method pim1 = mim.int_method_of_element(cv1);
  getfem::papprox_integration pai1 =
    getfem::get_approx_im_or_fail(pim1);
  getfem::pfem pf1 = mf.fem_of_element(cv1);
  size_type nbd1 = pf1->nb_dof(cv1);
  
  if (pf1 != pf1_old || pai1 != pai1_old) {
    pfp1 = fem_precomp(pf1, &pai1->integration_points(), pim1);
    pf1_old = pf1; pai1_old = pai1;
  }
  
  bgeot::vectors_to_base_matrix(G1, m.points_of_convex(cv1));
  getfem::fem_interpolation_context ctx1(pgt1, pfp1, 0, G1, cv1,size_type(-1));
  
  getfem::pfem pf2 = mf.fem_of_element(cv2);
  size_type nbd2 = pf1->nb_dof(cv2);
  base_node xref2(pf2->dim());
  bgeot::pgeometric_trans pgt2 = m.trans_of_convex(cv2);
  gic.init(m.points_of_convex(cv2), pgt2);

  gmm::resize(M, nbd1, nbd2); gmm::clear(M);

  bgeot::vectors_to_base_matrix(G2, m.points_of_convex(cv2));
  
  getfem::fem_interpolation_context ctx2(pgt2, pf2, base_node(pgt2->dim()),
					 G2, cv2, size_type(-1));

  for (unsigned ii=0; ii < pai1->nb_points_on_convex(); ++ii) {
    ctx1.set_ii(ii);
    scalar_type coeff = pai1->integration_coefficients()[ii] * ctx1.J();
    bool converged;
    gic.invert(ctx1.xreal(), xref2, converged);
    GMM_ASSERT1(converged, "geometric transformation not well inverted ... !");
    // cout << "xref2 = " << xref2 << endl;
    ctx2.set_xref(xref2);

    pf1->real_base_value(ctx1, t1);
    pf2->real_base_value(ctx2, t2);
    
    for (size_type i = 0; i < nbd1; ++i)
      for (size_type j = 0; j < nbd2; ++j)
	M(i,j) += t1[i] * t2[j] * coeff;
  }
  // cout << "M = " << M << endl;
}







/* 
 * Main program 
 */

int main(int argc, char *argv[]) {

  GMM_SET_EXCEPTION_DEBUG; // Exceptions make a memory fault, to debug.
  FE_ENABLE_EXCEPT;        // Enable floating point exception for Nan.
    
  // Read parameters.
  bgeot::md_param PARAM;
  PARAM.read_command_line(argc, argv);
  u_version = PARAM.int_value("EXACT_SOL", "Which exact solution");
  
  // Load the mesh
  getfem::mesh mesh;
  std::string MESH_FILE = PARAM.string_value("MESH_FILE", "Mesh file");
  getfem::import_mesh(MESH_FILE, mesh);
  unsigned N = mesh.dim();
  
  // center the mesh in (0, 0).
  base_node Pmin(N), Pmax(N);
  mesh.bounding_box(Pmin, Pmax);
  Pmin += Pmax; Pmin /= -2.0;
  // Pmin[N-1] = -Pmax[N-1];
  mesh.translation(Pmin);
  scalar_type h = mesh.minimal_convex_radius_estimate();
  cout << "h = " << h << endl;
  
  // Level set definition
  unsigned lsdeg = PARAM.int_value("LEVEL_SET_DEGREE", "level set degree");
  bool simplify_level_set = 
    (PARAM.int_value("SIMPLIFY_LEVEL_SET",
		     "simplification or not of the level sets") != 0);
  Radius = PARAM.real_value("RADIUS", "Domain radius");
  getfem::level_set ls(mesh, lsdeg);
  getfem::level_set lsup(mesh, lsdeg, true), lsdown(mesh, lsdeg, true);
  const getfem::mesh_fem &lsmf = ls.get_mesh_fem();
  for (unsigned i = 0; i < lsmf.nb_dof(); ++i) {
    lsup.values()[i] = lsdown.values()[i] = ls.values()[i]
      = ls_value(lsmf.point_of_dof(i));
    lsdown.values(1)[i] = lsmf.point_of_dof(i)[1];
    lsup.values(1)[i] = -lsmf.point_of_dof(i)[1];
  }
  
  if (simplify_level_set) {
    scalar_type simplify_rate = std::min(0.03, 0.05 * sqrt(h));
    cout << "Simplification of level sets with rate: " <<
      simplify_rate << endl;
    ls.simplify(simplify_rate);
    lsup.simplify(simplify_rate);
    lsdown.simplify(simplify_rate); 
  }
  
  getfem::mesh_level_set mls(mesh), mlsup(mesh), mlsdown(mesh);
  mls.add_level_set(ls);
  mls.adapt();
  mlsup.add_level_set(lsup);
  mlsup.adapt();
  mlsdown.add_level_set(lsdown);
  mlsdown.adapt();
  
  getfem::mesh mcut;
  mls.global_cut_mesh(mcut);
  mcut.write_to_file("cut.mesh");
  
  // Integration method on the domain
  std::string IM = PARAM.string_value("IM", "Mesh file");
  std::string IMS = PARAM.string_value("IM_SIMPLEX", "Mesh file");
  int intins = getfem::mesh_im_level_set::INTEGRATE_INSIDE;
  getfem::mesh_im uncutmim(mesh);
  uncutmim.set_integration_method(mesh.convex_index(),
				  getfem::int_method_descriptor(IM));
  getfem::mesh_im_level_set mim(mls, intins,
				getfem::int_method_descriptor(IMS));
  mim.set_integration_method(mesh.convex_index(),
			     getfem::int_method_descriptor(IM));
  mim.adapt();
  
  
  // Integration methods on the boudary
  int intbound = getfem::mesh_im_level_set::INTEGRATE_BOUNDARY;
  getfem::mesh_im_level_set mimbounddown(mlsdown, intbound,
					 getfem::int_method_descriptor(IMS));
  mimbounddown.set_integration_method(mesh.convex_index(),
				      getfem::int_method_descriptor(IM));
  mimbounddown.adapt();
  getfem::mesh_im_level_set mimboundup(mlsup, intbound,
				       getfem::int_method_descriptor(IMS));
  mimboundup.set_integration_method(mesh.convex_index(),
				    getfem::int_method_descriptor(IM));
  mimboundup.adapt();
  
  // Finite element method for the unknown
  getfem::mesh_fem pre_mf(mesh);
  std::string FEM = PARAM.string_value("FEM", "finite element method");
  pre_mf.set_finite_element(mesh.convex_index(),
			    getfem::fem_descriptor(FEM));
  getfem::partial_mesh_fem mf(pre_mf);
  dal::bit_vector kept_dof = select_dofs_from_im(pre_mf, mim);
  dal::bit_vector rejected_elt;
  for (dal::bv_visitor cv(mim.convex_index()); !cv.finished(); ++cv)
    if (mim.int_method_of_element(cv) == getfem::im_none())
      rejected_elt.add(cv);
  mf.adapt(kept_dof, rejected_elt);
  size_type nb_dof = mf.nb_dof();
  
  // Finite element method for the rhs
  getfem::mesh_fem mf_rhs(mesh);
  std::string FEMR = PARAM.string_value("FEM_RHS", "finite element method");
  mf_rhs.set_finite_element(mesh.convex_index(),
			    getfem::fem_descriptor(FEMR));
  size_type nb_dof_rhs = mf_rhs.nb_dof();
  cout << "nb_dof_rhs = " << nb_dof_rhs << endl;
  
  // A P0 finite element
  const getfem::mesh_fem &mf_P0 = getfem::classical_mesh_fem(mesh, 0);
  
  // Finite element method for the multipliers
  getfem::mesh_fem pre_mf_mult(mesh);
  std::string FEMM = PARAM.string_value("FEM_MULT", "fem for multipliers");
  pre_mf_mult.set_finite_element(mesh.convex_index(),
				 getfem::fem_descriptor(FEMM));
  getfem::partial_mesh_fem mf_mult(pre_mf_mult);
  dal::bit_vector kept_dof_mult
    = select_dofs_from_im(pre_mf_mult, mimbounddown, N-1);
  mf_mult.adapt(kept_dof_mult, rejected_elt);
  size_type nb_dof_mult = mf_mult.nb_dof();
  cout << "nb_dof_mult = " << nb_dof_mult << endl;

 
  // Mass matrix on the boundary
  sparse_matrix B2(mf_rhs.nb_dof(), nb_dof);
  getfem::asm_mass_matrix(B2, mimboundup, mf_rhs, mf);

  sparse_matrix B(nb_dof_mult, nb_dof);
  getfem::asm_mass_matrix(B, mimbounddown, mf_mult, mf);

  int stabilized_dirichlet =
    PARAM.int_value("STABILIZED_DIRICHLET", "Stabilized version of "
		    "Dirichlet condition or not");
  scalar_type dir_gamma0(0);
  sparse_matrix MA(nb_dof_mult, nb_dof_mult), KA(nb_dof, nb_dof);
  sparse_matrix BA(nb_dof_mult, nb_dof);
  if (stabilized_dirichlet > 0) {

    sparse_row_matrix E1(nb_dof, nb_dof);

    if (stabilized_dirichlet == 2) {
      // Computation of the extrapolation operator
      scalar_type min_ratio =
	PARAM.real_value("MINIMAL_ELT_RATIO",
			 "Threshold ratio for the fully stab Dirichlet");

      cout << "Computation of the extrapolation operator" << endl;
      dal::bit_vector elt_black_list, dof_black_list;
      size_type nbe = mf_P0.nb_dof();
      plain_vector ratios(nbe);
      sparse_matrix MC1(nbe, nbe), MC2(nbe, nbe);
      getfem::asm_mass_matrix(MC1, mim, mf_P0);
      getfem::asm_mass_matrix(MC2, uncutmim, mf_P0);
      for (size_type i = 0; i < nbe; ++i) {
	size_type cv = mf_P0.first_convex_of_dof(i);
	ratios[cv] = gmm::abs(MC1(i,i)) / gmm::abs(MC2(i,i));
	if (ratios[cv] > 0 && ratios[cv] < min_ratio) elt_black_list.add(cv);
      }
      
	
      sparse_matrix EO(nb_dof, nb_dof);
      sparse_row_matrix T1(nb_dof, nb_dof), EX(nb_dof, nb_dof);
      asm_mass_matrix(EO, uncutmim, mf);

      for (size_type i = 0; i < nb_dof; ++i) {
	bool found = false;
	getfem::mesh::ind_cv_ct ct = mf.convex_to_dof(i);
	getfem::mesh::ind_cv_ct::const_iterator it;
	for (it = ct.begin(); it != ct.end(); ++it)
	  if (!elt_black_list.is_in(*it)) found = true;
	if (found)
	  { gmm::clear(gmm::mat_col(EO, i)); EO(i,i) = scalar_type(1); }
	else
	  dof_black_list.add(i);
      }

      bgeot::mesh_structure::ind_set is;
      base_matrix Mloc;
      for (dal::bv_visitor i(elt_black_list); !i.finished(); ++i) {
	mesh.neighbours_of_convex(i, is);
	size_type cv2 = size_type(-1);
	scalar_type ratio = scalar_type(0);
	for (size_type j = 0; j < is.size(); ++j) {
	  scalar_type r = ratios[is[j]];
	  if (r > ratio) { ratio = r; cv2 = is[j]; }
	}
	GMM_ASSERT1(cv2 != size_type(-1), "internal error");
	compute_mass_matrix_extra_element(Mloc, uncutmim, mf, i, cv2);
	for (size_type ii = 0; ii < gmm::mat_nrows(Mloc); ++ii) 
	  for (size_type jj = 0; jj < gmm::mat_ncols(Mloc); ++jj)
	    EX(mf.ind_dof_of_element(i)[ii], mf.ind_dof_of_element(cv2)[jj])
	      += Mloc(ii, jj);
      }

      gmm::copy(gmm::identity_matrix(), E1);
      gmm::copy(gmm::identity_matrix(), T1);
      for (dal::bv_visitor i(dof_black_list); !i.finished(); ++i)
	gmm::copy(gmm::mat_row(EX, i), gmm::mat_row(T1, i));

      plain_vector BE(nb_dof), BS(nb_dof);
      for (dal::bv_visitor i(dof_black_list); !i.finished(); ++i) {
	BE[i] = scalar_type(1);
	// TODO: store LU decomp.
	double rcond; 
	gmm::SuperLU_solve(EO, BS, BE, rcond);
	gmm::mult(gmm::transposed(T1), BS, gmm::mat_row(E1, i));
	BE[i] = scalar_type(0);
      }
      gmm::clean(E1, 1e-13);

//       gmm::clean(EO, 1e-13); cout << "E0 = " << gmm::transposed(EO) << endl; getchar();
//       cout << "T1 = " << T1 << endl; getchar();
      

      cout << "Extrapolation operator computed" << endl;

//       sparse_row_matrix A1(nb_dof, nb_dof);
//       gmm::mult(E1, gmm::transposed(EO), A1);
//       gmm::clean(A1, 1e-13);
//       cout << "A1 = " << A1 << endl;

    }

    dir_gamma0 = PARAM.real_value("DIRICHLET_GAMMA0",
				  "Stabilization parameter for "
				  "Dirichlet condition");
    getfem::asm_mass_matrix(MA, mimbounddown, mf_mult);
    asm_stabilization_mixed_term(BA, mimbounddown, mf, mf_mult, lsdown);
    asm_stabilization_symm_term(KA, mimbounddown, mf, lsdown);
    gmm::scale(MA, -dir_gamma0 * h);
    gmm::scale(BA, -dir_gamma0 * h);
    gmm::scale(KA, -dir_gamma0 * h);

    if (stabilized_dirichlet == 2) {
      sparse_matrix A1(nb_dof_mult, nb_dof);
      gmm::copy(BA, A1);
      gmm::mult(gmm::transposed(E1), gmm::transposed(A1), gmm::transposed(BA));
      sparse_matrix A2(nb_dof, nb_dof);
      gmm::mult(gmm::transposed(E1), KA, A2);
      gmm::mult(gmm::transposed(E1), gmm::transposed(A2), gmm::transposed(KA));
    }
    gmm::add(BA, B);
  }

  // Tests
  test_mim(mim, mf_rhs, false);
  test_mim(mimbounddown, mf_rhs, true);

  // Brick system
  getfem::mdbrick_generic_elliptic<> brick_laplacian(mim, mf);
    
  getfem::mdbrick_source_term<> brick_volumic_rhs(brick_laplacian);
  plain_vector F(nb_dof_rhs);
  getfem::interpolation_function(mf_rhs, F, rhs);
  brick_volumic_rhs.source_term().set(mf_rhs, F);

  // Neumann condition
  getfem::interpolation_function(mf_rhs, F, g_exact);
  plain_vector R(nb_dof);
  gmm::mult(gmm::transposed(B2), F, R);
  brick_volumic_rhs.set_auxF(R);

  // Dirichlet condition
  getfem::mdbrick_constraint<> brick_constraint(brick_volumic_rhs);
  brick_constraint.set_constraints(B, plain_vector(nb_dof_mult));
  brick_constraint.set_constraints_type(getfem::AUGMENTED_CONSTRAINTS);
  if (stabilized_dirichlet > 0)
    brick_constraint.set_optional_matrices(KA, MA);

  getfem::mdbrick_abstract<> *final_brick = &brick_constraint;
    
  // Solving the problem
  cout << "Total number of unknown: " << final_brick->nb_dof() << endl;
  getfem::standard_model_state MS(*final_brick);
  gmm::iteration iter(1e-9, 1, 40000);
  getfem::standard_solve(MS, *final_brick, iter);
  plain_vector U(nb_dof);
  gmm::copy(brick_laplacian.get_solution(MS), U);
  plain_vector LAMBDA(nb_dof_mult);
  gmm::copy(brick_constraint.get_mult(MS), LAMBDA);

  // interpolation of the solution on mf_rhs
  plain_vector Uint(nb_dof_rhs), Vint(nb_dof_rhs), Eint(nb_dof_rhs);
  getfem::interpolation(mf, mf_rhs, U, Uint);
  for (size_type i = 0; i < nb_dof_rhs; ++i) {
    Vint[i] = u_exact(mf_rhs.point_of_dof(i));
    Eint[i] = gmm::abs(Uint[i] - Vint[i]);
  }
    
  // computation of error on u.
  double errmax = 0.0, exactmax = 0.0;
  for (size_type i = 0; i < nb_dof_rhs; ++i)
    if (ls_value(mf_rhs.point_of_dof(i)) <= 0.0) {
      errmax = std::max(errmax, Eint[i]);
      exactmax = std::max(exactmax, Vint[i]);
    }
    else Eint[i] = 0.0;
  cout << "Linfty error: " << 100.0 * errmax / exactmax << "%" << endl;
  cout << "L2 error: " << 100.0
    * getfem::asm_L2_dist(mim, mf, U, mf_rhs, Vint)
    / getfem::asm_L2_norm(mim, mf_rhs, Vint) << "%" << endl;
  cout << "H1 error: " << 100.0
    * getfem::asm_H1_dist(mim, mf, U, mf_rhs, Vint)
    / getfem::asm_H1_norm(mim, mf_rhs, Vint) << "%" << endl;

  // computation of error on multipliers.

  gmm::resize(BA, nb_dof_mult, nb_dof_rhs); gmm::clear(BA);
  gmm::resize(KA, nb_dof_rhs, nb_dof_rhs);  gmm::clear(KA);
  gmm::resize(B, nb_dof_mult, nb_dof_mult); gmm::clear(B);
  asm_stabilization_mixed_term(BA, mimbounddown, mf_rhs, mf_mult, lsdown);
  getfem::asm_mass_matrix(B, mimbounddown, mf_mult, mf_mult);
  asm_stabilization_symm_term(KA, mimbounddown, mf_rhs, lsdown);

  scalar_type err_l2_mult =
    ( gmm::vect_sp(B, LAMBDA, LAMBDA) + gmm::vect_sp(KA, Vint, Vint)
      + 2 * gmm::vect_sp(BA, Vint, LAMBDA) ) / gmm::vect_sp(KA, Vint, Vint);
    
  cout << "L2 error on multipliers: "
       << gmm::sgn(err_l2_mult) * gmm::sqrt(gmm::abs(err_l2_mult)) * 100.0
       << "%" << endl;
  // cout << "L2^2 max on multipliers: " << gmm::vect_sp(KA, Vint, Vint);
  // cout << "  LAMBDA^2: " << gmm::vect_sp(B, LAMBDA, LAMBDA);
  // cout << "  Double produit: " <<  2*gmm::vect_sp(BA, Vint, LAMBDA)<<endl;

  // exporting solution in vtk format.
  {
    getfem::vtk_export exp("xfem_dirichlet.vtk", (2==1));
    exp.exporting(mf); 
    exp.write_point_data(mf, U, "solution");
    cout << "export done, you can view the data file with (for example)\n"
      "mayavi -d xfem_dirichlet.vtk -f WarpScalar -m BandedSurfaceMap "
      "-m Outline\n";
  }
  // exporting error in vtk format.
  {
    getfem::vtk_export exp("xfem_dirichlet_error.vtk", (2==1));
    exp.exporting(mf_rhs); 
    exp.write_point_data(mf_rhs, Eint, "error");
    cout << "export done, you can view the data file with (for example)\n"
      "mayavi -d xfem_dirichlet_error.vtk -f WarpScalar -m BandedSurfaceMap "
      "-m Outline\n";
  }
  // exporting multipliers in vtk format.
  {
    getfem::vtk_export exp("xfem_dirichlet_mult.vtk", (2==1));
    exp.exporting(mf_mult); 
    exp.write_point_data(mf_mult, LAMBDA, "multipliers");
    cout << "export done, you can view the data file with (for example)\n"
      "mayavi -d xfem_dirichlet_mult.vtk -f WarpScalar -m BandedSurfaceMap "
      "-m Outline\n";
  }

  lsmf.write_to_file("xfem_dirichlet_ls.mf", true);
  gmm::vecsave("xfem_dirichlet_ls.U", ls.values());

  unsigned nrefine = mf.linked_mesh().convex_index().card() < 200 ? 32 : 4;
  if (1) {
    cout << "saving the slice solution for matlab\n";
    getfem::stored_mesh_slice sl, sl0,sll;
    
    
    getfem::mesh_slicer slicer(mf.linked_mesh());
    getfem::slicer_build_stored_mesh_slice sbuild(sl);
    getfem::mesh_slice_cv_dof_data<plain_vector> mfU(mf,U);
    getfem::slicer_isovalues iso(mfU, 0.0, 0);
    getfem::slicer_build_stored_mesh_slice sbuild0(sl0);
    
    slicer.push_back_action(sbuild);  // full slice in sl
    slicer.push_back_action(iso);     // extract isosurface 0
    slicer.push_back_action(sbuild0); // store it into sl0
    slicer.exec(nrefine, mf.convex_index());
    
    getfem::mesh_slicer slicer2(mf.linked_mesh());
    getfem::mesh_slice_cv_dof_data<plain_vector> 
      mfL(ls.get_mesh_fem(), ls.values());
    getfem::slicer_isovalues iso2(mfL, 0.0, 0);
    getfem::slicer_build_stored_mesh_slice sbuildl(sll);
    slicer2.push_back_action(iso2);     // extract isosurface 0
    slicer2.push_back_action(sbuildl); // store it into sl0
    slicer2.exec(nrefine, mf.convex_index());
    
    sl.write_to_file("xfem_dirichlet.sl", true);
    sl0.write_to_file("xfem_dirichlet.sl0", true);
    sll.write_to_file("xfem_dirichlet.sll", true);
    plain_vector UU(sl.nb_points()), LL(sll.nb_points()); 
    sl.interpolate(mf, U, UU);
    gmm::vecsave("xfem_dirichlet.slU", UU);
    // gmm::scale(LAMBDA, 0.005);
    sll.interpolate(mf_mult, LAMBDA, LL);
    gmm::vecsave("xfem_dirichlet.slL", LL);
  }
  
  return 0; 
}
