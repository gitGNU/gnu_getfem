// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2001-2006 Y. Renard, J. Pommier.
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

/**\file gf_mdstate_set.cc
   \brief getfemint_mdstate setter.
*/

#include <getfemint.h>
#include <getfemint_mdstate.h>
#include <getfemint_mdbrick.h>

using namespace getfemint;


/*MLABCOM

  FUNCTION M = gf_mdstate_set(cmd, [, args])
  Modify a model state object.
  
  @SET MDSTATE:SET('compute_reduced_system')
  @SET MDSTATE:SET('compute_reduced_residual')
  @SET MDSTATE:SET('compute_residual')
  @SET MDSTATE:SET('compute_tangent_matrix')
  @SET MDSTATE:SET('clear')

  $Id$
MLABCOM*/

void gf_mdstate_set(getfemint::mexargs_in& in, getfemint::mexargs_out& out)
{
  if (in.narg() < 2) {
    THROW_BADARG( "Wrong number of input arguments");
  }
  getfemint_mdstate *md  = in.pop().to_getfemint_mdstate(true);
  std::string cmd        = in.pop().to_string();
  if (check_cmd(cmd, "compute_reduced_system", in, out, 0, 0, 0, 0)) {
    /*@SET MDSTATE:SET('compute_reduced_system')
      Compute the reduced system from the tangent matrix and constraints.
      @*/
    if (!md->is_complex()) md->real_mdstate().compute_reduced_system();
    else                   md->cplx_mdstate().compute_reduced_system();
  } else if (check_cmd(cmd, "compute_reduced_residual", in, out, 0, 0, 0, 0)) {
    /*@SET MDSTATE:SET('compute_reduced_residual')
      Compute the reduced residual from the residual and constraints.
      @*/
    if (!md->is_complex()) md->real_mdstate().compute_reduced_residual();
    else                   md->cplx_mdstate().compute_reduced_residual();
  } else if (check_cmd(cmd, "compute_residual", in, out, 1, 1, 0, 0)) {
    /*@SET MDSTATE:SET('compute_residual', @mdbrick B)
      Compute the residual for the brick B.
      @*/
    getfemint_mdbrick *b = in.pop().to_getfemint_mdbrick();
    if (md->is_complex() != b->is_complex()) 
      THROW_BADARG("MdState and MdBrick not compatible (real/complex)");
    if (!md->is_complex()) 
         b->real_mdbrick().compute_residual(md->real_mdstate());
    else b->cplx_mdbrick().compute_residual(md->cplx_mdstate());
  } else if (check_cmd(cmd, "compute_tangent_matrix", in, out, 1, 1, 0, 0)) {
    /*@SET MDSTATE:SET('compute_tangent_matrix', @mdbrick B)
      Update the tangent matrix from the brick B.
      @*/
    getfemint_mdbrick *b = in.pop().to_getfemint_mdbrick();
    if (md->is_complex() != b->is_complex()) 
      THROW_BADARG("MdState and MdBrick not compatible (real/complex)");
    if (!md->is_complex()) 
         b->real_mdbrick().compute_tangent_matrix(md->real_mdstate());
    else b->cplx_mdbrick().compute_tangent_matrix(md->cplx_mdstate());
  } else if (check_cmd(cmd, "clear", in, out, 0, 0, 0, 1)) {
    /*@SET MDSTATE:SET('clear')
      Clear the model state.
      @*/
    md->clear();
  } else bad_cmd(cmd);
}