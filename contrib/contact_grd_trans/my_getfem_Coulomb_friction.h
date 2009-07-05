// -*- c++ -*- (enables emacs c++ mode)
//===========================================================================
//
// Copyright (C) 2004-2008 Yves Renard
//
// This file is a part of GETFEM++
//
// Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
// under  the  terms  of the  GNU  Lesser General Public License as published
// by  the  Free Software Foundation;  either version 2.1 of the License,  or
// (at your option) any later version.
// This program  is  distributed  in  the  hope  that it will be useful,  but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
// You  should  have received a copy of the GNU Lesser General Public License
// along  with  this program;  if not, write to the Free Software Foundation,
// Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
// As a special exception, you  may use  this file  as it is a part of a free
// software  library  without  restriction.  Specifically,  if   other  files
// instantiate  templates  or  use macros or inline functions from this file,
// or  you compile this  file  and  link  it  with other files  to produce an
// executable, this file  does  not  by itself cause the resulting executable
// to be covered  by the GNU Lesser General Public License.  This   exception
// does not  however  invalidate  any  other  reasons why the executable file
// might be covered by the GNU Lesser General Public License.
//
//===========================================================================

/** @file getfem_Coulomb_friction.h
    @author Yves Renard <Yves.Renard@insa-lyon.fr>
    @author Julien Pommier <Julien.Pommier@insa-toulouse.fr>
    @date July 6, 2004.
    @brief Unilateral contact and Coulomb friction condition brick.
 */
#ifndef GETFEM_COULOMB_FRICTION_H__
#define GETFEM_COULOMB_FRICTION_H__

#include "getfem/getfem_modeling.h"

namespace getfem {


# define MDBRICK_COULOMB_FRICTION 434245

  /**
   * Unilateral contact and Coulomb friction condition brick.
   * (for conformal small displacement problems)
   * @ingroup bricks
   */
  template<typename MODEL_STATE = standard_model_state>
  class mdbrick_Coulomb_friction : public mdbrick_abstract<MODEL_STATE>  {
    
    TYPEDEF_MODEL_STATE_TYPES;

    mdbrick_abstract<MODEL_STATE> &sub_problem;
    size_type num_fem;

    T_MATRIX BN, BT;
    typedef gmm::row_matrix<gmm::rsvector<value_type> > RT_MATRIX;
    RT_MATRIX AUG_M; // For Hansbo augmentation.
    VECTOR gap, threshold, WT, WN, friction_coef, RLN, RLT;
    value_type r, alpha, beta;
    size_type d, nbc;
    gmm::dense_matrix<size_type> CH;

    const mesh_fem *mf_u;
    gmm::sub_interval SUBU, SUBN, SUBT;
    
    bool Tresca_version, symmetrized, contact_only, really_stationary;

    template<typename VEC> static void ball_projection(const VEC &x,
						       value_type radius) {
      value_type a = gmm::vect_norm2(x);
      if (radius <= 0) gmm::clear(const_cast<VEC&>(x));
      else if (a > radius) gmm::scale(const_cast<VEC&>(x), radius/a); 
    }

    template<class VEC, class VECR>
    static void ball_projection_grad_r(const VEC &x, value_type radius,
				       VECR &g) {
      value_type a = gmm::vect_norm2(x);
      if (radius > 0 && a >= radius)
	gmm::copy(gmm::scaled(x, value_type(1)/a), g);
      else gmm::clear(g);
    }

    template <class VEC, class MAT>
    static void ball_projection_grad(const VEC &x, double radius, MAT &g) {
      if (radius <= value_type(0)) { gmm::clear(g); return; }
      gmm::copy(gmm::identity_matrix(), g);
      value_type a = gmm::vect_norm2(x);
      if (a >= radius) { 
	gmm::scale(g, radius/a);
	// gmm::rank_one_update(g, gmm::scaled(x, -radius/(a*a*a)), x);
	for (size_type i = 0; i < x.size(); ++i)
	  for (size_type j = 0; j < x.size(); ++j)
	    g(i,j) -= radius*x[i]*x[j] / (a*a*a);
      }
    }

