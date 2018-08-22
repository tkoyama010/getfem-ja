/*===========================================================================

 Copyright (C) 2002-2017 Yves Renard

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

===========================================================================*/


#include "getfem/dal_static_stored_objects.h"
#include "getfem/dal_singleton.h"
#include <map>
#include <list>
#include <set>
#include <algorithm>
#include <deque>


namespace dal {

  // 0 = only undestroyed, 1 =  Normal, 2 very noisy, 
#define DAL_STORED_OBJECT_DEBUG_NOISY 2

static bool dal_static_stored_tab_valid__ = true;
  
#if DAL_STORED_OBJECT_DEBUG
  static std::map <const static_stored_object *, std::string> _created_objects;
  static std::map <const static_stored_object *, std::string> _added_objects;
  static std::map <const static_stored_object *, std::string> _deleted_objects;
 

  void stored_debug_created(const static_stored_object *o,
			    const std::string &name) {
    if (dal_static_stored_tab_valid__) {
      _created_objects[o] = name;
#   if DAL_STORED_OBJECT_DEBUG_NOISY > 1
      cout << "Created " << name << " : " << o << endl;
#   endif
    }
  }
  void stored_debug_added(const static_stored_object *o) {
    if (dal_static_stored_tab_valid__) {
      auto it = _created_objects.find(o);
      if (it == _created_objects.end()) {
	_added_objects[o] = "";
#   if DAL_STORED_OBJECT_DEBUG_NOISY > 0
	cout << "Adding an unknown object " << o << " of type "
	     << typeid(*o).name() << " add DAL_STORED_OBJECT_DEBUG_CREATED"
	  "(o, name)  in its constructor" << endl;
#   endif
      } else {
	_added_objects[o] = it->second;
	_created_objects.erase(it);
#   if DAL_STORED_OBJECT_DEBUG_NOISY > 1
	cout << "Added " << it->second << " : " << o << endl;
#   endif
      }
      if (_deleted_objects.size()) {
	cout << endl << "Number of stored objects: " << _added_objects.size()
	     << endl << "Number of unstored created objects: "
	     << _created_objects.size() << endl
	     << "Number of undestroyed object: "
	     << _deleted_objects.size() << endl;
	for (auto &x : _deleted_objects)
	  cout << "UNDESTROYED OBJECT " << x.second << " : " << x.first << endl;
      }
    }
  }

  void stored_debug_deleted(const static_stored_object *o) {
    if (dal_static_stored_tab_valid__) {
      auto it = _added_objects.find(o);
      if (it == _added_objects.end()) {
	cout << "Deleting an unknown object ! " << o << endl;
	_deleted_objects[o] = "";
      } else {
	_deleted_objects[o] = it->second;
	_added_objects.erase(it);
#   if DAL_STORED_OBJECT_DEBUG_NOISY > 1
	cout << "Deleted " << it->second << " : " << o << endl;
#   endif
      }
    }
  }

  void stored_debug_destroyed(const static_stored_object *o,
			      const std::string &name) {
    if (dal_static_stored_tab_valid__) {
      auto it = _deleted_objects.find(o);
      if (it == _deleted_objects.end()) {
	it = _created_objects.find(o);
	if (it == _created_objects.end()) {
	  it = _added_objects.find(o);
	  if (it == _added_objects.end()) {
	    cout << "Destroy an unknown object ! " << o << " name given : "
		 << name << endl;
	  } else {
	    _added_objects.erase(it);
	    cout << "Destroy a non deleted object !! " << o << " name given : "
		 << name << endl;
	  }
	} else {
#   if DAL_STORED_OBJECT_DEBUG_NOISY > 1
	  cout << "Destroy an unadded object " << it->second << " : "
	       << o << endl;
#   endif
	  _created_objects.erase(it);
	}
      } else {
#   if DAL_STORED_OBJECT_DEBUG_NOISY > 1
	cout << "Destroy " << it->second << " : " << o << " name given : "
	     << name << endl;
#   endif
	_deleted_objects.erase(it);
      }
      
    }
  }




#endif




  // Gives a pointer to a key of an object from its pointer, while looking in the storage of
  // a specific thread
  pstatic_stored_object_key key_of_stored_object(pstatic_stored_object o, size_t thread) 
  {
    stored_object_tab::stored_key_tab& stored_keys 
      = dal::singleton<stored_object_tab>::instance(thread).stored_keys_;
    GMM_ASSERT1(dal_static_stored_tab_valid__, "Too late to do that");
    stored_object_tab::stored_key_tab::iterator it = stored_keys.find(o);
    if (it != stored_keys.end()) return it->second;
    return 0;
  }

