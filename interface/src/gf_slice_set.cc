// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2006-2006 Yves Renard, Julien Pommier.
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

#include <map>
#include <getfemint_misc.h>
#include <getfemint_mesh.h>
#include <getfemint_mesh_slice.h>
#include <getfem/getfem_mesh_slice.h>

using namespace getfemint;

/*MLABCOM

  FUNCTION [...] = gf_slice_set(sl, operation)

  Edition of mesh slices.

  @SET SLICE:SET('pts')

  $Id$
MLABCOM*/


void gf_slice_set(getfemint::mexargs_in& in, getfemint::mexargs_out& out)
{
  if (in.narg() < 2) {
    THROW_BADARG( "Wrong number of input arguments");
  }
  getfemint_mesh_slice *mi_sl = in.pop().to_getfemint_mesh_slice(true);
  getfem::stored_mesh_slice *sl = &mi_sl->mesh_slice();
  std::string cmd                  = in.pop().to_string();
  if (check_cmd(cmd, "pts", in, out, 1, 1, 0, 0)) {
    /*@SET SLICE:SET('pts', @dmat P)
      Replace the points of the slice.
      
      The new points P are stored in the columns the matrix. Note that you can use
      the function to apply a deformation to a slice, or to change the dimension of
      the slice (the number of rows of P is not required to be equal to
      SLICE:GET('dim')).
      @*/
    darray w = in.pop().to_darray(-1, sl->nb_points());
    size_type min_dim = 0;
    for (size_type ic=0; ic < sl->nb_convex(); ++ic) {
      for (getfem::mesh_slicer::cs_simplexes_ct::const_iterator it = sl->simplexes(ic).begin();
	   it != sl->simplexes(ic).end(); ++it)
	min_dim = std::max(min_dim, it->dim());
    }
    if (w.getm() < min_dim) 
      GMM_THROW(getfemint_error, "can't reduce the dimension of the slice to " << 
		w.getm() << " (it contains simplexes of dimension " << min_dim << ")");
    sl->set_dim(w.getm()); /* resize the points */
    for (size_type ic=0, cnt=0; ic < sl->nb_convex(); ++ic) {
      for (getfem::mesh_slicer::cs_nodes_ct::iterator it=sl->nodes(ic).begin();
           it != sl->nodes(ic).end(); ++it) {
        for (size_type k=0; k < sl->dim(); ++k)
          (*it).pt[k] = w[cnt++];
      }
    }
  } else bad_cmd(cmd);
}