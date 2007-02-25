// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2001-2006 Yves Renard, Julien Pommier.
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

#ifndef GETFEM_MISC_H__
#define GETFEM_MISC_H__

#include <getfemint.h>
#include <gmm/gmm_iter.h>

namespace getfem {
  class abstract_hyperelastic_law;
}

namespace getfemint {
  typedef getfem::convex_face convex_face;
  typedef getfem::convex_face_ct convex_face_ct;

  gfi_array* convert_to_gfi_sparse(const gf_real_sparse_by_row & smat, double threshold=1e-12);
  gfi_array* convert_to_gfi_sparse(const gf_real_sparse_by_col & smat, double threshold=1e-12);


  void build_edge_list(const getfem::mesh &m, bgeot::edge_list &el, mexargs_in &in);

  void build_convex_face_lst(const getfem::mesh& m, std::vector<convex_face>& l, const iarray *v);

  getfem::mesh_region to_mesh_region(const iarray &v);
  getfem::mesh_region to_mesh_region(const getfem::mesh& m, const iarray *v=0);
  inline getfem::mesh_region to_mesh_region(const getfem::mesh& m, mexargs_in &in) {
    if (in.remaining()) { iarray v = in.pop().to_iarray(); return to_mesh_region(m, &v); }
    else return to_mesh_region(m);
  }

  void interpolate_on_convex_ref(const getfem::mesh_fem *mf, getfem::size_type cv, 
				 const std::vector<getfem::base_node> &pt, 
				 const darray& U,
				 getfem::base_matrix &pt_val);
  void
  eval_on_triangulated_surface(const getfem::mesh* mesh, int Nrefine,
			       const std::vector<convex_face>& cvf,
			       mexargs_out& out,
			       const getfem::mesh_fem *pmf, const darray& U);
  
  std::auto_ptr<getfem::abstract_hyperelastic_law> abstract_hyperelastic_law_from_name(const std::string &lawname);

  class interruptible_iteration : public gmm::iteration {
  public:
    interruptible_iteration(double r=1e-8);
  };
}
#endif