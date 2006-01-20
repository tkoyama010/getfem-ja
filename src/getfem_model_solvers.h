// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Library : GEneric Tool for Finite Element Methods (getfem)
// File    : getfem_model_solvers.h : Standard solvers for the brick
//           system.
// Date    : June 15, 2004.
// Author  : Yves Renard <Yves.Renard@insa-toulouse.fr>
//
//========================================================================
//
// Copyright (C) 2004-2006 Yves Renard
//
// This file is a part of GETFEM++
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


/**
   @file getfem_model_solvers.h
   @brief Standard solver for model bricks
   @see getfem_modeling.h
*/

#ifndef GETFEM_MODEL_SOLVERS_H__
#define GETFEM_MODEL_SOLVERS_H__

#include <getfem_modeling.h>
#include <gmm_MUMPS_interface.h>

namespace getfem {

  template <typename T> struct sort_abs_val_
  { bool operator()(T x, T y) { return (gmm::abs(x) < gmm::abs(y)); } };

  template <typename MAT> void print_eigval(const MAT &M) {
    // just to test a stiffness matrix if needing
    typedef typename gmm::linalg_traits<MAT>::value_type T;
    size_type n = gmm::mat_nrows(M);
    gmm::dense_matrix<T> MM(n, n), Q(n, n);
    std::vector<T> eigval(n);
    gmm::copy(M, MM);
    // gmm::symmetric_qr_algorithm(MM, eigval, Q);
    gmm::implicit_qr_algorithm(MM, eigval, Q);
    std::sort(eigval.begin(), eigval.end(), sort_abs_val_<T>());
    cout << "eival = " << eigval << endl;
//     cout << "vectp : " << gmm::mat_col(Q, n-1) << endl;
//     cout << "vectp : " << gmm::mat_col(Q, n-2) << endl;
//     double emax, emin;
//     cout << "condition number" << condition_number(MM,emax,emin) << endl;
//     cout << "emin = " << emin << endl;
//     cout << "emax = " << emax << endl;
  }

  /* ***************************************************************** */
  /*     Line search definition                                        */
  /* ***************************************************************** */

  struct abstract_newton_line_search {
    scalar_type conv_alpha, conv_r;
    size_type it, itmax;
    virtual void init_search(scalar_type r) = 0;
    virtual scalar_type next_try(void) = 0;
    virtual bool is_converged(scalar_type) = 0;
    virtual scalar_type converged_value(void) { return conv_alpha; };
    virtual scalar_type converged_residual(void) { return conv_r; };
  };


  struct simplest_newton_line_search : public abstract_newton_line_search {
    scalar_type alpha, alpha_mult, first_res, alpha_max_ratio, alpha_min;
    virtual void init_search(scalar_type r)
    { conv_alpha = alpha = scalar_type(1); conv_r = first_res = r; it = 0; }
    virtual scalar_type next_try(void)
    { conv_alpha = alpha; alpha *= alpha_mult; ++it; return conv_alpha; }
    virtual bool is_converged(scalar_type r) {
      conv_r = r;
      return ((it <= 1 && r < first_res)
	      || (r <= first_res * alpha_max_ratio)
	      || (conv_alpha <= alpha_min)
	      || it >= itmax);
    }
    simplest_newton_line_search
    (size_type imax = size_type(-1),
     scalar_type a_max_ratio = scalar_type(6)/scalar_type(5),
     scalar_type a_min = scalar_type(1)/scalar_type(1000),
     scalar_type a_mult = scalar_type(3)/scalar_type(5))
      : alpha_mult(a_mult), alpha_max_ratio(a_max_ratio), alpha_min(a_min)
      { itmax = imax; }
  };

  struct basic_newton_line_search : public abstract_newton_line_search {
    scalar_type alpha, alpha_mult, first_res, alpha_max_ratio;
    scalar_type alpha_min, prev_res;
    virtual void init_search(scalar_type r) {
      conv_alpha = alpha = scalar_type(1);
      prev_res = conv_r = first_res = r; it = 0;
    }
    virtual scalar_type next_try(void)
    { conv_alpha = alpha; alpha *= alpha_mult; ++it; return conv_alpha; }
    virtual bool is_converged(scalar_type r) {
      if ((r < first_res / scalar_type(2)) || (conv_alpha <= alpha_min)
	  || it >= itmax)
	{ conv_r = r; return true; }
      if (it > 1 && r > prev_res && prev_res < alpha_max_ratio * first_res)
	return true;
      prev_res = conv_r = r;
      return false;
    }
    basic_newton_line_search
    (size_type imax = size_type(-1),
     scalar_type a_max_ratio = scalar_type(5)/scalar_type(3),
     scalar_type a_min = scalar_type(1)/scalar_type(1000),
     scalar_type a_mult = scalar_type(3)/scalar_type(5))
      : alpha_mult(a_mult), alpha_max_ratio(a_max_ratio),
	alpha_min(a_min) { itmax = imax; }
  };