  // gives a key of the stored object while looking in the storage of other threads
  pstatic_stored_object_key key_of_stored_object_other_threads(pstatic_stored_object o) 
  {
    for(size_t thread = 0; thread<getfem::num_threads();thread++)
    {
      if (thread == this_thread()) continue;
      pstatic_stored_object_key key = key_of_stored_object(o,thread);
      if (key) return key;
    }
    return 0;
  }

  pstatic_stored_object_key key_of_stored_object(pstatic_stored_object o) 
  {
    pstatic_stored_object_key key = key_of_stored_object(o,this_thread());
    if (key) return key;
    else return (num_threads() > 1) ? key_of_stored_object_other_threads(o) : 0;
    return 0;
  }

  bool exists_stored_object(pstatic_stored_object o) 
  {
    stored_object_tab::stored_key_tab& stored_keys 
      = dal::singleton<stored_object_tab>::instance().stored_keys_;
    if (dal_static_stored_tab_valid__) {
      return  (stored_keys.find(o) != stored_keys.end());
    }
    return false;
  }

  pstatic_stored_object search_stored_object(pstatic_stored_object_key k) 
  {
    stored_object_tab& stored_objects
        = dal::singleton<stored_object_tab>::instance();
    if (dal_static_stored_tab_valid__) {
      pstatic_stored_object p = stored_objects.search_stored_object(k);
      if (p) return p;
    }
    return 0;
  }

  pstatic_stored_object search_stored_object_on_all_threads(pstatic_stored_object_key k)
  {
    auto& stored_objects = singleton<stored_object_tab>::instance();
    if (!dal_static_stored_tab_valid__) return nullptr;
    auto p = stored_objects.search_stored_object(k);
    if (p) return p;
    if (num_threads()  == 1) return nullptr;
    for(size_t thread = 0; thread < getfem::num_threads(); thread++)
    {
      if (thread == this_thread()) continue;
      auto& other_objects = singleton<stored_object_tab>::instance(thread);
      p = other_objects.search_stored_object(k);
      if (p) return p;
    }
    return nullptr;
   }

  std::pair<stored_object_tab::iterator, stored_object_tab::iterator> iterators_of_object(
    pstatic_stored_object o)
  {
    for(size_t thread=0; thread < num_threads(); ++thread)
    {
      stored_object_tab& stored_objects
        = dal::singleton<stored_object_tab>::instance(thread);
      if (!dal_static_stored_tab_valid__) continue;
      stored_object_tab::iterator it = stored_objects.iterator_of_object_(o);
      if (it != stored_objects.end()) return {it, stored_objects.end()};
    }
    return {dal::singleton<stored_object_tab>::instance().end(),
            dal::singleton<stored_object_tab>::instance().end()};
  }


  void test_stored_objects(void) 
  {
    for(size_t thread = 0; thread < num_threads(); ++thread)
    {
      stored_object_tab& stored_objects 
        = dal::singleton<stored_object_tab>::instance(thread);
      if (!dal_static_stored_tab_valid__) continue;
      stored_object_tab::stored_key_tab& stored_keys = stored_objects.stored_keys_;

      GMM_ASSERT1(stored_objects.size() == stored_keys.size(), 
        "keys and objects tables don't match");
      for (stored_object_tab::stored_key_tab::iterator it = stored_keys.begin();
        it != stored_keys.end(); ++it)
      {
        auto itos = iterators_of_object(it->first);
        GMM_ASSERT1(itos.first != itos.second, "Key without object is found");
      }
      for (stored_object_tab::iterator it = stored_objects.begin();
        it != stored_objects.end(); ++it)
      {
        auto itos = iterators_of_object(it->second.p);
        GMM_ASSERT1(itos.first != itos.second, "Object has key but cannot be found");
      }
    }
  }