    void precomp(MODEL_STATE &MS, size_type i0) {
      size_type i1 = this->mesh_fem_positions[num_fem];
      gmm::resize(RLN, gmm::mat_nrows(BN));
      gmm::resize(RLT, gmm::mat_nrows(BT));
      SUBU = gmm::sub_interval(i0 + i1, mf_u->nb_dof());
      SUBN = gmm::sub_interval(i0 + sub_problem.nb_dof(), gmm::mat_nrows(BN));
      SUBT = gmm::sub_interval(i0 + sub_problem.nb_dof() + gmm::mat_nrows(BN),
			       gmm::mat_nrows(BT));
      gmm::add(gmm::sub_vector(MS.state(), SUBN), gmm::scaled(gap, r*alpha), RLN);
      if (gmm::vect_size(WN) > 0)
	gmm::mult_add(BN, gmm::scaled(WN, -r*alpha), RLN);
      gmm::mult_add(BN, gmm::scaled(gmm::sub_vector(MS.state(), SUBU),
				    -r*alpha), RLN);
      if (gmm::mat_nrows(AUG_M) > 0)
	gmm::mult_add(AUG_M, gmm::scaled(gmm::sub_vector(MS.state(),SUBN),-r),
		      RLN);
      if (!contact_only) {
	gmm::copy(gmm::sub_vector(MS.state(), SUBT), RLT);
	if (gmm::vect_size(WT) > 0)
	  gmm::mult_add(BT, gmm::scaled(WT, -r*beta), RLT);
	if (!really_stationary)
	  gmm::mult_add(BT, gmm::scaled(gmm::sub_vector(MS.state(), SUBU),
					-r*beta), RLT);
      }
    }

    void proper_update(void) {
      mf_u = this->mesh_fems[num_fem];
      d = mf_u->linked_mesh().dim();
      gmm::resize(BN, nbc, mf_u->nb_dof());
      gmm::resize(BT, nbc*(d-1), mf_u->nb_dof());
      gmm::resize(gap, nbc); gmm::resize(friction_coef, nbc);
      gmm::resize(threshold, nbc);
      // gmm::resize(WT, mf_u->nb_dof()); gmm::resize(WN, mf_u->nb_dof());
      this->proper_additional_dof = gmm::mat_nrows(BN)
	+ (contact_only ? 0 : gmm::mat_nrows(BT));
      this->proper_mixed_variables.clear();
      this->proper_mixed_variables.add(sub_problem.nb_dof(),
				       this->proper_additional_dof);
    }

  public :
    
    inline size_type nb_contact_nodes(void) const
    { return gmm::mat_nrows(BN); }
    