  struct systematic_newton_line_search : public abstract_newton_line_search {
    scalar_type alpha, alpha_mult, first_res;
    scalar_type alpha_min, prev_res;
    bool first;
    virtual void init_search(scalar_type r) {
      conv_alpha = alpha = scalar_type(1);
      prev_res = conv_r = first_res = r; it = 0; first = true;
    }
    virtual scalar_type next_try(void)
    { scalar_type a = alpha; alpha *= alpha_mult; ++it; return a; }
    virtual bool is_converged(scalar_type r) {
      // cout << "a = " << alpha / alpha_mult << " r = " << r << endl;
      if (r < conv_r || first)
	{ conv_r = r; conv_alpha = alpha / alpha_mult; first = false; }
      if ((alpha <= alpha_min*alpha_mult) || it >= itmax) return true;
      return false;
    }
    systematic_newton_line_search
    (size_type imax = size_type(-1),
     scalar_type a_min = scalar_type(1)/scalar_type(10000),
     scalar_type a_mult = scalar_type(3)/scalar_type(5))
      : alpha_mult(a_mult), alpha_min(a_min)  { itmax = imax; }
  };

  /* ***************************************************************** */
  /*     Linear solvers definition                                     */
  /* ***************************************************************** */

  template <class MAT, class VECT> 
  struct abstract_linear_solver {
    virtual void operator ()(const MAT &, VECT &, const VECT &,
			     gmm::iteration &) const  = 0;
  };

  template <class MAT, class VECT> 
  struct linear_solver_cg_preconditioned_ildlt
    : public abstract_linear_solver<MAT, VECT> {
    void operator ()(const MAT &M, VECT &x, const VECT &b,
		     gmm::iteration &iter)  const {
      gmm::ildlt_precond<MAT> P(M);
      gmm::cg(M, x, b, P, iter);
      if (!iter.converged()) DAL_WARNING2("cg did not converge!");
    }
  };

  template <class MAT, class VECT> 
  struct linear_solver_gmres_preconditioned_ilu
    : public abstract_linear_solver<MAT, VECT> {
    void operator ()(const MAT &M, VECT &x, const VECT &b,
		     gmm::iteration &iter)  const {
      gmm::ilu_precond<MAT> P(M);
      gmm::gmres(M, x, b, P, 300, iter);
      if (!iter.converged()) DAL_WARNING2("gmres did not converge!");
    }
  };

  template <class MAT, class VECT> 
  struct linear_solver_gmres_preconditioned_ilut
    : public abstract_linear_solver<MAT, VECT> {
    void operator ()(const MAT &M, VECT &x, const VECT &b,
		     gmm::iteration &iter)  const {
      gmm::ilut_precond<MAT> P(M, 20, 1E-10);
      gmm::gmres(M, x, b, P, 300, iter);
      if (!iter.converged()) DAL_WARNING2("gmres did not converge!");
    }
  };

  template <class MAT, class VECT> 
  struct linear_solver_superlu
    : public abstract_linear_solver<MAT, VECT> {
    void operator ()(const MAT &M, VECT &x, const VECT &b,
		     gmm::iteration &iter)  const {
      double rcond;
      SuperLU_solve(M, x, b, rcond);
      if (iter.get_noisy()) cout << "condition number: " << 1.0/rcond<< endl;
    }
  };


#ifdef GMM_USES_MUMPS
  template <class MAT, class VECT> 
  struct linear_solver_mumps : public abstract_linear_solver<MAT, VECT> {
    void operator ()(const MAT &M, VECT &x, const VECT &b,
		     gmm::iteration &) const 
    { gmm::MUMPS_solve(MS, x, b); }
  };
#endif

#if GETFEM_PARA_LEVEL > 1 && GETFEM_PARA_SOLVER == MUMPS_PARA_SOLVER
  template <class MAT, class VECT> 
  struct linear_solver_distributed_mumps
    : public abstract_linear_solver<MAT, VECT> {
    void operator ()(const MAT &M, VECT &x, const VECT &b,
		     gmm::iteration &) const { 
      double tt_ref=MPI_Wtime();
      MUMPS_distributed_matrix_solve(M, x, b);
      cout<<"temps MUMPS "<< MPI_Wtime() - tt_ref<<endl;
    }
  };
#endif