  void add_dependency(pstatic_stored_object o1,
                      pstatic_stored_object o2) {
    bool dep_added = false;
    for(size_t thread=0; thread < num_threads(); ++thread)
    {
      stored_object_tab& stored_objects
          = dal::singleton<stored_object_tab>::instance(thread);
      if (!dal_static_stored_tab_valid__) return;
      if ((dep_added = stored_objects.add_dependency_(o1,o2))) break;
    }
    GMM_ASSERT1(dep_added, "Failed to add dependency between " << o1
		<< " of type " << typeid(*o1).name() << " and " << o2
		<< " of type "  << typeid(*o2).name() << ". ");

    bool dependent_added = false;
    for(size_t thread=0; thread < num_threads(); ++thread)
    {
      stored_object_tab& stored_objects
          = dal::singleton<stored_object_tab>::instance(thread);
      if ((dependent_added = stored_objects.add_dependent_(o1,o2))) break;
    }
    GMM_ASSERT1(dependent_added, "Failed to add dependent between " << o1
		<< " of type " << typeid(*o1).name() << " and " << o2
		<< " of type "  << typeid(*o2).name() << ". ");
  }



  /*remove a dependency (from storages of all threads). 
  Return true if o2 has no more dependent object. */
  bool del_dependency(pstatic_stored_object o1,
    pstatic_stored_object o2) 
  {
    bool dep_deleted = false;
    for(size_t thread=0; thread < num_threads(); ++thread)
    {
      stored_object_tab& stored_objects
          = dal::singleton<stored_object_tab>::instance(thread);
      if (!dal_static_stored_tab_valid__) return false;
      if ((dep_deleted = stored_objects.del_dependency_(o1,o2))) break;
    }
    GMM_ASSERT1(dep_deleted, "Failed to delete dependency between " << o1 << " of type "
    << typeid(*o1).name() << " and " << o2 << " of type "  << typeid(*o2).name() << ". ");

    bool dependent_deleted = false;
    bool dependent_empty = false;
    for(size_t thread=0; thread < num_threads(); ++thread)
    {
      stored_object_tab& stored_objects
          = dal::singleton<stored_object_tab>::instance(thread);
      dependent_deleted = stored_objects.del_dependent_(o1,o2);
      if (dependent_deleted)
      {
        dependent_empty = stored_objects.has_dependent_objects(o2);
        break;
      }
    }
    GMM_ASSERT1(dependent_deleted, "Failed to delete dependent between " << o1 << " of type "
    << typeid(*o1).name() << " and " << o2 << " of type "  << typeid(*o2).name() << ". ");

    return dependent_empty;
  }


  void add_stored_object(pstatic_stored_object_key k, pstatic_stored_object o,
                         permanence perm) {
    GMM_ASSERT1(dal_static_stored_tab_valid__, "Too late to add an object");
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    if (dal_static_stored_tab_valid__)
      stored_objects.add_stored_object(k,o,perm);
  }

  void basic_delete(std::list<pstatic_stored_object> &to_delete) {
    stored_object_tab& stored_objects_this_thread
      = dal::singleton<stored_object_tab>::instance();

    if (dal_static_stored_tab_valid__) {

    stored_objects_this_thread.basic_delete_(to_delete);
    
    if (!to_delete.empty()) //need to delete from other threads
      {
        for(size_t thread=0; thread < num_threads(); ++thread)
	  { 
	    if (thread == this_thread()) continue;
	    stored_object_tab& stored_objects
              = dal::singleton<stored_object_tab>::instance(thread);
	    stored_objects.basic_delete_(to_delete);
	    if (to_delete.empty()) break;
	  }
      }
    if (me_is_multithreaded_now())
      {
        if (!to_delete.empty()) GMM_WARNING1("Not all objects were deleted");
      }
    else
      {
        GMM_ASSERT1(to_delete.empty(), "Could not delete objects");
      }
    }
  }
  