    virtual void do_compute_tangent_matrix(MODEL_STATE &MS, size_type i0,
					   size_type) {
      precomp(MS, i0);
      
      RT_MATRIX BBN(gmm::mat_nrows(BN), gmm::mat_ncols(BN));
      RT_MATRIX MM(nb_contact_nodes(), nb_contact_nodes());
      gmm::copy(gmm::scaled(BN, -alpha), BBN);
      if (gmm::mat_nrows(AUG_M) > 0)
	gmm::copy(gmm::scaled(AUG_M, -value_type(1)), MM);
      size_type l = 0;
      for (size_type i=0; i < nb_contact_nodes(); ++i) {
	if (i == CH(l,0)) {
	  if (CH(l,1) == 0) {
	    gmm::clear(BBN[i]);
	    if (gmm::mat_nrows(AUG_M) > 0) gmm::clear(MM[i]);
	    MM(i, i) = -value_type(1)/r;
	  }
	  ++l;
	}
	else if (RLN[i] >= value_type(0)) {
	  gmm::clear(BBN[i]);
	  if (gmm::mat_nrows(AUG_M) > 0) gmm::clear(MM[i]);
	  MM(i, i) = -value_type(1)/r;
	}
      }
      gmm::copy(BBN, gmm::sub_matrix(MS.tangent_matrix(), SUBN, SUBU));
      gmm::copy(MM, gmm::sub_matrix(MS.tangent_matrix(), SUBN));

//       gmm::copy(gmm::scaled(BN, -alpha),
// 		gmm::sub_matrix(MS.tangent_matrix(), SUBN, SUBU));
//       gmm::clear(gmm::sub_matrix(MS.tangent_matrix(), SUBN));
//       if (gmm::mat_nrows(AUG_M) > 0)
// 	gmm::copy(gmm::scaled(AUG_M, -value_type(1)),
// 		  gmm::sub_matrix(MS.tangent_matrix(), SUBN));
//       for (size_type i=0; i < nb_contact_nodes(); ++i) {
// 	if (RLN[i] >= value_type(0)) {
// 	  gmm::clear(gmm::sub_matrix(MS.tangent_matrix(),
// 				     gmm::sub_interval(SUBN.first()+i,1),
// 				     SUBU));
// 	  if (gmm::mat_nrows(AUG_M) > 0)
// 	    gmm::clear(gmm::sub_matrix(MS.tangent_matrix(),
// 				  gmm::sub_interval(SUBN.first()+i,1), SUBN));
// 	  MS.tangent_matrix()(SUBN.first()+i, SUBN.first()+i)=-value_type(1)/r;
// 	}
//       }
      
      if (!contact_only) {
	base_matrix pg(d-1, d-1);
	base_vector vg(d-1);

	RT_MATRIX BBT(gmm::mat_nrows(BT), gmm::mat_ncols(BT));
	gmm::dense_matrix<value_type> BTi(d-1,  gmm::mat_ncols(BT));
	
	l = 0;
	for (size_type i=0; i < nb_contact_nodes(); ++i) {
	  gmm::sub_interval SUBI(i*(d-1), d-1);
	  gmm::sub_interval SUBJ(SUBT.first()+i*(d-1),(d-1));
	  gmm::sub_interval SUBJJ(i*(d-1),(d-1));
	  value_type th = Tresca_version ? threshold[i]
	    : - (MS.state())[SUBN.first()+i] * friction_coef[i];
	  
	  if (i == CH(l,0)) {
	    std::vector<double> my_rlt(1);
	    if (CH(l,2) == 0) my_rlt[0] = - 2.0;
	    else if (CH(l,3) == 0) my_rlt[0] = 2.0;
	    else my_rlt[0] = 0.0;
	    ball_projection_grad(my_rlt, 1.0, pg);
	  } else
	    ball_projection_grad(gmm::sub_vector(RLT, SUBI), th, pg);
	  if (!really_stationary)
	    gmm::mult(gmm::scaled(pg, -beta), 
		      gmm::sub_matrix(BT, SUBI,
				      gmm::sub_interval(0,gmm::mat_ncols(BT))),
		      BTi);
	  gmm::copy(BTi, gmm::sub_matrix(BBT, SUBJJ, SUBU));

	  if (!Tresca_version) {
	    if (i == CH(l,0)) {
	      std::vector<double> my_rlt(1);
	      if (CH(l,2) == 0) my_rlt[0] = - 2.0;
	      else if (CH(l,3) == 0) my_rlt[0] = 2.0;
	      else my_rlt[0] = 0.0;
	      ball_projection_grad_r(my_rlt, 1.0, vg);
	      ++l;
	    } else
	      ball_projection_grad_r(gmm::sub_vector(RLT, SUBI), th, vg);
	    for (size_type k = 0; k < d-1; ++k)
	      MS.tangent_matrix()(SUBT.first()+i*(d-1)+k, SUBN.first()+i)
		= - friction_coef[i] * vg[k] / r;
	  }
	  for (size_type j = 0; j < d-1; ++j) pg(j,j) -= value_type(1);
	  gmm::copy(gmm::scaled(pg,value_type(1)/r), 
		    gmm::sub_matrix(MS.tangent_matrix(), SUBJ));
	}
	T_MATRIX BBBT(gmm::mat_nrows(BT), gmm::mat_ncols(BT));
	gmm::copy(BBT, BBBT);
	gmm::copy(BBBT, gmm::sub_matrix(MS.tangent_matrix(), SUBT, SUBU));
      }

//       if (!contact_only) {
// 	base_matrix pg(d-1, d-1);
// 	base_vector vg(d-1);
	
// 	for (size_type i=0; i < nb_contact_nodes(); ++i) {
// 	  gmm::sub_interval SUBI(i*(d-1), d-1);
// 	  gmm::sub_interval SUBJ(SUBT.first()+i*(d-1),(d-1));
// 	  value_type th = Tresca_version ? threshold[i]
// 	    : - (MS.state())[SUBN.first()+i] * friction_coef[i];
	  
// 	  ball_projection_grad(gmm::sub_vector(RLT, SUBI), th, pg);
// 	  if (!really_stationary)
// 	    gmm::mult(gmm::scaled(pg, -beta), 
// 		      gmm::sub_matrix(BT, SUBI,
// 				      gmm::sub_interval(0,gmm::mat_ncols(BT))),
// 		      gmm::sub_matrix(MS.tangent_matrix(), SUBJ, SUBU));
	  
// 	  if (!Tresca_version) {
// 	    ball_projection_grad_r(gmm::sub_vector(RLT, SUBI), th, vg);
// 	    for (size_type k = 0; k < d-1; ++k)
// 	      MS.tangent_matrix()(SUBT.first()+i*(d-1)+k, SUBN.first()+i)
// 		= - friction_coef[i] * vg[k] / r;
// 	  }
// 	  for (size_type j = 0; j < d-1; ++j) pg(j,j) -= value_type(1);
// 	  gmm::copy(gmm::scaled(pg,value_type(1)/r), 
// 		    gmm::sub_matrix(MS.tangent_matrix(), SUBJ));
// 	}
//       }

      
      if (symmetrized) {
	T_MATRIX tmp(mf_u->nb_dof(), mf_u->nb_dof());
	
	gmm::resize(tmp, mf_u->nb_dof(), gmm::mat_nrows(BN));
	gmm::copy(gmm::transposed(gmm::sub_matrix(MS.tangent_matrix(),
						  SUBN, SUBU)), tmp);
	gmm::copy(tmp, gmm::sub_matrix(MS.tangent_matrix(), SUBU, SUBN));
	gmm::resize(tmp, mf_u->nb_dof(), mf_u->nb_dof());
	gmm::mult(gmm::transposed(gmm::scaled(BN,-r*alpha)),
		  gmm::sub_matrix(MS.tangent_matrix(), SUBN, SUBU), tmp);
	gmm::add(tmp, gmm::sub_matrix(MS.tangent_matrix(), SUBU));
	
	if (!contact_only) {
	  gmm::mult(gmm::transposed(gmm::scaled(BT,-r*beta)),
		    gmm::sub_matrix(MS.tangent_matrix(), SUBT, SUBU), tmp);
	  gmm::add(tmp, gmm::sub_matrix(MS.tangent_matrix(), SUBU));
	  gmm::resize(tmp, mf_u->nb_dof(), gmm::mat_nrows(BT));
	  gmm::copy(gmm::transposed(gmm::sub_matrix(MS.tangent_matrix(),
						    SUBT, SUBU)), tmp);
	  gmm::copy(tmp, gmm::sub_matrix(MS.tangent_matrix(), SUBU, SUBT));
	}
      }
      else {
	gmm::copy(gmm::scaled(gmm::transposed(BN), value_type(-1)),
		  gmm::sub_matrix(MS.tangent_matrix(), SUBU, SUBN));
	if (!contact_only)
	  gmm::copy(gmm::scaled(gmm::transposed(BT), value_type(-1)), 
		    gmm::sub_matrix(MS.tangent_matrix(), SUBU, SUBT));
      }
    }
    