  /* ***************************************************************** */
  /*     Newton algorithm.                                             */
  /* ***************************************************************** */

  template <typename PB> 
  void classical_Newton(PB &pb, gmm::iteration &iter,
			const abstract_linear_solver<typename PB::MATRIX,
			typename PB::VECTOR> &linear_solver) {
    // TODO : take iter into account for the Newton. compute a consistent 
    //        max residu.
    typedef typename gmm::linalg_traits<typename PB::VECTOR>::value_type T;
    gmm::iteration iter_linsolv0 = iter;
    iter_linsolv0.reduce_noisy();
    iter_linsolv0.set_resmax(iter.get_resmax()/100.0);

    pb.compute_residual();

    typename PB::VECTOR dr(gmm::vect_size(pb.residual()));
    typename PB::VECTOR b(gmm::vect_size(pb.residual()));

    while (!iter.finished(gmm::vect_norm2(pb.residual()))) {
      gmm::iteration iter_linsolv = iter_linsolv0;
      pb.compute_tangent_matrix();
      gmm::clear(dr);
      gmm::copy(gmm::scaled(pb.residual(), T(-1)), b);
      linear_solver(pb.tangent_matrix(), dr, b, iter_linsolv);
      pb.line_search(dr); // it is assumed that the line search execute
      // a pb.compute_residual();
      ++iter;
    }
  }

  /* ***************************************************************** */
  /*     Intermediary structure for Newton algorithms.                 */
  /* ***************************************************************** */

  template <typename MODEL_STATE> 
  struct model_problem {

    TYPEDEF_MODEL_STATE_TYPES;
    typedef T_MATRIX MATRIX;

    MODEL_STATE &MS;
    mdbrick_abstract<MODEL_STATE> &pb;
    abstract_newton_line_search &ls;
    VECTOR stateinit, d;

    void compute_tangent_matrix(void)
    { pb.compute_tangent_matrix(MS);  MS.compute_reduced_system(); }

    const T_MATRIX &tangent_matrix(void)
    { return MS.reduced_tangent_matrix(); }
    
    void compute_residual(void)
    { pb.compute_residual(MS); MS.compute_reduced_residual(); }

    const VECTOR &residual(void) { return MS.reduced_residual(); }

    void line_search(VECTOR &dr) {
      gmm::resize(d, pb.nb_dof());
      gmm::resize(stateinit, pb.nb_dof());
      gmm::copy(MS.state(), stateinit);
      MS.unreduced_solution(dr, d);
      R alpha, res;
      
      ls.init_search(gmm::vect_norm2(residual()));
      do {
	alpha = ls.next_try();
	gmm::add(stateinit, gmm::scaled(d, alpha), MS.state());
	compute_residual();
	res = MS.reduced_residual_norm();
      } while (!ls.is_converged(res));
      
      if (alpha != ls.converged_value()) {
	alpha = ls.converged_value();
	gmm::add(stateinit, gmm::scaled(d, alpha), MS.state());
	res = ls.converged_residual();
	compute_residual();
      }
    }

    model_problem(MODEL_STATE &MS_, mdbrick_abstract<MODEL_STATE> &pb_,
		  abstract_newton_line_search &ls_)
      : MS(MS_), pb(pb_), ls(ls_) {}

  };

  /* ***************************************************************** */
  /*     Standard solve.                                               */
  /* ***************************************************************** */