  void del_stored_objects(std::list<pstatic_stored_object> &to_delete,
                          bool ignore_unstored) 
  {
    getfem::omp_guard lock;
    GMM_NOPERATION(lock);
    // stored_object_tab& stored_objects
    //   = dal::singleton<stored_object_tab>::instance();

    if (dal_static_stored_tab_valid__) {

      std::list<pstatic_stored_object>::iterator it, itnext;
      for (it = to_delete.begin(); it != to_delete.end(); it = itnext) {
        itnext = it; itnext++;

        auto itos = iterators_of_object(*it);
        if (itos.first == itos.second) {
          if (ignore_unstored)
            to_delete.erase(it);
          else
            if (me_is_multithreaded_now()) {
              GMM_WARNING1("This object is (already?) not stored : "<< it->get()
              << " typename: " << typeid(*it->get()).name() 
              << "(which could happen in multithreaded code and is OK)");
            } else {
              GMM_ASSERT1(false, "This object is not stored : " << it->get()
              << " typename: " << typeid(*it->get()).name());
            }
        }
        else
          itos.first->second.valid = false;
      }

      for (pstatic_stored_object pobj : to_delete) {
        if (pobj) {
          auto itos = iterators_of_object(pobj);
          GMM_ASSERT1(itos.first != itos.second, "An object disapeared !");
          itos.first->second.valid = false;
          auto second_dep = itos.first->second.dependencies;
          for (const pstatic_stored_object pdep : second_dep) {
            if (del_dependency(pobj, pdep)) {
              auto itods = iterators_of_object(pdep);
              if (itods.first->second.perm == AUTODELETE_STATIC_OBJECT
                  && itods.first->second.valid) {
                itods.first->second.valid = false;
                to_delete.push_back(pdep);
              }
            }
          }
          for (pstatic_stored_object
               pdep : itos.first->second.dependent_object) {
            auto itods = iterators_of_object(pdep);
            if (itods.first != itods.second) {
              GMM_ASSERT1(itods.first->second.perm != PERMANENT_STATIC_OBJECT,
              "Trying to delete a permanent object " << pdep);
              if (itods.first->second.valid) {
                itods.first->second.valid = false;
                to_delete.push_back(itods.first->second.p);
              }
            }
          }
        }
      }
      basic_delete(to_delete);
    }
  }


  void del_stored_object(const pstatic_stored_object &o, bool ignore_unstored) 
  {
    std::list<pstatic_stored_object> to_delete;
    to_delete.push_back(o);
    del_stored_objects(to_delete, ignore_unstored);
  }


  void del_stored_objects(permanence perm) 
  {
    std::list<pstatic_stored_object> to_delete;
    for(size_t thread=0; thread<getfem::num_threads();thread++)
    {
      stored_object_tab& stored_objects
        = dal::singleton<stored_object_tab>::instance(thread);
      if (!dal_static_stored_tab_valid__) continue;
      if (perm == PERMANENT_STATIC_OBJECT) perm = STRONG_STATIC_OBJECT;
      stored_object_tab::iterator it;
      for (it = stored_objects.begin(); it != stored_objects.end(); ++it)
        if (it->second.perm >= perm)
          to_delete.push_back(it->second.p);
    }
    del_stored_objects(to_delete, false);
  }

  void list_stored_objects(std::ostream &ost) 
  {
    for(size_t thread=0; thread<getfem::num_threads();thread++)
    {
      stored_object_tab::stored_key_tab& stored_keys 
        = dal::singleton<stored_object_tab>::instance(thread).stored_keys_;
      if (!dal_static_stored_tab_valid__) continue;
      if (stored_keys.begin() == stored_keys.end())
        ost << "No static stored objects" << endl;
      else ost << "Static stored objects" << endl;
      for (const auto &t : stored_keys) 
        ost << "Object: " << t.first << " typename: "
            << typeid(*(t.first)).name() << endl;
    }
  }

  size_t nb_stored_objects(void) 
  {
    long num_objects=0;
    for(size_t thread=0;thread<getfem::num_threads(); ++thread)
    {
      stored_object_tab::stored_key_tab& stored_keys 
      = dal::singleton<stored_object_tab>::instance(thread).stored_keys_;
      if (!dal_static_stored_tab_valid__) continue;
      num_objects+=stored_keys.size();
    }
    return num_objects;
  }




/**
  STATIC_STORED_TAB -------------------------------------------------------
*/
  stored_object_tab::stored_object_tab() 
    : std::map<enr_static_stored_object_key, enr_static_stored_object>(),
      locks_(), stored_keys_()
  { dal_static_stored_tab_valid__ = true; }

  stored_object_tab::~stored_object_tab()
  { dal_static_stored_tab_valid__ = false; }

  pstatic_stored_object
  stored_object_tab::search_stored_object(pstatic_stored_object_key k) const
  {
   getfem::local_guard guard = locks_.get_lock();
   stored_object_tab::const_iterator it=find(enr_static_stored_object_key(k));
   return (it != end()) ? it->second.p : 0;
  }

