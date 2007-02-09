// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2002-2006 Julien Pommier
//
// This file is a part of GETFEM++
//
// Getfem++ is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,
// USA.
//
//========================================================================

#ifndef GETFEMINT_OBJECT_H__
#define GETFEMINT_OBJECT_H__

#include <getfemint_std.h>

namespace getfemint
{

  /* common class for all getfem objects addressable from the interface */

  enum { STATIC_OBJ = 1, CONST_OBJ = 2 };

  class getfem_object {
    friend class workspace_data;
    friend class workspace_stack;
  public:
    typedef const void* internal_key_type;
  protected:
    //  public:
    id_type workspace;
    id_type id;
    std::vector<id_type> used_by; /* list of objects which depends on this object */
    internal_key_type ikey; /* generally the pointer to the corresponding 
			       getfem object */

    static const id_type anonymous_workspace = id_type(-1);

    typedef int obj_flags_t;
    obj_flags_t flags; /* if STATIC_OBJ, the linked getfem object is not
			  deleted when the getfem_object is destroyed
			  (example: pfem, mesh_fems obtained with
			  classical_mesh_fem(..) etc. */
    
    /* fonctions reserv�es au workspace */
    void set_workspace(id_type w) { workspace = w; }
    void set_id(id_type i) { id = i; }

  public:

    getfem_object() : ikey(0) { workspace = 0; id = 0; flags = 0; }

    /* these functions can't be pure virtual functions !?
       it breaks the linking of getfem with libgetfemint.so
    */
    virtual ~getfem_object() { id = id_type(-1); ikey = 0; workspace = id = 0x77777777;}
    virtual id_type class_id() const = 0;

    virtual size_type memsize() const { return 0; }

    /* 
       the clear function is called before deletion 
       the object should prepare itself to be deleted
       after or before any other object it uses.
       (for ex. the mesh_fem object may be destroyed
       before or after its linked_mesh)
       
       be careful not do destroy objects which are marked static!
    */
    virtual void clear_before_deletion() {};
   
    /*
      mark the object as a static object (infinite lifetime)
    */
    void set_flags(int v) { flags = v; workspace = anonymous_workspace; }
    bool is_static() const { return flags & STATIC_OBJ; }
    bool is_const()  const { return flags & CONST_OBJ; }

    id_type get_workspace() const { return workspace; }
    id_type get_id() const { return id; }
    bool is_anonymous() const { return workspace == anonymous_workspace; }
    const std::vector<id_type>& get_used_by() const { return used_by; }
  };
}
#endif