  /** A default solver for the model brick system.  
      
  Of course it could be not very well suited for a particular
  problem, so it could be copied and adapted to change solvers,
  add a special traitement on the problem, etc ...  This is in
  fact a model for your own solver.

  For small problems, a direct solver is used
  (getfem::SuperLU_solve), for larger problems, a conjugate
  gradient gmm::cg (if the problem is coercive) or a gmm::gmres is
  used (preconditionned with an incomplete factorization).

  When MPI/METIS is enabled, a partition is done via METIS, and the
  gmm::additive_schwarz is used as the parallel solver.

  @ingroup bricks
  */
  template <typename MODEL_STATE> void
  standard_solve(MODEL_STATE &MS, mdbrick_abstract<MODEL_STATE> &problem,
		 gmm::iteration &iter) {

    TYPEDEF_MODEL_STATE_TYPES;
    basic_newton_line_search ls;
    model_problem<MODEL_STATE> mdpb(MS, problem, ls);

    MS.adapt_sizes(problem); // to be sure it is ok, but should be done before
    
    std::auto_ptr<abstract_linear_solver<T_MATRIX, VECTOR> > lsolver;

#if GETFEM_PARA_LEVEL == 1 && GETFEM_PARA_SOLVER == MUMPS_PARA_SOLVER
    lsolver.reset(new linear_solver_mumps<T_MATRIX, VECTOR>);
#elif GETFEM_PARA_LEVEL > 1 && GETFEM_PARA_SOLVER == MUMPS_PARA_SOLVER
    lsolver.reset(new linear_solver_distributed_mumps<T_MATRIX, VECTOR>);
#else
    size_type ndof = problem.nb_dof(), max3d = 15000, dim = problem.dim();
# ifdef GMM_USES_MUMPS
    max3d = 100000;
# endif
    if ((ndof<200000 && dim<=2) || (ndof<max3d && dim<=3) || (ndof<1000)) {
# ifdef GMM_USES_MUMPS
      lsolver.reset(new linear_solver_mumps<T_MATRIX, VECTOR>);
# else
      lsolver.reset(new linear_solver_superlu<T_MATRIX, VECTOR>);
# endif
    }
    else {
      if (problem.is_coercive()) 
	lsolver.reset
	  (new linear_solver_cg_preconditioned_ildlt<T_MATRIX, VECTOR>);
      else if (problem.mixed_variables().card() != 0)
	lsolver.reset
	  (new linear_solver_gmres_preconditioned_ilu<T_MATRIX, VECTOR>);
      else 
	lsolver.reset
	  (new linear_solver_gmres_preconditioned_ilut<T_MATRIX, VECTOR>);
    }
#endif

    if (problem.is_linear()) {
      mdpb.compute_tangent_matrix();
      mdpb.compute_residual();
      VECTOR dr(gmm::vect_size(mdpb.residual())), d(problem.nb_dof());
      VECTOR b(gmm::vect_size(dr));
      gmm::copy(gmm::scaled(mdpb.residual(), value_type(-1)), b);
      (*lsolver)(mdpb.tangent_matrix(), dr, b, iter);
      MS.unreduced_solution(dr, d);
      gmm::add(d, MS.state());
    }
    else
      classical_Newton(mdpb, iter, *lsolver);
  }




















#if 0 // a garder pour Schwarz additif


