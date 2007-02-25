// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2000-2007 Julien Pommier
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

#ifndef BGEOT_RTREE_H
#define BGEOT_RTREE_H

/** @file bgeot_rtree.h
   @author  Julien Pommier <Julien.Pommier@insa-toulouse.fr>
   @date January 2004.
    @brief region-tree for window/point search on a set of rectangles.
*/

#include <set>
#include "bgeot_vector.h"

namespace bgeot {

  struct box_index {
    size_type id;
    base_node min, max;
  };  

  struct rtree_elt_base;

  /** Balanced tree of n-dimensional rectangles.
   *
   * This is not a dynamic structure. Once a query has been made on the
   * tree, new boxes should not be added.
   */
  class rtree : public boost::noncopyable {
  public:
    typedef std::deque<box_index> box_cont;
    typedef std::vector<const box_index*> pbox_cont;
    typedef std::set<const box_index*> pbox_set;

    rtree() : root(0) {}
    ~rtree() { destroy_tree(); }
    void add_box(base_node min, base_node max, size_type id=size_type(-1)) {
      box_index bi; bi.min = min; bi.max = max; bi.id = (id + 1) ? id : boxes.size();
      boxes.push_back(bi);
    }
    size_type nb_boxes() const { return boxes.size(); }
    void clear() { destroy_tree(); boxes.clear(); }

    void find_intersecting_boxes(const base_node& bmin, const base_node& bmax, pbox_set& boxlst);
    void find_containing_boxes(const base_node& bmin, const base_node& bmax, pbox_set& boxlst);
    void find_contained_boxes(const base_node& bmin, const base_node& bmax, pbox_set& boxlst);
    void find_boxes_at_point(const base_node& P, pbox_set& boxlst);

    void find_intersecting_boxes(const base_node& bmin, const base_node& bmax, std::vector<size_type>& idvec)
    { pbox_set bs; find_intersecting_boxes(bmin, bmax, bs);  pbox_set_to_idvec(bs, idvec); }
    void find_containing_boxes(const base_node& bmin, const base_node& bmax, std::vector<size_type>& idvec)
    { pbox_set bs; find_containing_boxes(bmin, bmax, bs);  pbox_set_to_idvec(bs, idvec); }
    void find_contained_boxes(const base_node& bmin, const base_node& bmax, std::vector<size_type>& idvec)
    { pbox_set bs; find_contained_boxes(bmin, bmax, bs);  pbox_set_to_idvec(bs, idvec); }
    void find_boxes_at_point(const base_node& P, std::vector<size_type>& idvec)
    { pbox_set bs; find_boxes_at_point(P, bs);  pbox_set_to_idvec(bs, idvec); }
    void dump();
  private:
    void build_tree();
    void destroy_tree();
    static void pbox_set_to_idvec(pbox_set bs, std::vector<size_type>& idvec) {
      idvec.reserve(bs.size()); idvec.resize(0);
      for (pbox_set::const_iterator it=bs.begin(); it != bs.end(); ++it) idvec.push_back((*it)->id);
    }

    box_cont boxes;
    rtree_elt_base *root;
  };

}

#endif