    virtual void do_compute_residual(MODEL_STATE &MS, size_type i0,size_type) {
      precomp(MS, i0);
      value_type c1(1);
      size_type l = 0;
      
      for (size_type i=0; i < nb_contact_nodes(); ++i) {
	if (i == CH(l,0)) {
	  if (CH(l,1) == 0) RLN[i] = value_type(0);
	  if (!contact_only) {
	    if (CH(l,2) == 0) 
	      RLT[i] = Tresca_version ? 
		-threshold[i] : friction_coef[i]*(MS.state())[SUBN.first()+i];
	    else if (CH(l,3) == 0) 
	      RLT[i] = Tresca_version ? 
		threshold[i] : -friction_coef[i]*(MS.state())[SUBN.first()+i];
	  }
	  ++l;
	}
	else {
	  RLN[i] = std::min(value_type(0), RLN[i]);
	  if (!contact_only)
	    ball_projection(gmm::sub_vector(RLT, gmm::sub_interval(i*(d-1),d-1)),
			    Tresca_version ? threshold[i]
			    : -friction_coef[i]*(MS.state())[SUBN.first()+i]);
	}
      }
      
      if (symmetrized) {
	gmm::mult_add(gmm::transposed(BN), gmm::scaled(RLN, -c1),
		      gmm::sub_vector(MS.residual(), SUBU));
	if (!contact_only)
	  gmm::mult_add(gmm::transposed(BT), gmm::scaled(RLT, -c1),
			gmm::sub_vector(MS.residual(), SUBU));
      } else {
	gmm::mult_add(gmm::transposed(BN),
		      gmm::scaled(gmm::sub_vector(MS.state(), SUBN),-c1),
		      gmm::sub_vector(MS.residual(), SUBU));
	if (!contact_only)
	  gmm::mult_add(gmm::transposed(BT),
			gmm::scaled(gmm::sub_vector(MS.state(), SUBT),-c1),
			gmm::sub_vector(MS.residual(), SUBU));
      }
      
      /* residual on LN */
      gmm::add(gmm::scaled(gmm::sub_vector(MS.state(), SUBN), -c1/r),
	       gmm::scaled(RLN, c1/r), gmm::sub_vector(MS.residual(), SUBN));
      // gmm::scale(gmm::sub_vector(MS.residual(), SUBN), c1 / r);

      /* residual on LT */
      if (!contact_only) {
	gmm::add(gmm::scaled(gmm::sub_vector(MS.state(),SUBT), -c1/r),
		 gmm::scaled(RLT, c1/r), gmm::sub_vector(MS.residual(), SUBT));
	// gmm::scale(gmm::sub_vector(MS.residual(), SUBT), c1 / r);
      }
    }

