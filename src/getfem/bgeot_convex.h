/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================

 Copyright (C) 1999-2017 Yves Renard

 This file is a part of GetFEM++

 GetFEM++  is  free software;  you  can  redistribute  it  and/or modify it
 under  the  terms  of the  GNU  Lesser General Public License as published
 by  the  Free Software Foundation;  either version 3 of the License,  or
 (at your option) any later version along with the GCC Runtime Library
 Exception either version 3.1 or (at your option) any later version.
 This program  is  distributed  in  the  hope  that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License and GCC Runtime Library Exception for more details.
 You  should  have received a copy of the GNU Lesser General Public License
 along  with  this program;  if not, write to the Free Software Foundation,
 Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

 As a special exception, you  may use  this file  as it is a part of a free
 software  library  without  restriction.  Specifically,  if   other  files
 instantiate  templates  or  use macros or inline functions from this file,
 or  you compile this  file  and  link  it  with other files  to produce an
 executable, this file  does  not  by itself cause the resulting executable
 to be covered  by the GNU Lesser General Public License.  This   exception
 does not  however  invalidate  any  other  reasons why the executable file
 might be covered by the GNU Lesser General Public License.

===========================================================================*/

/**@file bgeot_convex.h
   @author  Yves Renard <Yves.Renard@insa-lyon.fr>
   @date December 20, 1999.
   @brief Convex objects (structure + vertices)
*/

#ifndef BGEOT_CONVEX_H__
#define BGEOT_CONVEX_H__

#include "bgeot_convex_structure.h"

namespace bgeot {

  /** @defgroup convexes Convexes */
  /** @addtogroup convexes */
  /*@{*/

  /// generic definition of a convex ( bgeot::convex_structure + vertices coordinates )
  template<class PT, class PT_TAB = std::vector<PT> > class convex {
  public :
    
    typedef PT point_type;
    typedef PT_TAB point_tab_type;
    typedef typename PT_TAB::size_type size_type;
    
    typedef gmm::tab_ref_index_ref< typename PT_TAB::const_iterator,
      convex_ind_ct::const_iterator> ref_convex_pt_ct;
    
    typedef gmm::tab_ref_index_ref< typename PT_TAB::const_iterator,
      ref_convex_ind_ct::const_iterator> dref_convex_pt_ct;
    
  protected :
    
    pconvex_structure cvs;
    PT_TAB pts;
    
  public :
    
    ref_convex_pt_ct points_of_face(short_type i) const {
      return ref_convex_pt_ct(pts.begin(), cvs->ind_points_of_face(i).begin(),
                              cvs->ind_points_of_face(i).end());
    }
    
    /** Return "direct" points. These are the subset of points than can be
        used to build a direct vector basis. (rarely used)
    */
    ref_convex_pt_ct dir_points() const {
      return ref_convex_pt_ct(pts.begin(), cvs->ind_dir_points().begin(),
                              cvs->ind_dir_points().end());
    }
    /** Direct points for a given face.
        @param i the face number.
    */
    dref_convex_pt_ct dir_points_of_face(short_type i) const {
      return dref_convex_pt_ct(pts.begin(),
                               cvs->ind_dir_points_of_face(i).begin(),
                               cvs->ind_dir_points_of_face(i).end());
    }
    pconvex_structure structure() const { return cvs; }
    pconvex_structure &structure() { return cvs; }
    const PT_TAB &points() const { return pts; }
    PT_TAB &points() { return pts; }
    short_type nb_points() const { return cvs->nb_points(); }
    
    //void translate(const typename PT::vector_type &v);
    //template <class CONT> void base_of_orthogonal(CONT &tab);
    convex() { }
    /** Build a convex object.
        @param c the convex structure.
        @param t the points array.
    */
    convex(pconvex_structure c, const PT_TAB &t) : cvs(c), pts(t) {}
    convex(pconvex_structure c) : cvs(c) {}
  };
  /*@}*/
  /*template<class PT, class PT_TAB>
    void convex<PT, PT_TAB>::translate(const typename PT::vector_type &v) {
    typename PT_TAB::iterator b = pts.begin(), e = pts.end();
    for ( ; b != e ; ++b) *b += v;
  }
  */
  /*  
  template<class PT, class PT_TAB> template<class CONT>
    void convex<PT, PT_TAB>::base_of_orthogonal(CONT &tab)
  { // programmation a revoir.
    int N = (points())[0].size();
    pconvex_structure cv = structure();
    int n = cv->dim();
    dal::dynamic_array<typename PT::vector_type> vect_;
    vsvector<double> A(N), B(N);
    ref_convex_ind_ct dptf = cv->ind_dir_points_of_face(f);
    int can_b = 0;
    
    for (int i = 0; i < n-1; i++) {
      vect_[i]  = (points())[dptf[i+1]]; vect_[i] -= (points())[dptf[0]];
      
      for (j = 0; j < i; j++)
        A[j] = vect_sp(vect_[i], vect_[j]);
      for (j = 0; j < i; j++)
        { B = vect_[j]; B *= A[j]; vect_[i] -= B; }
      vect_[i] /= vect_norm2(vect_[i]);
    }
    
    for (int i = n; i < N; i++) {
      vect_[i] = vect_[0];
      vect_random(vect_[i]);
      for (j = 0; j < i; j++)
        A[j] = vect_sp(vect_[i], vect_[j]);
      for (j = 0; j < i; j++)
        { B = vect_[j]; B *= A[j]; vect_[i] -= B; }
      
      if (vect_norm2(vect_[i]) < 1.0E-4 )
        i--;
      else
        vect_[i] /= vect_norm2(vect_[i]);
    }
    for (int i = n; i < N; i++) tab[i-n] = vect_[i];
  }
  */