  template <typename MODEL_STATE> void
  standard_solve(MODEL_STATE &MS, mdbrick_abstract<MODEL_STATE> &problem,
		 gmm::iteration &iter) {

    TYPEDEF_MODEL_STATE_TYPES;

    // TODO : take iter into account for the Newton. compute a consistent 
    //        max residu.

    size_type ndof = problem.nb_dof();

    bool is_linear = problem.is_linear();
    std::auto_ptr<abstract_newton_line_search>
      pline_search(new basic_newton_line_search);

    R alpha(0);
    VECTOR stateinit;

    dal::bit_vector mixvar;
    gmm::iteration iter_linsolv0 = iter;
    iter_linsolv0.set_maxiter(10000);
    if (!is_linear) { 
      iter_linsolv0.reduce_noisy();
      iter_linsolv0.set_resmax(iter.get_resmax()/100.0);
    }


#if GETFEM_PARA_LEVEL > 1
    double t_init = MPI_Wtime();
#endif
    MS.adapt_sizes(problem); // to be sure it is ok, but should be done before
    problem.compute_residual(MS);
#if GETFEM_PARA_LEVEL > 1
    cout << "comput  residual  time = " << MPI_Wtime() - t_init << endl;
    t_init = MPI_Wtime();
#endif
    problem.compute_tangent_matrix(MS);
#if GETFEM_PARA_LEVEL > 1
    cout << "comput tangent time = " << MPI_Wtime() - t_init << endl;
    // cout << "CM = " << MS.constraints_matrix() << endl;
    // cout << "TM = " << MS.tangent_matrix() << endl;
    t_init = MPI_Wtime();
#endif
    MS.compute_reduced_system();

#if GETFEM_PARA_LEVEL > 1
    cout << "comput reduced system time = " << MPI_Wtime() - t_init << endl;
#endif

#if GETFEM_PARA_LEVEL > 1 && GETFEM_PARA_SOLVER == SCHWARZADD_PARA_SOLVER
    /* use a domain partition ? */
    double t_ref = MPI_Wtime();
    std::set<const mesh *> mesh_set;
    for (size_type i = 0; i < problem.nb_mesh_fems(); ++i) {
      mesh_set.insert(&(problem.get_mesh_fem(i).linked_mesh()));
    }

    cout << "You have " << mesh_set.size() << " meshes\n";

    std::vector< std::vector<int> > eparts(mesh_set.size());
    size_type nset = 0;
    int nparts = 64;//(ndof / 1000)+1; // number of sub-domains.

    // Quand il y a plusieurs maillages, on d�coupe tous les maillages en
    // autant de parties
    // et on regroupe les ddl de chaque partition par numéro de sous-partie.
    for (std::set<const mesh *>::iterator it = mesh_set.begin();
	 it != mesh_set.end(); ++it, ++nset) {
      int ne = int((*it)->nb_convex());
      int nn = int((*it)->nb_points()), k = 0, etype = 0, numflag = 0;
      int edgecut;
      
      bgeot::pconvex_structure cvs
	= (*it)->structure_of_convex((*it)->convex_index().first())->basic_structure();
      
      if (cvs == bgeot::simplex_structure(2)) { k = 3; etype = 1; }
      else if (cvs == bgeot::simplex_structure(3)) { k = 4; etype = 2; }
      else if (cvs == bgeot::parallelepiped_structure(2)) { k = 4; etype = 4; }
      else if (cvs == bgeot::parallelepiped_structure(3)) { k = 8; etype = 3; }
      else DAL_THROW(failure_error,
		     "This kind of element is not taken into account");

      
      std::vector<int> elmnts(ne*k), npart(nn);
      eparts[nset].resize(ne);
      int j = 0;
      // should be adapted for high order geotrans
      for (dal::bv_visitor i((*it)->convex_index()); !i.finished(); ++i, ++j)
	for (int l = 0; l < k; ++l)
	  elmnts[j*k+l] = (*it)->ind_points_of_convex(i)[l];

      METIS_PartMeshNodal(&ne, &nn, &(elmnts[0]), &etype, &numflag, &nparts,
			  &edgecut, &(eparts[nset][0]), &(npart[0]));
    }

    std::vector<dal::bit_vector> Bidof(nparts);
    size_type apos = 0;
    for (size_type i = 0; i < problem.nb_mesh_fems(); ++i) {
      const mesh_fem &mf = problem.get_mesh_fem(i);
      nset = 0;
      for (std::set<const mesh *>::iterator it = mesh_set.begin();
	   it != mesh_set.end(); ++it, ++nset)
	if (*it == &(mf.linked_mesh())) break; 
      size_type pos = problem.get_mesh_fem_position(i);
      if (pos != apos)
	DAL_THROW(failure_error, "Multiplicators are not taken into account");
      size_type length = mf.nb_dof();
      apos += length;
      for (dal::bv_visitor j(mf.convex_index()); !j.finished(); ++j) {
	size_type k = eparts[nset][j];
	for (size_type l = 0; l < mf.nb_dof_of_element(i); ++l)
	  Bidof[k].add(mf.ind_dof_of_element(j)[l] + pos);
      }
      if (apos != ndof)
	DAL_THROW(failure_error, "Multiplicators are not taken into account");
    }

    std::vector< gmm::row_matrix< gmm::rsvector<value_type> > > Bi(nparts);    
    for (size_type i = 0; i < size_type(nparts); ++i) {
      gmm::resize(Bi[i], ndof, Bidof[i].card());
      size_type k = 0;
      for (dal::bv_visitor j(Bidof[i]); !j.finished(); ++j, ++k)
	Bi[i](j, k) = value_type(1);
    }

    std::vector< gmm::row_matrix< gmm::rsvector<value_type> > > Bib(nparts);
    gmm::col_matrix< gmm::rsvector<value_type> > Bitemp;
    if (problem.nb_constraints() > 0) {
      for (size_type i = 0; i < size_type(nparts); ++i) {
	gmm::resize(Bib[i], gmm::mat_ncols(MS.nullspace_matrix()),
		    gmm::mat_ncols(Bi[i]));
       	gmm::mult(gmm::transposed(MS.nullspace_matrix()), Bi[i], Bib[i]);
	
	gmm::resize(Bitemp, gmm::mat_nrows(Bib[i]), gmm::mat_ncols(Bib[i]));
	gmm::copy(Bib[i], Bitemp);
	R EPS = gmm::mat_norminf(Bitemp) * gmm::default_tol(R());
	size_type k = 0;
	for (size_type j = 0; j < gmm::mat_ncols(Bitemp); ++j, ++k) {
	  // should do Schmidt orthogonalization for most sofisticated cases 
	  if (k != j) Bitemp.swap_col(j, k);
	  if (gmm::vect_norm2(gmm::mat_col(Bitemp, k)) < EPS) --k;
	}
	gmm::resize(Bitemp, gmm::mat_nrows(Bib[i]), k);
	gmm::resize(Bib[i], gmm::mat_nrows(Bib[i]), k);
	gmm::copy(Bitemp, Bib[i]);
	// cout << "Bib[" << i << "] = " <<  Bib[i] << endl;
	// cout << "Bi[" << i << "] = " <<  Bi[i] << endl;
      }
    } else std::swap(Bi, Bib);    
    cout << "METIS time = " << MPI_Wtime() - t_ref << endl;
#endif

    // cout << "RTM = " << MS.reduced_tangent_matrix() << endl;
    
    
    R act_res = MS.reduced_residual_norm(), act_res_new(0);

    while (is_linear || !iter.finished(act_res)) {
      
      size_type nreddof = gmm::vect_size(MS.reduced_residual());
      gmm::iteration iter_linsolv = iter_linsolv0;
      VECTOR d(ndof), dr(nreddof);
      
      if (!(iter.first())) {
	problem.compute_tangent_matrix(MS);
	MS.compute_reduced_system();
#if GETFEM_PARA_LEVEL > 1
        DAL_THROW(failure_error, "oups ...");
#endif
      }
      
      //       if (iter.get_noisy())
      // cout << "tangent matrix " << MS.tangent_matrix() << endl;
      //      cout << "tangent matrix is "
      // 	   << (gmm::is_symmetric(MS.tangent_matrix(),
      //          1E-6 * gmm::mat_maxnorm(MS.tangent_matrix())) ? "" : "not ")
      // 	   <<  "symmetric. ";

      //      cout << "MM = " << MS.reduced_tangent_matrix() << endl;

//       gmm::dense_matrix<value_type> MM(nreddof,nreddof), Q(nreddof,nreddof);
//       std::vector<value_type> eigval(nreddof);
//       gmm::copy(MS.reduced_tangent_matrix(), MM);
//       // gmm::symmetric_qr_algorithm(MM, eigval, Q);
//       gmm::implicit_qr_algorithm(MM, eigval, Q);
//       std::sort(eigval.begin(), eigval.end(), sort_abs_val_<value_type>());
//       cout << "eival = " << eigval << endl;
//       cout << "vectp : " << gmm::mat_col(Q, nreddof-1) << endl;
//       cout << "vectp : " << gmm::mat_col(Q, nreddof-2) << endl;
//       double emax, emin;
//       cout << "condition number" << condition_number(MM,emax,emin) << endl;
      //       cout << "emin = " << emin << endl;
      //       cout << "emax = " << emax << endl;

      if (iter.get_noisy()) {
	cout << "there are " << gmm::mat_nrows(MS.constraints_matrix())
	     << " constraints\n";
	mixvar = problem.mixed_variables();
	cout << "there are " << mixvar.card() << " mixed variables\n";
      }
      size_type dim = problem.dim();

#if (GETFEM_PARA_LEVEL > 1) && (GETFEM_PARA_SOLVER == SCHWARZADD_PARA_SOLVER)
      double t_ref,t_final;
      t_ref=MPI_Wtime();
      cout<<"begin Seq AS"<<endl; // � remmetre en fonction : utilisation
      // de mpi_distributed_matrix a revoir : mettre une r�f�rence.
      additive_schwarz(gmm::mpi_distributed_matrix<..>(MS.reduced_tangent_matrix()), dr,
		       gmm::scaled(MS.reduced_residual(), value_type(-1)),
		       gmm::identity_matrix(), Bib, iter_linsolv, gmm::using_superlu(),
		       gmm::using_cg());
      t_final=MPI_Wtime();
      cout<<"temps Seq AS "<< t_final-t_ref<<endl;
#elif GETFEM_PARA_LEVEL == 1 && GETFEM_PARA_SOLVER == MUMPS_PARA_SOLVER
      MUMPS_solve(MS.reduced_tangent_matrix(), dr,
		  gmm::scaled(MS.reduced_residual(), value_type(-1)));
#elif GETFEM_PARA_LEVEL > 1 && GETFEM_PARA_SOLVER == MUMPS_PARA_SOLVER
      double tt_ref=MPI_Wtime();
      MUMPS_distributed_matrix_solve(MS.reduced_tangent_matrix(), dr,
				     gmm::scaled(MS.reduced_residual(),
						 value_type(-1)));
      cout<<"temps MUMPS "<< MPI_Wtime() - tt_ref<<endl;
#else
      // if (0) {
      if (
#ifdef GMM_USES_MUMPS
	  (ndof < 200000 && dim <= 2) || (ndof < 100000 && dim <= 3)
	  || (ndof < 1000)
#else  
	  (ndof < 200000 && dim <= 2) || (ndof < 15000 && dim <= 3)
	  || (ndof < 1000)
#endif
	  ) {
	
	// cout << "M = " << MS.reduced_tangent_matrix() << endl;
	// cout << "L = " << MS.reduced_residual() << endl;
	
	double t = dal::uclock_sec();
#ifdef GMM_USES_MUMPS
	DAL_TRACE2("Solving with MUMPS\n");
	MUMPS_solve(MS.reduced_tangent_matrix(), dr,
		    gmm::scaled(MS.reduced_residual(), value_type(-1)));
#else
	double rcond;
	SuperLU_solve(MS.reduced_tangent_matrix(), dr,
		      gmm::scaled(MS.reduced_residual(), value_type(-1)),
		      rcond);
	if (iter.get_noisy()) cout << "condition number: " << 1.0/rcond<< endl;
#endif
	cout << "resolution time = " << dal::uclock_sec() - t << endl;
      }
      else {
	if (problem.is_coercive()) {
	  gmm::ildlt_precond<T_MATRIX> P(MS.reduced_tangent_matrix());
	  gmm::cg(MS.reduced_tangent_matrix(), dr, 
		  gmm::scaled(MS.reduced_residual(), value_type(-1)),
		  P, iter_linsolv);
	  if (!iter_linsolv.converged()) DAL_WARNING2("cg did not converge!");
	} else {
	  if (mixvar.card() == 0) {
	    gmm::ilu_precond<T_MATRIX> P(MS.reduced_tangent_matrix());
	  
	    gmm::gmres(MS.reduced_tangent_matrix(), dr, 
		       gmm::scaled(MS.reduced_residual(),  value_type(-1)), P,
		       300, iter_linsolv);
	  }
	  else {
	    gmm::ilut_precond<T_MATRIX> P(MS.reduced_tangent_matrix(),
					  20, 1E-10);
	    // gmm::identity_matrix P;
	    gmm::gmres(MS.reduced_tangent_matrix(), dr, 
		       gmm::scaled(MS.reduced_residual(),  value_type(-1)),
		       P, 300, iter_linsolv);
	  }
	  if (!iter_linsolv.converged())
	    DAL_WARNING2("gmres did not converge!");
	}
      } 
#endif // GETFEM_PARA_LEVEL < 2

      MS.unreduced_solution(dr,d);
    
      if (is_linear) {
	gmm::add(d, MS.state()); return;
      }
      else { // line search for the non-linear case.
	gmm::resize(stateinit, ndof);
	gmm::copy(MS.state(), stateinit);
      
	pline_search->init_search(act_res);
	do {
	  alpha = pline_search->next_try();
	  gmm::add(stateinit, gmm::scaled(d, alpha), MS.state());
	  problem.compute_residual(MS);
	  MS.compute_reduced_residual();
	  act_res_new = MS.reduced_residual_norm();
	} while (!pline_search->is_converged(act_res_new));

	if (alpha != pline_search->converged_value()) {
	  alpha = pline_search->converged_value();
	  gmm::add(stateinit, gmm::scaled(d, alpha), MS.state());
	  act_res_new = pline_search->converged_residual();
	}
      }
      act_res = act_res_new; ++iter;
    
      if (iter.get_noisy()) cout << "alpha = " << alpha << " ";
    }
    
  }


#endif













}  /* end of namespace getfem.                                             */


#endif /* GETFEM_MODEL_SOLVERS_H__  */