    void init(void) {
      this->add_sub_brick(sub_problem);
      this->proper_is_linear_ = this->proper_is_coercive_ = false;
      this->proper_is_symmetric_ = symmetrized && contact_only;
      r = value_type(1);
      beta = value_type(1);
      alpha = value_type(1);
      this->force_update();
    }

    void set_stationary(bool b) { really_stationary = b; }
    void set_beta(value_type a) { beta = a; }
    void set_alpha(value_type a) { alpha = a; }
    void set_CH(const gmm::dense_matrix<size_type> &CH_) {
      gmm::resize(CH, gmm::mat_nrows(CH_), gmm::mat_ncols(CH_ ));
      gmm::copy(CH_, CH);
    }

    template<typename MAT> void set_augmented_matrix(const MAT &M) {
      gmm::resize(AUG_M, gmm::mat_nrows(M), gmm::mat_ncols(M));
      gmm::copy(M, AUG_M);
    }

    void set_r(value_type r_) { r = r_; }
    value_type get_r(void) const { return r; }
    template <class VEC> void set_WT(const VEC &WT_)
    { gmm::resize(WT, gmm::vect_size(WT_)); gmm::copy(WT_, WT); }
    template <class VEC> void set_WN(const VEC &WN_)
    { gmm::resize(WN, gmm::vect_size(WN_)); gmm::copy(WN_, WN); }

    VECTOR &get_gap(void) { return gap; }
    const VECTOR &get_gap(void) const { return gap; }

    VECTOR &get_friction_coef(void) { return friction_coef; }
    const VECTOR &get_friction_coef(void) const { return friction_coef; }

    SUBVECTOR get_LN(MODEL_STATE &MS) {
      SUBN = gmm::sub_interval(this->first_index() + sub_problem.nb_dof(),
			       gmm::mat_nrows(BN));
      return gmm::sub_vector(MS.state(), SUBN);
    }

    SUBVECTOR get_LT(MODEL_STATE &MS) {
      SUBT = gmm::sub_interval(this->first_index() + sub_problem.nb_dof()
			       + gmm::mat_nrows(BN),  gmm::mat_nrows(BT));
      return gmm::sub_vector(MS.state(), SUBT);
    }

    /* contact and friction */
    template <class MAT, class VEC> mdbrick_Coulomb_friction
    (mdbrick_abstract<MODEL_STATE> &problem, const MAT &BN_, const VEC &gap_,
     scalar_type FC_, const MAT &BT_, size_type num_fem_=0)
      : sub_problem(problem), num_fem(num_fem_) {
      contact_only = Tresca_version = symmetrized = really_stationary = false;
      nbc = gmm::mat_nrows(BN_);
      init();
      gmm::copy(BN_, BN); gmm::copy(BT_, BT); gmm::copy(gap_, gap);
      std::fill(friction_coef.begin(), friction_coef.end(), FC_);
    }

    /* contact only.        */
    template <class MAT, class VEC> mdbrick_Coulomb_friction
    (mdbrick_abstract<MODEL_STATE> &problem, const MAT &BN_, const VEC &gap_,
     size_type num_fem_=0) : sub_problem(problem), num_fem(num_fem_) {
      contact_only = true;
      Tresca_version = symmetrized = really_stationary = false;
      nbc = gmm::mat_nrows(BN_);
      init();
      gmm::copy(BN_, BN); gmm::copy(gap_, gap);
    }

  };


}  /* end of namespace getfem.                                             */


#endif /* GETFEM_COULOMB_FRICTION_H__ */