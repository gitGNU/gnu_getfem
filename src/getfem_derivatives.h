/* -*- c++ -*- (enables emacs c++ mode)                                    */
/* *********************************************************************** */
/*                                                                         */
/* Library :  GEneric Tool for Finite Element Methods (getfem)             */
/* File    :  getfem_assembling.h : assemble linear system for fem.        */
/*     									   */
/* Date : November 17, 2000.                                               */
/* Authors : Yves Renard, Yves.Renard@gmm.insa-tlse.fr                     */
/*           Julien Pommier, pommier@gmm.insa-tlse.fr                      */
/*                                                                         */
/* *********************************************************************** */
/*                                                                         */
/* Copyright (C) 2001  Yves Renard.                                        */
/*                                                                         */
/* This file is a part of GETFEM++                                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; version 2 of the License.                 */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.         */
/*                                                                         */
/* *********************************************************************** */

#ifndef __GETFEM_DERIVATIVES_H
#define __GETFEM_DERIVATIVES_H

#include <getfem_mesh_fem.h>

namespace getfem
{
  
  /* ********************************************************************* */
  /*       Precomputation on geometric transformations.                    */
  /* ********************************************************************* */

  class _geotrans_iprecomp
  {
  protected :
    
    bgeot::pgeometric_trans pgt;
    pfem pf;
    std::vector<base_matrix> pc;
    std::vector<base_matrix> hpc;
    
  public :

    inline const base_matrix &grad(size_type i) { return pc[i]; }
    inline const base_matrix &hessian(size_type i) { return hpc[i]; }

    _geotrans_iprecomp(const _ipre_geot_light &ls);
  };
  
  typedef _geotrans_iprecomp * pgeotrans_iprecomp;

  pgeotrans_iprecomp geotrans_iprecomp(bgeot::pgeometric_trans pg,
				       pfem pf);


  /*
    mf_target should be a lagrange discontinous element
    does not work with vectorial elements. ... to be done ...
  */

  template<class VECT>
  void compute_gradient(mesh_fem &mf, mesh_fem &mf_target,
			const VECT &U, VECT &V, dim_type Q)
  {
    size_type cv;
    size_type N = mf.linked_mesh().dim();
    assert(mf.linked_mesh() == mf_target.linked_mesh());
    base_matrix G, val;
    base_vector coeff;
 
    dal::bit_vector nn = mf.convex_index();
      
    pgeotrans_iprecomp pgip;
    pfem pf, pfold = NULL;
    pgeometric_trans pgt;

    for (cv << nn; cv != ST_NIL; cv << nn) {
      pf = mf_target.fem_of_element(cv);
      assert(pf->is_equivalent());
      assert(pf->is_lagrange());

      pgt = mf.linked_mesh().trans_of_convex(cv);
      if (pfold != pf)
        pgip = geotrans_iprecomp(pgt, pf);



      size_type P = pgt->structure()->dim(); /* dimension of the convex.*/
      base_matrix a(N, pgt->nb_points());
      base_matrix pc(pgt->nb_points() , P);
      base_matrix grad(N, P), TMP1(P,P), B0(P,N), B1(1, N), CS(P,P);
      size_type nbpt = 0, i;
      
      /* TODO: prendre des iterateurs pour faire la copie */
      for (size_type j = 0; j < pgt->nb_points(); ++j) // � optimiser !!
	for (size_type i = 0; i < N; ++i)
	  { 
	    a(i,j) = mf.linked_mesh().points_of_convex(cv)[j][i];
	  }
       
	  
      
      coeff.resize(pf->nb_dof());
      val.resize(pf->target_dim(), P);
      B1.resize(pf->target_dim(), N);
      
      for (size_type j = 0; j < pf->nb_dof(); ++j) {
	if (!pgt->is_linear() || j == 0) {
	  // computation of the pseudo inverse
	  bgeot::mat_product(a, pgip(j), grad);
	  if (P != N)
	    {
	      bgeot::mat_product_tn(grad, grad, CS);
	      bgeot::mat_inv_cholesky(CS, TMP1);
	      bgeot::mat_product_tt(CS, grad, B0);
	    }
	  else
	    {
	      bgeot::mat_gauss_inverse(grad, TMP1); B0 = grad;
	    }
	}

	assert(pf->target_dim() == 1); // !!
	for (size_type q = 0; q < Q; ++q) {
	  for (size_type l = 0; l < pf->nb_dof(); ++l)
	    coeff[l] = U[mf.ind_dof_of_element(cv)[l] * Q + q ];
	  pf->interpolation_grad(pf->node_of_dof(j), G, coeff, val);
	  bgeot::mat_product(val, B0, B1);

	  for (size_type l = 0; l < N; ++l)
	    V[mf_target.ind_dof_of_element(cv)[j]*Q*N+q*N+l] = B1(0, l);
	}
      }
	
    }
  }







}

#endif