  bool stored_object_tab::add_dependency_(pstatic_stored_object o1,
                                          pstatic_stored_object o2)
  {
    getfem::local_guard guard = locks_.get_lock();
    stored_key_tab::const_iterator it = stored_keys_.find(o1);
    if (it == stored_keys_.end()) return false;
    iterator ito1 = find(it->second);
    GMM_ASSERT1(ito1 != end(), "Object has a key, but cannot be found");
    ito1->second.dependencies.insert(o2);
    return true;
  }

  void stored_object_tab::add_stored_object(pstatic_stored_object_key k, 
    pstatic_stored_object o,  permanence perm) 
  {
    DAL_STORED_OBJECT_DEBUG_ADDED(o.get());
    getfem::local_guard guard = locks_.get_lock();
    GMM_ASSERT1(stored_keys_.find(o) == stored_keys_.end(),
      "This object has already been stored, possibly with another key");
    stored_keys_[o] = k;
    insert(std::make_pair(enr_static_stored_object_key(k),
                          enr_static_stored_object(o, perm)));
    size_t t = this_thread();
    GMM_ASSERT2(stored_keys_.size() == size() && t != size_t(-1), 
      "stored_keys are not consistent with stored_object tab");
  }

  bool stored_object_tab::add_dependent_(pstatic_stored_object o1,
    pstatic_stored_object o2)
  {
    getfem::local_guard guard = locks_.get_lock();
    stored_key_tab::const_iterator it = stored_keys_.find(o2);
    if (it == stored_keys_.end()) return false;
    iterator ito2 = find(it->second);
    GMM_ASSERT1(ito2 != end(), "Object has a key, but cannot be found");
    ito2->second.dependent_object.insert(o1);
    return true;
  }

  bool stored_object_tab::del_dependency_(pstatic_stored_object o1,
    pstatic_stored_object o2)
  {
    getfem::local_guard guard = locks_.get_lock();
    stored_key_tab::const_iterator it1 = stored_keys_.find(o1);
    if (it1 == stored_keys_.end()) return false;
    iterator ito1 = find(it1->second);
    GMM_ASSERT1(ito1 != end(), "Object has a key, but cannot be found");
    ito1->second.dependencies.erase(o2);
    return true;
  }

  stored_object_tab::iterator stored_object_tab::iterator_of_object_(pstatic_stored_object o)
  {
    stored_key_tab::const_iterator itk = stored_keys_.find(o);
    if (itk == stored_keys_.end()) return end();
    iterator ito = find(itk->second);
    GMM_ASSERT1(ito != end(), "Object has a key, but is not stored");
    return ito;
  }

  bool stored_object_tab::del_dependent_(pstatic_stored_object o1,
    pstatic_stored_object o2)
  {
    getfem::local_guard guard = locks_.get_lock();
    stored_key_tab::const_iterator it2 = stored_keys_.find(o2);
    if (it2 == stored_keys_.end()) return false;
    iterator ito2 = find(it2->second);
    GMM_ASSERT1(ito2 != end(), "Object has a key, but cannot be found");
    ito2->second.dependent_object.erase(o1);
    return true;
  }

  bool stored_object_tab::exists_stored_object(pstatic_stored_object o) const
  {
    getfem::local_guard guard = locks_.get_lock();
    return (stored_keys_.find(o) != stored_keys_.end());
  }

  bool stored_object_tab::has_dependent_objects(pstatic_stored_object o) const
  {
    getfem::local_guard guard = locks_.get_lock();
    stored_key_tab::const_iterator it = stored_keys_.find(o);
    GMM_ASSERT1(it != stored_keys_.end(), "Object is not stored");
    const_iterator ito = find(it->second);
    GMM_ASSERT1(ito != end(), "Object has a key, but cannot be found");
    return ito->second.dependent_object.empty();
  }



  void stored_object_tab::basic_delete_
  (std::list<pstatic_stored_object> &to_delete)
  {
    getfem::local_guard guard = locks_.get_lock();
    std::list<pstatic_stored_object>::iterator it;
    for (it = to_delete.begin(); it != to_delete.end(); ) 
    {
      DAL_STORED_OBJECT_DEBUG_DELETED(it->get());
      stored_key_tab::iterator itk = stored_keys_.find(*it);
      stored_object_tab::iterator ito = end();
      if (itk != stored_keys_.end())
      {
          ito = find(itk->second);
          stored_keys_.erase(itk);
      }
      if (ito != end()) 
      {
        erase(ito);
        it = to_delete.erase(it);
      } else {
        ++it;
      }
    }
  }



}
