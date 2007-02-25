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

/**\file gf_mdbrick_set.cc
   \brief getfemint_mdbrick getter.
*/

#include <getfemint.h>
#include <getfemint_misc.h>
#include <getfemint_mdbrick.h>
#include <getfemint_mdstate.h>
#include <getfem/getfem_model_solvers.h>

using namespace getfemint;

/*MLABCOM

  FUNCTION M = gf_mdbrick_get(cmd, [, args])
  Get information from a brick, or launch the solver.

  @RDATTR MDBRICK:GET('nbdof')
  @RDATTR MDBRICK:GET('dim')
  @RDATTR MDBRICK:GET('is_linear')
  @RDATTR MDBRICK:GET('is_symmetric')
  @RDATTR MDBRICK:GET('is_coercive')
  @RDATTR MDBRICK:GET('is_complex')
  @GET MDBRICK:GET('mixed_variables') 
  @RDATTR MDBRICK:GET('subclass')
  @GET MDBRICK:GET('param_list')
  @GET MDBRICK:GET('param')
  @GET MDBRICK:GET('solve')
  @GET MDBRICK:GET('von mises')
  @GET MDBRICK:GET('tresca')
  @GET MDBRICK:GET('memsize')

  $Id$
MLABCOM*/
void gf_mdbrick_get(getfemint::mexargs_in& in, getfemint::mexargs_out& out)
{
  if (in.narg() < 2) {
    THROW_BADARG( "Wrong number of input arguments");
  }
  getfemint_mdbrick *b   = in.pop().to_getfemint_mdbrick();
  std::string cmd        = in.pop().to_string();
  if (check_cmd(cmd, "typename", in, out, 0, 0, 0, 1)) {
    out.pop().from_string(typeid(b->mdbrick()).name());
  } else if (check_cmd(cmd, "nbdof", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MDBRICK:GET('nbdof')
      Get the total number of dof of the current problem.

      This is the sum of the brick specific dof plus the dof of the
      parent bricks.
      @*/
    out.pop().from_integer(b->mdbrick().nb_dof());
  } else if (check_cmd(cmd, "dim", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MDBRICK:GET('dim')
      Get the dimension of the main mesh (2 for a 2D mesh, etc).
      @*/
    out.pop().from_integer(b->mdbrick().dim());
  } else if (check_cmd(cmd, "nb_constraints", in, out, 0, 0, 0, 1)) {
    out.pop().from_integer(b->mdbrick().nb_constraints());
  } else if (check_cmd(cmd, "is_linear", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MDBRICK:GET('is_linear')
      Return true if the problem is linear.
      @*/
    out.pop().from_integer(b->mdbrick().is_linear());
  } else if (check_cmd(cmd, "is_symmetric", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MDBRICK:GET('is_symmetric')
      Return true if the problem is symmetric.
      @*/
    out.pop().from_integer(b->mdbrick().is_symmetric());
  } else if (check_cmd(cmd, "is_coercive", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MDBRICK:GET('is_coercive')
      Return true if the problem is coercive.
      @*/
    out.pop().from_integer(b->mdbrick().is_coercive());
  } else if (check_cmd(cmd, "is_complex", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MDBRICK:GET('is_complex')
      Return true if the problem uses complex numbers.
      @*/
    out.pop().from_integer(b->is_complex());
  } else if (check_cmd(cmd, "mixed_variables", in, out, 0, 0, 0, 1)) {
    /*@GET MDBRICK:GET('mixed_variables') 
    Identify the indices of mixed variables (typically the pressure,
    etc.) in the tangent matrix.
    @*/
    out.pop().from_bit_vector(b->mdbrick().mixed_variables());
  } else if (check_cmd(cmd, "subclass", in, out, 0, 0, 0, 1)) {
    /*@RDATTR MDBRICK:GET('subclass')
      Get the typename of the brick.
      @*/
    out.pop().from_string(b->sub_class().c_str());
  } else if (check_cmd(cmd, "param_list", in, out, 0, 0, 0, 1)) {
    /*@GET MDBRICK:GET('param_list') 
      Get the list of parameters names.
      
      Each brick embeds a number of parameters (the Lam� coefficients
      for the linearized elasticity brick, the wave number for the
      Helmholtz brick,...), described as a (scalar, or vector, tensor
      etc) field on a mesh_fem. You can read/change the parameter values with
      MDBRICK:GET('param') and MDBRICK:SET('param').
      @*/
    std::vector<std::string> lst;
    getfem::mdbrick_abstract_common_base::PARAM_MAP::const_iterator 
      it  = b->mdbrick().get_parameters().begin(), 
      ite = b->mdbrick().get_parameters().end();
    for ( ; it != ite; ++it) lst.push_back(it->first);
    out.pop().from_string_container(lst);
  } else if (check_cmd(cmd, "param", in, out, 1, 2, 0, 1)) {
    /*@GET MDBRICK:GET('param', @str parameter_name) 
      Get the parameter value.
      
      When the parameter has been assigned a specific mesh_fem, it is returned 
      as a large array (the last dimension being the mesh_fem dof). When no mesh_fem
      has been assigned, the parameter is considered to be constant over the mesh.
      @*/
    
    std::string pname = in.pop().to_string();
    for (unsigned i=0; i < pname.size(); ++i) if (pname[i] == ' ') pname[i] = '_';
    getfem::mdbrick_abstract_parameter *p = b->param(pname);
    if (!p) THROW_BADARG("wrong parameter name for this brick: " << pname);
    real_mdbrick_parameter *rp = dynamic_cast<real_mdbrick_parameter*>(p);
    cplx_mdbrick_parameter *cp = dynamic_cast<cplx_mdbrick_parameter*>(p);
    
    bool is_default_mf = p->is_using_default_mesh_fem();
    unsigned ll = (is_default_mf ? p->mf().nb_dof() : 1);

    array_dimensions ad; 
    if (!config::has_1D_arrays() && p->fsizes().size() == 0 && !is_default_mf) {
      /* prefer a row vector for matlab */
      ad.push_back(1);
    } else ad.assign(p->fsizes()); 
    if (!is_default_mf) ad.push_back(p->mf().nb_dof()); 
    if (rp) {
      darray a = out.pop().create_array(ad, double());
      assert(a.size() == rp->get().size()/ll);
      std::copy(rp->get().begin(), rp->get().begin()+a.size(), a.begin());
    } else if (cp) {
      carray a = out.pop().create_array(ad, complex_type());
      assert(a.size() == cp->get().size()/ll);
      std::copy(cp->get().begin(), cp->get().begin() + a.size(), a.begin());
    } else {
      THROW_BADARG("unknown parameter type for '" << pname << 
		   "' (yes this is a bug)");
    }
    //check_sub_class(b, "mdBrick:ScalarElliptic");
    //out.pop().from_vector(b->cast<getfem::mdbrick_scalar_elliptic<real_model_state> >().get_coeff());
  } else if (check_cmd(cmd, "solve", in, out, 1, -1, 0, 0)) {
    /*@GET MDBRICK:GET('solve', @mdstate mds [,...])
      Run the standard getfem solver.
      
      Note that you should be able to use your own solver if you want
      (it is possible to obtain the tangent matrix and its right hand
      side with the MDSTATE:GET('tangent matrix') etc.). 

      Various options can be specified:<Par>
      
      - 'noisy' or 'very noisy' : the solver will display some<par>
      information showing the progress (residual values etc.).<par>
      - 'max_iter', NIT         : set the maximum iterations numbers.<par>
      - 'max_res', RES          : set the target residual value.<par>
      - 'lsolver', SOLVERNAME   : select explicitely the solver used for<
      the linear systems (the default value is 'auto', which lets
      getfem choose itself). Possible values are 'superlu', 'mumps'
      (if supported), 'cg/ildlt', 'gmres/ilu' and 'gmres/ilut'.
      @*/
    
    getfemint_mdstate *md = in.pop().to_getfemint_mdstate();
    if (b->is_complex() != md->is_complex()) 
      THROW_BADARG("cannot mix complex mdbricks with real mdstate");

    //gmm::iteration iter;
    getfemint::interruptible_iteration iter;
    std::string lsolver = "auto";
    while (in.remaining() && in.front().is_string()) {
      std::string opt = in.pop().to_string();
      if (cmd_strmatch(opt, "noisy")) iter.set_noisy(1);
      else if (cmd_strmatch(opt, "very noisy")) iter.set_noisy(2);
      else if (cmd_strmatch(opt, "max_iter")) {
	if (in.remaining()) iter.set_maxiter(in.pop().to_integer());
	else THROW_BADARG("missing value for " << opt);
      } else if (cmd_strmatch(opt, "max_res")) {
	if (in.remaining()) iter.set_resmax(in.pop().to_scalar());
	else THROW_BADARG("missing value for " << opt);
      } else if (cmd_strmatch(opt, "lsolver")) {
	if (in.remaining()) lsolver = in.pop().to_string();
	else THROW_BADARG("missing solver name for " << opt);
      } else THROW_BADARG("bad option: " << opt);
    }
    if (!md->is_complex()) {
      getfem::standard_solve(md->real_mdstate(), b->real_mdbrick(), iter,
			     getfem::select_linear_solver(b->real_mdbrick(), lsolver));
    } else {
      getfem::standard_solve(md->cplx_mdstate(), b->cplx_mdbrick(), iter,
			     getfem::select_linear_solver(b->cplx_mdbrick(), lsolver));
    }


  } else if (check_cmd(cmd, "von mises", in, out, 2, 2, 0, 1) ||
	     check_cmd(cmd, "tresca"   , in, out, 2, 2, 0, 1)) {
    /*@GET VM=MDBRICK:GET('von mises', @mdstate mds, @tmf MFVM)
      Compute the Von Mises stress on the mesh_fem MFVM.

      Only available on bricks where it has a meaning: linearized
      elasticity, plasticity, nonlinear elasticity.. Note that in 2D
      it is not the "real" Von Mises (which should take into account
      the 'plane stress' or 'plane strain' aspect), but a pure 2D Von
      Mises.
      @*/
    /*@GET VM=MDBRICK:GET('tresca', @mdstate mds, @tmf MFVM)
      Compute the Tresca stress criterion on the mesh_fem MFVM.

      Only available on bricks where it has a meaning: linearized elasticity, 
      plasticity, nonlinear elasticity..
      @*/
    bool tresca = cmd_strmatch(cmd, "tresca");
    getfemint_mdstate *mds = in.pop().to_getfemint_mdstate();
    const getfem::mesh_fem &mf_vm = *in.pop().to_const_mesh_fem();
    darray w = out.pop().create_darray_h(mf_vm.nb_dof());
    bool ok = false;
    if (!ok) {
      getfem::mdbrick_isotropic_linearized_elasticity<real_model_state> *bb = 
	b->cast0<getfem::mdbrick_isotropic_linearized_elasticity<real_model_state> >();
      if (bb) {
	bb->compute_Von_Mises_or_Tresca(mds->real_mdstate(), mf_vm, w, tresca);
	ok = true;
      }
    }
    if (!ok) {
      getfem::mdbrick_nonlinear_elasticity<real_model_state> *bb = 
	b->cast0<getfem::mdbrick_nonlinear_elasticity<real_model_state> >();
      if (bb) {
	bb->compute_Von_Mises_or_Tresca(mds->real_mdstate(), mf_vm, w, tresca);
	ok = true;
      }
    }
    if (!ok) {
      getfem::mdbrick_plasticity<real_model_state> *bb = 
	b->cast0<getfem::mdbrick_plasticity<real_model_state> >();
      if (bb) {
	bb->compute_constraints(mds->real_mdstate());
	bb->compute_Von_Mises_or_Tresca(mf_vm, w, tresca);
	ok = true;
      }
    }
    if (!ok) 
      THROW_BADARG("this brick cannot compute " << cmd << "!");
  } else if (check_cmd(cmd, "memsize", in, out, 0, 0, 0, 1)) {
    /*@GET MDBRICK:GET('memsize')
      Return the amount of memory (in bytes) used by the model brick.
      @*/
    out.pop().from_integer(b->memsize());
  } else bad_cmd(cmd);
  if (in.remaining()) THROW_BADARG("too many arguments!");
}