  template<class PT, class PT_TAB>
    std::ostream &operator <<(std::ostream &o, const convex<PT, PT_TAB> &cv)
  {
    o << *(cv.structure());
    o << " points : ";
    for (size_type i = 0; i < cv.nb_points(); ++i) o << cv.points()[i] << " ";
    o << endl;
    return o;
  }

  /* ********************************************************************** */
  /* Unstabilized part.                                                     */
  /* ********************************************************************** */

  template<class PT, class PT_TAB>
    convex<PT, PT_TAB> simplex(const PT_TAB &t, int nc)
  { return convex<PT, PT_TAB>(simplex_structure(nc), t); }


  template<class PT, class PT_TAB1, class PT_TAB2>
    convex<PT> convex_product(const convex<PT, PT_TAB1> &cv1,
                              const convex<PT, PT_TAB2> &cv2)
  { // optimisable
    typename convex<PT>::point_tab_type tab;
    tab.resize(cv1.nb_points() * cv2.nb_points());
    size_type i,j,k;
    for (i = 0, k = 0; i < cv1.nb_points(); ++i)
      for (j = 0; j < cv2.nb_points(); ++j, ++k)
        { tab[k] = (cv1.points())[i]; tab[k] += (cv2.points())[j]; }
    return convex<PT>(
             convex_product_structure(cv1.structure(), cv2.structure()), tab);
  }

  struct special_convex_structure_key_ : virtual public dal::static_stored_object_key {
    pconvex_structure p;
    bool compare(const static_stored_object_key &oo) const override {
      auto &o = dynamic_cast<const special_convex_structure_key_ &>(oo);
      return p < o.p;
    }
    bool equal(const static_stored_object_key &oo) const override {
      auto &o = dynamic_cast<const special_convex_structure_key_ &>(oo);
      if (p == o.p) return true;

      auto pkey = dal::key_of_stored_object(p);
      auto poo_key = dal::key_of_stored_object(o.p);
      return *pkey == *poo_key;
    }
    special_convex_structure_key_(pconvex_structure pp) : p(pp) {}
  };

  template<class PT, class PT_TAB1, class PT_TAB2>
    convex<PT> convex_direct_product(const convex<PT, PT_TAB1> &cv1,
                                     const convex<PT, PT_TAB2> &cv2) {
    if (cv1.nb_points() == 0 || cv2.nb_points() == 0)
      throw std::invalid_argument(
                     "convex_direct_product : null convex product");
    
    if (!dal::exists_stored_object(cv1.structure())) {
      dal::pstatic_stored_object_key
        pcs = std::make_shared<special_convex_structure_key_>(cv1.structure());
      dal::add_stored_object(pcs, cv1.structure(),
                             dal::AUTODELETE_STATIC_OBJECT);
    }
    if (!dal::exists_stored_object(cv2.structure())) {
      dal::pstatic_stored_object_key
        pcs = std::make_shared<special_convex_structure_key_>(cv2.structure());
      dal::add_stored_object(pcs, cv2.structure(),
                             dal::AUTODELETE_STATIC_OBJECT);
    }
    convex<PT> r(convex_product_structure(cv1.structure(), cv2.structure()));
    r.points().resize(r.nb_points());
    std::fill(r.points().begin(), r.points().end(), PT(r.structure()->dim()));
    dim_type dim1 = cv1.structure()->dim();
    typename PT_TAB1::const_iterator it1, it1e = cv1.points().end();
    typename PT_TAB2::const_iterator it2, it2e = cv2.points().end();
    typename convex<PT>::point_tab_type::iterator it = r.points().begin();
    for (it2 = cv2.points().begin(); it2 != it2e; ++it2)
      for (it1 = cv1.points().begin() ; it1 != it1e; ++it1, ++it)
      {
        std::copy((*it1).begin(), (*it1).end(), (*it).begin());
        std::copy((*it2).begin(), (*it2).end(), (*it).begin()+dim1);
      }
    return r;
  }

  template<class PT, class PT_TAB>
    convex<PT> convex_multiply(const convex<PT, PT_TAB> &cv, dim_type n)
  {
    if (cv.nb_points() == 0 || n == 0)
      throw std::invalid_argument(
                     "convex_multiply : null convex product");
    convex<PT> r(multiply_convex_structure(cv.structure(), n));
    r.points().resize(r.nb_points());
    std::fill(r.points().begin(), r.points().end(), PT(r.structure()->dim()));
    dim_type dim1 = cv.structure()->dim();
    typename convex<PT>::point_tab_type::iterator it = r.points().begin();
    typename PT_TAB::const_iterator it1  = cv.points().begin(), it2,
                                    it1e = cv.points().end();
    for (dim_type k = 0; k < n; ++k)
      for (it2 = it1; it2 != it1e; ++it2) *it++ = *it2;
    return r;
  }

}  /* end of namespace bgeot.                                             */


#endif /* BGEOT_CONVEX_H__                                                */
