// -*- c++ -*- (enables emacs c++ mode)
//===========================================================================
//
// Copyright (C) 2009-2009 Yves Renard
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
//===========================================================================

#include <iomanip>
#include "gmm/gmm_range_basis.h"
#include "getfem/getfem_models.h"
#include "getfem/getfem_assembling.h"
#include "getfem/getfem_derivatives.h"


namespace getfem {

  void model::var_description::set_size(size_type s) {
    n_temp_iter = 0;
    default_iter = 0;
    if (is_complex)
      complex_value.resize(n_iter);
    else
      real_value.resize(n_iter);
    v_num_var_iter.resize(n_iter);
    for (size_type i = 0; i < n_iter; ++i)
      if (is_complex)
	complex_value[i].resize(s);
      else
	real_value[i].resize(s);
  }

  size_type model::var_description::add_temporary(gmm::uint64_type id_num) {
    size_type nit = n_iter;
    for (; nit < n_iter + n_temp_iter ; ++nit)
      if (v_num_var_iter[nit] == id_num) break;
    if (nit >=  n_iter + n_temp_iter) {
      ++n_temp_iter;
      if (is_complex) {
	size_type s = complex_value[0].size();
	complex_value.resize(n_iter + n_temp_iter);
	complex_value[nit].resize(s);
      } else {
	size_type s = real_value[0].size();
	real_value.resize(n_iter + n_temp_iter);
	real_value[nit].resize(s);
      }
    }
    default_iter = nit;
    return nit;
  }

  void model::var_description::clear_temporaries(void) {
    n_temp_iter = 0;
    default_iter = 0;
    if (is_complex)
      complex_value.resize(n_iter);
    else
      real_value.resize(n_iter);
  }

  bool model::check_name_valitity(const std::string &name, bool assert) const {
    VAR_SET::const_iterator it = variables.find(name);
    if (it != variables.end()) {
      GMM_ASSERT1(!assert, "Variable " << name << " already exists");
      return false;
    }
    bool valid = true;
    if (name.size() == 0) valid = false;
    else {
      if (!isalpha(name[0])) valid = false;
      for (size_type i = 1; i < name.size(); ++i)
	if (!(isalnum(name[i]) || name[i] == '_')) valid = false;
    }
    GMM_ASSERT1(!assert || valid, "Illegal variable name : " << name);
    return valid;
  }

  std::string model::new_name(const std::string &name) {
    std::string res_name = name;
    bool valid = check_name_valitity(res_name, false);
    VAR_SET::const_iterator it = variables.find(res_name);
    GMM_ASSERT1(valid || it != variables.end(),
		"Illegal variable name : " << name);
    for (size_type i = 2; it != variables.end(); ++i) {
      std::stringstream m;
      m << name << '_' << i;
      res_name = m.str();
      it = variables.find(res_name);
    }
    return res_name;
  }

  void model::actualize_sizes(void) const {
    act_size_to_be_done = false;
    std::map<std::string, std::vector<std::string> > multipliers;
    std::map<std::string, bool > tobedone;

    for (VAR_SET::iterator it = variables.begin(); it != variables.end();
	 ++it) {
      if (it->second.is_fem_dofs && it->second.filter == VDESCRFILTER_INFSUP) {
	VAR_SET::iterator it2 = variables.find(it->second.filter_var);
	GMM_ASSERT1(it2 != variables.end(), "The primal variable of the "
		    "multiplier does not exist");
	GMM_ASSERT1(it2->second.is_fem_dofs, "The primal variable of the "
		    "multiplier is not a fem variable");
	multipliers[it->second.filter_var].push_back(it->first);
	if (it->second.v_num < it->second.mf->version_number() ||
	    it->second.v_num < it2->second.mf->version_number())
	  tobedone[it->second.filter_var] = true;
      }
    }

    for (VAR_SET::iterator it = variables.begin(); it != variables.end();
	  ++it) {
      if (it->second.is_fem_dofs) {
	switch (it->second.filter) {
	case VDESCRFILTER_NO:
	  if (it->second.v_num < it->second.mf->version_number()) {
	    size_type s = it->second.mf->nb_dof();
	    if (!it->second.is_variable) s *= it->second.qdim;
	    it->second.set_size(s);
	    it->second.v_num = act_counter();
	  }
	  break;
	case VDESCRFILTER_REGION: 
	  if (it->second.v_num < it->second.mf->version_number()) {
	    dal::bit_vector dor
	      = it->second.mf->dof_on_region(it->second.m_region);
	    it->second.partial_mf->adapt(dor);
	    it->second.set_size(it->second.partial_mf->nb_dof());
	    it->second.v_num = act_counter();
	  }
	  break;
	default : break;
	}
      }
    }

    for (std::map<std::string, bool >::iterator itbd = tobedone.begin();
	 itbd != tobedone.end(); ++itbd) {
      std::vector<std::string> &mults = multipliers[itbd->first];
      VAR_SET::iterator it2 = variables.find(itbd->first);

      gmm::col_matrix< gmm::rsvector<scalar_type> > MGLOB;
      if (mults.size() > 1) {
	size_type s = 0;
	for (size_type k = 0; k < mults.size(); ++k) {
	  VAR_SET::iterator it = variables.find(mults[k]);
	  s += it->second.mf->nb_dof();
	}
	gmm::resize(MGLOB, it2->second.mf->nb_dof(), s);
      }

      size_type s = 0;
      std::set<size_type> glob_columns;
      for (size_type k = 0; k < mults.size(); ++k) {
	VAR_SET::iterator it = variables.find(mults[k]);

	// This step forces the recomputation of corresponding bricks.
	// A test to check if a modification is really necessary could
	// be done first ... (difficult to coordinate with other multipliers)
	dal::bit_vector alldof; alldof.add(0, it->second.mf->nb_dof());
	it->second.partial_mf->adapt(alldof);
	it->second.set_size(it->second.partial_mf->nb_dof());

	// Obtening the coupling matrix between the multipier and
	// the primal variable. A search is done on all the terms of the
	// model. The corresponding terms are added. If no term is available
	// a warning is printed and the variable is cancelled.

	gmm::col_matrix< gmm::rsvector<scalar_type> >
	  MM(it2->second.mf->nb_dof(), it->second.mf->nb_dof());
	bool termadded = false;

	for (size_type ib = 0; ib < bricks.size(); ++ib) {
	  const brick_description &brick = bricks[ib];
	  bool bupd = false;
	  bool cplx = is_complex() && brick.pbr->is_complex();

	  for (size_type j = 0; j < brick.tlist.size(); ++j) {

	    const term_description &term = brick.tlist[j];
	    
	    if (term.is_matrix_term && !mults[k].compare(term.var1) &&
		!it2->first.compare(term.var2)) {
	      if (!bupd) {
		brick.terms_to_be_computed = true;
		update_brick(ib, BUILD_MATRIX);
		bupd = true;
	      }
	      if (cplx)
		gmm::add(gmm::transposed(gmm::real_part(brick.cmatlist[j])),
			 MM);
	      else
		gmm::add(gmm::transposed(brick.rmatlist[j]), MM);
	      termadded = true;

	    } else if (term.is_matrix_term && !mults[k].compare(term.var2) &&
			!it2->first.compare(term.var1)) {
	      if (!bupd) {
		brick.terms_to_be_computed = true;
		update_brick(ib, BUILD_MATRIX);
		bupd = true;
	      }
	      if (cplx)
		gmm::add(gmm::real_part(brick.cmatlist[j]), MM);
	      else
		gmm::add(brick.rmatlist[j], MM);
	      termadded = true;
	    }
	  }
	}
	
	if (!termadded)
	  GMM_WARNING1("No term present to filter the multiplier " << mults[k]
		       << ". The multiplier is cancelled.");

	//
	// filtering
	//
	std::set<size_type> columns;
	gmm::range_basis(MM, columns);
	if (mults.size() > 1) {
	  gmm::copy(MM, gmm::sub_matrix
		    (MGLOB,gmm::sub_interval(0, it2->second.mf->nb_dof()),
		     gmm::sub_interval(s, it->second.mf->nb_dof())));
	  for (std::set<size_type>::iterator itt = columns.begin();
	     itt != columns.end(); ++itt)
	    glob_columns.insert(s + *itt);
	  s += it->second.mf->nb_dof();
	} else {
	  dal::bit_vector kept;
	  for (std::set<size_type>::iterator itt = columns.begin();
	       itt != columns.end(); ++itt)
	    kept.add(*itt);
	  it->second.partial_mf->adapt(kept);
	  it->second.set_size(it->second.partial_mf->nb_dof());
	  it->second.v_num = act_counter();
	}
      }

      if (mults.size() > 1) {
	range_basis(MGLOB, glob_columns, 1E-12, gmm::col_major(), true);

	s = 0;
	for (size_type k = 0; k < mults.size(); ++k) {
	  VAR_SET::iterator it = variables.find(mults[k]);
	  dal::bit_vector kept;
	  size_type nbdof = it->second.mf->nb_dof();
	  for (std::set<size_type>::iterator itt = glob_columns.begin();
	       itt != glob_columns.end(); ++itt)
	    if (*itt > s && *itt < s + nbdof) kept.add(*itt-s);
	  it->second.partial_mf->adapt(kept);
	  it->second.set_size(it->second.partial_mf->nb_dof());
	  it->second.v_num = act_counter();
	  s += it->second.mf->nb_dof();
	}
      }
    }

    size_type tot_size = 0;

    for (VAR_SET::iterator it = variables.begin(); it != variables.end();
	 ++it)
      if (it->second.is_variable) {
	it->second.I = gmm::sub_interval(tot_size, it->second.size());
	tot_size += it->second.size();
      }
      
    if (complex_version) {
      gmm::resize(cTM, tot_size, tot_size);
      gmm::resize(crhs, tot_size);
    }
    else {
      gmm::resize(rTM, tot_size, tot_size);
      gmm::resize(rrhs, tot_size);
    }
  }
   

  void model::listvar(std::ostream &ost) const {
    if (variables.size() == 0)
      ost << "Model with no variable nor data" << endl;
    else {
      ost << "List of model variables and data:" << endl;
      for (VAR_SET::const_iterator it = variables.begin();
	   it != variables.end(); ++it) {
	if (it->second.is_variable) ost << "Variable ";
	else ost << "Data     ";
	ost << std::setw(20) << std::left << it->first;
	if (it->second.n_iter == 1)
	  ost << " 1 copy   ";
	else 
	  ost << std::setw(2) << std::right << it->second.n_iter
	      << " copies ";
	if (it->second.is_fem_dofs) ost << "fem dependant ";
	else ost << "constant size ";
	size_type d = sizeof(scalar_type);
	if (is_complex()) d *= 2;
	ost << std::setw(8) << std::right << it->second.size() * d
	    << " bytes.";
	ost << endl;
      }
    }
  }

  void model::add_fixed_size_variable(const std::string &name, size_type size,
				      size_type niter) {
    check_name_valitity(name);
    variables[name] = var_description(true, is_complex(), false, niter);
    act_size_to_be_done = true;
    variables[name].set_size(size);
  }
  
  void model::add_fixed_size_data(const std::string &name, size_type size,
				      size_type niter) {
    check_name_valitity(name);
    variables[name] = var_description(false, is_complex(), false, niter);
    variables[name].set_size(size);
  }

  void model::add_fem_variable(const std::string &name, const mesh_fem &mf,
			       size_type niter) {
    check_name_valitity(name);
    variables[name] = var_description(true, is_complex(), true, niter,
				      VDESCRFILTER_NO, &mf);
    variables[name].set_size(mf.nb_dof());
    add_dependency(mf);
    act_size_to_be_done = true;
    leading_dim = std::max(leading_dim, mf.linked_mesh().dim());
  }
  
  void model::add_fem_data(const std::string &name, const mesh_fem &mf,
			       dim_type qdim, size_type niter) {
    check_name_valitity(name);
    variables[name] = var_description(false, is_complex(), true, niter,
				      VDESCRFILTER_NO, &mf, 0, qdim);
    variables[name].set_size(mf.nb_dof()*qdim);
    add_dependency(mf); 
  }

  void model::add_multiplier(const std::string &name, const mesh_fem &mf,
			     const std::string &primal_name,
			     size_type niter) {
    check_name_valitity(name);
    variables[name] = var_description(true, is_complex(), true, niter,
				      VDESCRFILTER_INFSUP, &mf, 0,
				      1, primal_name);
    variables[name].set_size(mf.nb_dof());
    act_size_to_be_done = true;
    add_dependency(mf);
  }

  size_type model::add_brick(pbrick pbr, const varnamelist &varnames,
			     const varnamelist &datanames,
			     const termlist &terms,
			     const mimlist &mims, size_type region) {
    bricks.push_back(brick_description(pbr, varnames, datanames, terms,
				       mims, region));
    for  (size_type i = 0; i < bricks.back().mims.size(); ++i)
      add_dependency(*(bricks.back().mims[i]));

    GMM_ASSERT1(pbr->is_real() || is_complex(),
		"Impossible to add a complex brick to a real model");
    if (is_complex() && pbr->is_complex()) {
      bricks.back().cmatlist.resize(terms.size());
      bricks.back().cveclist.resize(terms.size());
    } else {
      bricks.back().rmatlist.resize(terms.size());
      bricks.back().rveclist.resize(terms.size());
    }
    is_linear_ = is_linear_ && pbr->is_linear();
    is_symmetric_ = is_symmetric_ && pbr->is_symmetric();
    is_coercive_ = is_coercive_ && pbr->is_coercive();

    for (size_type i=0; i < varnames.size(); ++i)
      GMM_ASSERT1(variables.find(varnames[i]) != variables.end(),
		  "Undefined model variable " << varnames[i]);
    for (size_type i=0; i < datanames.size(); ++i)
      GMM_ASSERT1(variables.find(datanames[i]) != variables.end(),
		  "Undefined model data or variable " << datanames[i]);
    
    return size_type(bricks.size() - 1);
  }

  const std::string &model::varname_of_brick(size_type ind_brick,
				      size_type ind_var) {
    GMM_ASSERT1(ind_brick < bricks.size(), "Inexistent brick");
    GMM_ASSERT1(ind_var < bricks[ind_brick].vlist.size(),
	       "Inexistent brick variable");
    return bricks[ind_brick].vlist[ind_var];
  }
  
  const std::string &model::dataname_of_brick(size_type ind_brick,
					      size_type ind_data) {
    GMM_ASSERT1(ind_brick < bricks.size(), "Inexistent brick");
    GMM_ASSERT1(ind_data < bricks[ind_brick].dlist.size(),
		"Inexistent brick data");
    return bricks[ind_brick].dlist[ind_data];
  }

  void model::listbricks(std::ostream &ost) const {
    if (bricks.size() == 0)
      ost << "Model with no bricks" << endl;
    else {
      ost << "List of model bricks:" << endl;
      for (size_type i = 0; i < bricks.size(); ++i) {
	ost << "Brick " << std::setw(3) << std::right << i
	    << " " << std::setw(20) << std::right
	    << bricks[i].pbr->brick_name() << endl;
	ost << "  concerned variables: " << bricks[i].vlist[0];
	for (size_type j = 1; j < bricks[i].vlist.size(); ++j)
	  ost << ", " << bricks[i].vlist[j];
	ost << "." << endl;
	ost << "  brick with " << bricks[i].tlist.size() << " term";
	if (bricks[i].tlist.size() > 1) ost << "s";
	ost << endl;
	// + lister les termes
      }
    }
  }

  // Call the brick to compute the terms
  void model::update_brick(size_type ib, assembly_version version) const {
    const brick_description &brick = bricks[ib];
    bool cplx = is_complex() && brick.pbr->is_complex();
    bool tobecomputed = brick.terms_to_be_computed
      || !(brick.pbr->is_linear());
    bool dispatchcall = tobecomputed;
    
    // check variable list to test if a mesh_fem as changed. 
    for (size_type i = 0; i < brick.vlist.size() && !tobecomputed; ++i) {
      var_description &vd = variables[brick.vlist[i]];
      if (vd.v_num > brick.v_num) dispatchcall = tobecomputed = true;
      if (vd.v_num_data > brick.v_num) dispatchcall = true;
    }
    
    // check data list to test if a vector value of a data has changed. 
    for (size_type i = 0; i < brick.dlist.size() && !tobecomputed; ++i) {
      var_description &vd = variables[brick.dlist[i]];
      if (vd.v_num > brick.v_num || vd.v_num_data > brick.v_num)
	dispatchcall = tobecomputed = true;
    }

    if (tobecomputed) {
      // Initialization of vector and matrices.
      for (size_type j = 0; j < brick.tlist.size(); ++j) {
	const term_description &term = brick.tlist[j];
	size_type nbd1 = variables[term.var1].size();
	size_type nbd2 = term.is_matrix_term ?
	  variables[term.var2].size() : 0;
	if (term.is_matrix_term &&
	    (brick.pbr->is_linear() || (version | BUILD_MATRIX))) {
	  if (cplx)
	    brick.cmatlist[j] = model_complex_sparse_matrix(nbd1, nbd2);
	  else
	    brick.rmatlist[j] = model_real_sparse_matrix(nbd1, nbd2);
	}
	if (brick.pbr->is_linear() || (version | BUILD_RHS)) {
	  if (cplx) {
	    gmm::clear(brick.cveclist[j]);
	    gmm::resize(brick.cveclist[j], nbd1);
	  } else {
	    gmm::clear(brick.rveclist[j]);
	    gmm::resize(brick.rveclist[j], nbd1);
	  }
	}
      }
      
      // Brick call for all terms.
      if (brick.pbr->is_linear() || !(brick.pdispatch)) {
	if (cplx)
	  brick.pbr->asm_complex_tangent_terms(*this, brick.vlist, brick.dlist,
					       brick.mims,
					       brick.cmatlist, brick.cveclist,
					       brick.region, version);
	else
	  brick.pbr->asm_real_tangent_terms(*this, brick.vlist, brick.dlist,
					    brick.mims,
					    brick.rmatlist, brick.rveclist,
					    brick.region, version);
      }
      brick.v_num = act_counter();
    }
      
    if (dispatchcall && brick.pdispatch) {
      
      if (cplx) 
	brick.pdispatch->asm_complex_tangent_terms(*this, brick.pbr,
						   brick.vlist, brick.dlist,
						   brick.mims,
						   brick.cmatlist,
						   brick.cveclist,
						   brick.region, version);
      
      else
	brick.pdispatch->asm_real_tangent_terms(*this, brick.pbr,
						brick.vlist, brick.dlist,
						brick.mims,
						brick.rmatlist,
						  brick.rveclist,
						brick.region, version);
      brick.v_num = act_counter();
    }

    if (brick.pbr->is_linear()) brick.terms_to_be_computed = false;
  }


  void model::assembly(assembly_version version) {

    context_check(); if (act_size_to_be_done) actualize_sizes();
    if (is_complex()) { gmm::clear(cTM); gmm::clear(crhs); }
    else { gmm::clear(rTM); gmm::clear(rrhs); }

    // � mettre aussi � la fin avec la suppression des variables temporaires ?
    for (VAR_SET::iterator it=variables.begin(); it != variables.end(); ++it)
      it->second.clear_temporaries();

    for (size_type ib = 0; ib < bricks.size(); ++ib) {
      brick_description &brick = bricks[ib];

      update_brick(ib, version);

      bool cplx = is_complex() && brick.pbr->is_complex();
      
      // Assembly of terms
      for (size_type j = 0; j < brick.tlist.size(); ++j) {
	term_description &term = brick.tlist[j];
	gmm::sub_interval I1 = variables[term.var1].I;
	gmm::sub_interval I2(0,0);
	if (term.is_matrix_term) I2 = variables[term.var2].I;

	if (cplx) {
	  if (term.is_matrix_term && (version | BUILD_MATRIX)) {
	    gmm::add(brick.cmatlist[j], gmm::sub_matrix(cTM, I1, I2));
	    if (brick.pbr->is_linear() && !is_linear()
		&& (version | BUILD_RHS)) {
	      gmm::mult(brick.cmatlist[j],
			gmm::scaled(variables[term.var1].complex_value[0],
				    std::complex<scalar_type>(-1)),
			gmm::sub_vector(crhs, I1));
	    }
	    if (term.is_symmetric && I1.first() != I2.first()) {
	      gmm::add(gmm::transposed(brick.cmatlist[j]),
		       gmm::sub_matrix(cTM, I2, I1));
	      if (brick.pbr->is_linear() && !is_linear()
		  && (version | BUILD_RHS)) {
		gmm::mult(gmm::conjugated(brick.cmatlist[j]),
			  gmm::scaled(variables[term.var2].complex_value[0],
				      std::complex<scalar_type>(-1)),
			  gmm::sub_vector(crhs, I2));
	      }
	    }
	  }
	  if (version | BUILD_RHS)
	    gmm::add(brick.cveclist[j], gmm::sub_vector(crhs, I1));
	} else if (is_complex()) {
	  if (term.is_matrix_term && (version | BUILD_MATRIX)) {
	    gmm::add(brick.rmatlist[j], gmm::sub_matrix(cTM, I1, I2));
	    if (brick.pbr->is_linear() && !is_linear()
		&& (version | BUILD_RHS)) {
	      gmm::mult(brick.rmatlist[j],
			gmm::scaled(variables[term.var1].real_value[0],
				    scalar_type(-1)),
			gmm::sub_vector(crhs, I1));
	    }
	    if (term.is_symmetric && I1.first() != I2.first()) {
	      gmm::add(gmm::transposed(brick.rmatlist[j]),
		       gmm::sub_matrix(cTM, I2, I1));
	      if (brick.pbr->is_linear() && !is_linear()
		  && (version | BUILD_RHS)) {
		gmm::mult(gmm::transposed(brick.rmatlist[j]),
			  gmm::scaled(variables[term.var2].real_value[0],
				      scalar_type(-1)),
			  gmm::sub_vector(crhs, I2));
	      }
	    }
	  }
	  if (version | BUILD_RHS)
	    gmm::add(brick.rveclist[j], gmm::sub_vector(crhs, I1));
	} else {
	  if (term.is_matrix_term && (version | BUILD_MATRIX)) {
	    gmm::add(brick.rmatlist[j], gmm::sub_matrix(rTM, I1, I2));
	    if (brick.pbr->is_linear() && !is_linear()
		&& (version | BUILD_RHS)) {
	      gmm::mult(brick.rmatlist[j],
			gmm::scaled(variables[term.var1].real_value[0],
				    scalar_type(-1)),
			gmm::sub_vector(rrhs, I1));
	    }
	    if (term.is_symmetric && I1.first() != I2.first()) {
	      gmm::add(gmm::transposed(brick.rmatlist[j]),
		       gmm::sub_matrix(rTM, I2, I1));
	      if (brick.pbr->is_linear() && !is_linear()
		  && (version | BUILD_RHS)) {
		gmm::mult(gmm::transposed(brick.rmatlist[j]),
			  gmm::scaled(variables[term.var2].real_value[0],
				      scalar_type(-1)),
			  gmm::sub_vector(rrhs, I2));
	      }
	    }
	  }
	  if (version | BUILD_RHS)
	    gmm::add(brick.rveclist[j], gmm::sub_vector(rrhs, I1));
	}
      }


      if (brick.pbr->is_linear())
	brick.terms_to_be_computed = false;
      else 
	if (cplx) {
	  brick.cmatlist = complex_matlist(brick.tlist.size());
	  brick.cveclist = complex_veclist(brick.tlist.size());
	} else {
	  brick.rmatlist = real_matlist(brick.tlist.size());
	  brick.rveclist = real_veclist(brick.tlist.size());	    
	}

    }
  }

  const mesh_fem &model::mesh_fem_of_variable(const std::string &name) const {
    VAR_SET::const_iterator it = variables.find(name);
    GMM_ASSERT1(it!=variables.end(), "Undefined variable " << name);
    return it->second.associated_mf();
  }
  
  const mesh_fem *model::pmesh_fem_of_variable(const std::string &name) const {
    VAR_SET::const_iterator it = variables.find(name);
    GMM_ASSERT1(it!=variables.end(), "Undefined variable " << name);
    return it->second.passociated_mf();
  }
  
  const model_real_plain_vector &
  model::real_variable(const std::string &name, size_type niter) const {
    GMM_ASSERT1(!complex_version, "This model is a complex one");
    context_check(); if (act_size_to_be_done) actualize_sizes();
    VAR_SET::const_iterator it = variables.find(name);
    GMM_ASSERT1(it!=variables.end(), "Undefined variable " << name);
    if (niter == size_type(-1)) niter = it->second.default_iter;
    GMM_ASSERT1(it->second.n_iter > niter, "Unvalid iteration number "
		<< niter);
    return it->second.real_value[niter];
  }
  
  const model_complex_plain_vector &
  model::complex_variable(const std::string &name, size_type niter) const {
    GMM_ASSERT1(complex_version, "This model is a real one");
    context_check(); if (act_size_to_be_done) actualize_sizes();
    VAR_SET::const_iterator it = variables.find(name);
    GMM_ASSERT1(it!=variables.end(), "Undefined variable " << name);
    if (niter == size_type(-1)) niter = it->second.default_iter;
    GMM_ASSERT1(it->second.n_iter > niter, "Unvalid iteration number "
		<< niter);
    return it->second.complex_value[niter];    
  }

  model_real_plain_vector &
  model::set_real_variable(const std::string &name, size_type niter) {
    GMM_ASSERT1(!complex_version, "This model is a complex one");
    context_check(); if (act_size_to_be_done) actualize_sizes();
    VAR_SET::iterator it = variables.find(name);
    GMM_ASSERT1(it!=variables.end(), "Undefined variable " << name);
    it->second.v_num_data = act_counter();
    if (niter == size_type(-1)) niter = it->second.default_iter;
    GMM_ASSERT1(it->second.n_iter > niter, "Unvalid iteration number "
		<< niter);
    return it->second.real_value[niter];
  }
  
  model_complex_plain_vector &
  model::set_complex_variable(const std::string &name, size_type niter) {
    GMM_ASSERT1(complex_version, "This model is a real one");
    context_check(); if (act_size_to_be_done) actualize_sizes();
    VAR_SET::iterator it = variables.find(name);
    GMM_ASSERT1(it!=variables.end(), "Undefined variable " << name);
    it->second.v_num_data = act_counter();    
    if (niter == size_type(-1)) niter = it->second.default_iter;
    GMM_ASSERT1(it->second.n_iter > niter, "Unvalid iteration number "
		<< niter);
    return it->second.complex_value[niter];    
  }


  // ----------------------------------------------------------------------
  //
  //
  // Standard bricks
  //
  //
  // ----------------------------------------------------------------------


  // ----------------------------------------------------------------------
  //
  // Generic elliptic brick
  //
  // ----------------------------------------------------------------------

  struct generic_elliptic_brick : public virtual_brick {

    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Generic elliptic brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Generic elliptic brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() <= 1,
		  "Wrong number of variables for generic elliptic brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      size_type N = m.dim(), Q = mf_u.get_qdim(), s = 1;
      const mesh_im &mim = *mims[0];
      const model_real_plain_vector *A = 0;
      const mesh_fem *mf_a = 0;
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      if (dl.size() > 0) {
	A = &(md.real_variable(dl[0]));
	mf_a = md.pmesh_fem_of_variable(dl[0]);
	s = gmm::vect_size(*A);
	if (mf_a) s = s * mf_a->get_qdim() / mf_a->nb_dof();
      }

      if (s == 1) {
	if (mf_a) {
	  if (Q > 1)
	    asm_stiffness_matrix_for_laplacian_componentwise
	      (matl[0], mim, mf_u, *mf_a, *A, rg);
	  else
	    asm_stiffness_matrix_for_laplacian
	      (matl[0], mim, mf_u, *mf_a, *A, rg);

	} else {
	  if (Q > 1)
	    asm_stiffness_matrix_for_homogeneous_laplacian_componentwise
	      (matl[0], mim, mf_u, rg);
	  else
	    asm_stiffness_matrix_for_homogeneous_laplacian
	      (matl[0], mim, mf_u, rg);
	  if (A) gmm::scale(matl[0], (*A)[0]);
	}
      } else if (s == N*N) {
	if (mf_a) {
	  if (Q > 1)
	    asm_stiffness_matrix_for_scalar_elliptic_componentwise
	      (matl[0], mim, mf_u, *mf_a, *A, rg);
	  else
	    asm_stiffness_matrix_for_scalar_elliptic
	      (matl[0], mim, mf_u, *mf_a, *A, rg);
	} else {
	  if (Q > 1)
	    asm_stiffness_matrix_for_homogeneous_scalar_elliptic_componentwise
	      (matl[0], mim, mf_u, *A, rg);
	  else
	    asm_stiffness_matrix_for_homogeneous_scalar_elliptic
	      (matl[0], mim, mf_u, *A, rg);
	}
      } else if (s == N*N*Q*Q) {
	if (mf_a)
	  asm_stiffness_matrix_for_vector_elliptic
	    (matl[0], mim, mf_u, *mf_a, *A, rg);
	else 
	  asm_stiffness_matrix_for_homogeneous_vector_elliptic
	    (matl[0], mim, mf_u, *A, rg);
      } else
	GMM_ASSERT1(false,
		    "Bad format generic elliptic brick coefficient");
    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &,
					   size_type region,
					   nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Generic elliptic brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Generic elliptic brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() <= 1,
		  "Wrong number of variables for generic elliptic brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      size_type N = m.dim(), Q = mf_u.get_qdim(), s = 1;
      const mesh_im &mim = *mims[0];
      const model_real_plain_vector *A = 0;
      const mesh_fem *mf_a = 0;
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      

      if (dl.size() > 0) {
	A = &(md.real_variable(dl[0]));
	mf_a = md.pmesh_fem_of_variable(dl[0]);
	s = gmm::vect_size(*A);
	if (mf_a) s = s * mf_a->get_qdim() / mf_a->nb_dof();
      }

      if (s == 1) {
	if (mf_a) {
	  if (Q > 1)
	    asm_stiffness_matrix_for_laplacian_componentwise
	      (matl[0], mim, mf_u, *mf_a, *A, rg);
	  else
	    asm_stiffness_matrix_for_laplacian
	      (matl[0], mim, mf_u, *mf_a, *A, rg);

	} else {
	  if (Q > 1)
	    asm_stiffness_matrix_for_homogeneous_laplacian_componentwise
	      (gmm::real_part(matl[0]), mim, mf_u, rg);
	  else
	    asm_stiffness_matrix_for_homogeneous_laplacian
	      (gmm::real_part(matl[0]), mim, mf_u, rg);
	  if (A) gmm::scale(matl[0], (*A)[0]);
	}
      } else if (s == N*N) {
	if (mf_a) {
	  if (Q > 1)
	    asm_stiffness_matrix_for_scalar_elliptic_componentwise
	      (matl[0], mim, mf_u, *mf_a, *A, rg);
	  else
	    asm_stiffness_matrix_for_scalar_elliptic
	      (matl[0], mim, mf_u, *mf_a, *A, rg);
	} else {
	  if (Q > 1)
	    asm_stiffness_matrix_for_homogeneous_scalar_elliptic_componentwise
	      (matl[0], mim, mf_u, *A, rg);
	  else
	    asm_stiffness_matrix_for_homogeneous_scalar_elliptic
	      (matl[0], mim, mf_u, *A, rg);
	}
      } else if (s == N*N*Q*Q) {
	if (mf_a)
	  asm_stiffness_matrix_for_vector_elliptic
	    (matl[0], mim, mf_u, *mf_a, *A, rg);
	else 
	  asm_stiffness_matrix_for_homogeneous_vector_elliptic
	    (matl[0], mim, mf_u, *A, rg);
      } else
	GMM_ASSERT1(false,
		    "Bad format generic elliptic brick coefficient");
    }

    generic_elliptic_brick(void) {
      set_flags("Generic elliptic", true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, true /* is complex */);
    }

  };

  size_type add_Laplacian_brick(model &md, const mesh_im &mim,
				const std::string &varname,
				size_type region) {
    pbrick pbr = new generic_elliptic_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    return md.add_brick(pbr, model::varnamelist(1, varname),
			model::varnamelist(), tl, model::mimlist(1, &mim),
			region);
  }

  size_type add_generic_elliptic_brick(model &md, const mesh_im &mim,
				       const std::string &varname,
				       const std::string &dataname,
				       size_type region) {
    pbrick pbr = new generic_elliptic_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    return md.add_brick(pbr, model::varnamelist(1, varname),
			model::varnamelist(1, dataname), tl,
			model::mimlist(1, &mim), region);
  }

  // ----------------------------------------------------------------------
  //
  // Source term brick
  //
  // ----------------------------------------------------------------------

  struct source_term_brick : public virtual_brick {

    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &,
					model::real_veclist &vecl,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1,
		  "Source term brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Source term brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() > 0 && dl.size() <= 2,
		  "Wrong number of variables for source term brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh_im &mim = *mims[0];
      const model_real_plain_vector &A = md.real_variable(dl[0]);
      const mesh_fem *mf_data = md.pmesh_fem_of_variable(dl[0]);
      mesh_region rg(region);
      mim.linked_mesh().intersect_with_mpi_region(rg);

      size_type s = gmm::vect_size(A);
      if (mf_data) s = s * mf_data->get_qdim() / mf_data->nb_dof();

      GMM_ASSERT1(mf_u.get_qdim() == s,
		  dl[0] << ": bad format of source term data. "
		  "Detected dimension is " << s << " should be "
		  << size_type(mf_u.get_qdim()));

      if (mf_data)
	asm_source_term(vecl[0], mim, mf_u, *mf_data, A, rg);
      else
	asm_homogeneous_source_term(vecl[0], mim, mf_u, A, rg);

      if (dl.size() > 1) gmm::add(md.real_variable(dl[1]), vecl[0]);

    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &,
					   model::complex_veclist &vecl,
					   size_type region,
					   nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1,
		  "Source term brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Source term brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() > 0 && dl.size() <= 2,
		  "Wrong number of variables for source term brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh_im &mim = *mims[0];
      const model_complex_plain_vector &A = md.complex_variable(dl[0]);
      const mesh_fem *mf_data = md.pmesh_fem_of_variable(dl[0]);
      mesh_region rg(region);
      mim.linked_mesh().intersect_with_mpi_region(rg);

      size_type s = gmm::vect_size(A);
      if (mf_data) s = s * mf_data->get_qdim() / mf_data->nb_dof();

      GMM_ASSERT1(mf_u.get_qdim() == s, "Bad format of source term data");

      if (mf_data)
	asm_source_term(vecl[0], mim, mf_u, *mf_data, A, rg);
      else
	asm_homogeneous_source_term(vecl[0], mim, mf_u, A, rg);

      if (dl.size() > 1) gmm::add(md.complex_variable(dl[1]), vecl[0]);

    }

    source_term_brick(void) {
      set_flags("Source term", true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, true /* is complex */);
    }


  };

  size_type add_source_term_brick(model &md, const mesh_im &mim,
				  const std::string &varname,
				  const std::string &dataname,
				  size_type region,
				  const std::string &directdataname) {
    pbrick pbr = new source_term_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname));
    model::varnamelist vdata(1, dataname);
    if (directdataname.size()) vdata.push_back(directdataname);
    return md.add_brick(pbr, model::varnamelist(1, varname),
			vdata, tl, model::mimlist(1, &mim), region);
  }

  // ----------------------------------------------------------------------
  //
  // Normal source term brick
  //
  // ----------------------------------------------------------------------

  struct normal_source_term_brick : public virtual_brick {

    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &,
					model::real_veclist &vecl,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1,
		  "Source term brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Source term brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 1,
		  "Wrong number of variables for source term brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh_im &mim = *mims[0];
      const model_real_plain_vector &A = md.real_variable(dl[0]);
      const mesh_fem *mf_data = md.pmesh_fem_of_variable(dl[0]);
      mesh_region rg(region);
      mim.linked_mesh().intersect_with_mpi_region(rg);

      size_type s = gmm::vect_size(A), N = mf_u.linked_mesh().dim();
      if (mf_data) s = s * mf_data->get_qdim() / mf_data->nb_dof();

      GMM_ASSERT1(mf_u.get_qdim()*N == s,
		  dl[0] << ": bad format of normal source term data. "
		  "Detected dimension is " << s << " should be "
		  << size_type(mf_u.get_qdim()*N));

      if (mf_data)
	asm_normal_source_term(vecl[0], mim, mf_u, *mf_data, A, rg);
      else
	asm_homogeneous_normal_source_term(vecl[0], mim, mf_u, A, rg);

    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &,
					   model::complex_veclist &vecl,
					   size_type region,
					   nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1,
		  "Source term brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Source term brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 1,
		  "Wrong number of variables for source term brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh_im &mim = *mims[0];
      const model_complex_plain_vector &A = md.complex_variable(dl[0]);
      const mesh_fem *mf_data = md.pmesh_fem_of_variable(dl[0]);
      mesh_region rg(region);
      mim.linked_mesh().intersect_with_mpi_region(rg);

      size_type s = gmm::vect_size(A), N = mf_u.linked_mesh().dim();
      if (mf_data) s = s * mf_data->get_qdim() / mf_data->nb_dof();

      GMM_ASSERT1(s == mf_u.get_qdim()*N, "Bad format of source term data");

      if (mf_data)
	asm_normal_source_term(vecl[0], mim, mf_u, *mf_data, A, rg);
      else
	asm_homogeneous_normal_source_term(vecl[0], mim, mf_u, A, rg);

    }

    normal_source_term_brick(void) {
      set_flags("Normal source term", true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, true /* is complex */);
    }


  };

  size_type add_normal_source_term_brick(model &md, const mesh_im &mim,
					 const std::string &varname,
					 const std::string &dataname,
					 size_type region) {
    pbrick pbr = new normal_source_term_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname));
    model::varnamelist vdata(1, dataname);
    return md.add_brick(pbr, model::varnamelist(1, varname),
			vdata, tl, model::mimlist(1, &mim), region);
  }


  // ----------------------------------------------------------------------
  //
  // Dirichlet condition brick
  //
  // ----------------------------------------------------------------------
  // Two variables : with multipliers
  // One variable : penalization

  struct Dirichlet_condition_brick : public virtual_brick {
    
    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &vecl,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Dirichlet condition brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Dirichlet condition brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() >= 1 && vl.size() <= 2 && dl.size() <= 2,
		  "Wrong number of variables for Dirichlet condition brick");

      bool penalized = (vl.size() == 1);
      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh_fem &mf_mult = md.mesh_fem_of_variable(vl[vl.size()-1]);
      const mesh_im &mim = *mims[0];
      const model_real_plain_vector *A = 0, *COEFF = 0;
      const mesh_fem *mf_data = 0;

      if (penalized) {
	COEFF = &(md.real_variable(dl[0]));
	GMM_ASSERT1(gmm::vect_size(*COEFF) == 1,
		    "Data for coefficient should be a scalar");
      }

      size_type s = 0, ind = (penalized ? 1 : 0);
      if (dl.size() > ind) {
	A = &(md.real_variable(dl[ind]));
	mf_data = md.pmesh_fem_of_variable(dl[ind]);
	s = gmm::vect_size(*A);
	if (mf_data) s = s * mf_data->get_qdim() / mf_data->nb_dof();
	GMM_ASSERT1(mf_u.get_qdim() == s,
		    dl[ind] << ": bad format of Dirichlet data. "
		    "Detected dimension is " << s << " should be "
		    << size_type(mf_u.get_qdim()));
      }
      mesh_region rg(region);
      mim.linked_mesh().intersect_with_mpi_region(rg);

      if (dl.size()) {
	if (mf_data)
	  asm_source_term(vecl[0], mim, mf_mult, *mf_data, *A, rg);
	else
	  asm_homogeneous_source_term(vecl[0], mim, mf_mult, *A, rg);
	if (penalized) gmm::scale(vecl[0], gmm::abs((*COEFF)[0]));
      }

      asm_mass_matrix(matl[0], mim, mf_mult, mf_u, region);
      if (penalized) gmm::scale(matl[0], gmm::abs((*COEFF)[0]));
    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &vecl,
					   size_type region,
					   nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Dirichlet condition brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Dirichlet condition brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() >= 1 && vl.size() <= 2 && dl.size() <= 2,
		  "Wrong number of variables for Dirichlet condition brick");

      bool penalized = (vl.size() == 1);
      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh_fem &mf_mult = md.mesh_fem_of_variable(vl[vl.size()-1]);
      const mesh_im &mim = *mims[0];
      const model_complex_plain_vector *A = 0, *COEFF = 0;
      const mesh_fem *mf_data = 0;

      if (penalized) {
	COEFF = &(md.complex_variable(dl[0]));
	GMM_ASSERT1(gmm::vect_size(*COEFF) == 1,
		    "Data for coefficient should be a scalar");
      }

      size_type s = 0, ind = (penalized ? 1 : 0);
      if (dl.size() > ind) {
	A = &(md.complex_variable(dl[ind]));
	mf_data = md.pmesh_fem_of_variable(dl[ind]);
	s = gmm::vect_size(*A);
	if (mf_data) s = s * mf_data->get_qdim() / mf_data->nb_dof();
	GMM_ASSERT1(mf_u.get_qdim() == s, "Bad format of Dirichlet data");
      }
      mesh_region rg(region);
      mim.linked_mesh().intersect_with_mpi_region(rg);

      if (dl.size() > ind) {
	if (mf_data)
	  asm_source_term(vecl[0], mim, mf_mult, *mf_data, *A, rg);
	else
	  asm_homogeneous_source_term(vecl[0], mim, mf_mult, *A, rg);
	if (penalized) gmm::scale(vecl[0], gmm::abs((*COEFF)[0]));
      }

      asm_mass_matrix(matl[0], mim, mf_mult, mf_u, region);
      if (penalized) gmm::scale(matl[0], gmm::abs((*COEFF)[0]));
    }

    Dirichlet_condition_brick(bool penalized) {
      set_flags(penalized ? "Dirichlet with penalization brick"
		          : "Dirichlet with multipliers brick",
		true /* is linear*/,
		true /* is symmetric */, penalized /* is coercive */,
		true /* is real */, true /* is complex */);
    }


  };

  size_type add_Dirichlet_condition_with_multipliers
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &multname, size_type region,
   const std::string &dataname) {
    pbrick pbr = new Dirichlet_condition_brick(false);
    model::termlist tl;
    tl.push_back(model::term_description(multname, varname, true));
    model::varnamelist vl(1, varname);
    vl.push_back(multname);
    model::varnamelist dl;
    if (dataname.size()) dl.push_back(dataname);
    return md.add_brick(pbr, vl, dl, tl, model::mimlist(1, &mim), region);
  }

  size_type add_Dirichlet_condition_with_multipliers
  (model &md, const mesh_im &mim, const std::string &varname,
   const mesh_fem &mf_mult, size_type region,
   const std::string &dataname) {
    std::string multname = md.new_name("mult_on_" + varname);
    md.add_multiplier(multname, mf_mult, varname);
    return add_Dirichlet_condition_with_multipliers
      (md, mim, varname, multname, region, dataname);
  }

  size_type add_Dirichlet_condition_with_multipliers
  (model &md, const mesh_im &mim, const std::string &varname,
   dim_type degree, size_type region,
   const std::string &dataname) {
    const mesh_fem &mf_u = md.mesh_fem_of_variable(varname);
    const mesh_fem &mf_mult = classical_mesh_fem(mf_u.linked_mesh(),
						 degree, mf_u.get_qdim());
    return add_Dirichlet_condition_with_multipliers
      (md, mim, varname, mf_mult, region, dataname);
  }

  const std::string &mult_varname_Dirichlet(model &md, size_type ind_brick) {
    return md.varname_of_brick(ind_brick, 1);
  }

  size_type add_Dirichlet_condition_with_penalization
  (model &md, const mesh_im &mim, const std::string &varname,
   scalar_type penalisation_coeff, size_type region, 
   const std::string &dataname) {
    std::string coeffname = md.new_name("penalization_on_" + varname);
    md.add_fixed_size_data(coeffname, 1);
    if (md.is_complex())
      md.set_complex_variable(coeffname)[0] = penalisation_coeff;
    else
      md.set_real_variable(coeffname)[0] = penalisation_coeff;
    pbrick pbr = new Dirichlet_condition_brick(true);
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    model::varnamelist vl(1, varname);
    model::varnamelist dl(1, coeffname);
    if (dataname.size()) dl.push_back(dataname);
    return md.add_brick(pbr, vl, dl, tl, model::mimlist(1, &mim), region);
  }

  void change_penalization_coeff(model &md, size_type ind_brick,
				 scalar_type penalisation_coeff) {
    const std::string &coeffname = md.dataname_of_brick(ind_brick, 0);
    if (!md.is_complex()) {
      model_real_plain_vector &d = md.set_real_variable(coeffname);
      GMM_ASSERT1(gmm::vect_size(d)==1,
		  "Wrong coefficient size, may be not a Dirichlet brick "
		  "with penalization");
      d[0] = penalisation_coeff;
    }
    else {
      model_complex_plain_vector &d = md.set_complex_variable(coeffname);
      GMM_ASSERT1(gmm::vect_size(d)==1,
		  "Wrong coefficient size, may be not a Dirichlet brick "
		  "with penalization");
      d[0] = penalisation_coeff;
    }
  }



  // ----------------------------------------------------------------------
  //
  // Helmholtz brick
  //
  // ----------------------------------------------------------------------

  struct Helmholtz_brick : public virtual_brick {

    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Helmholtz brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Helmholtz brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 1,
		  "Wrong number of variables for Helmholtz brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      size_type Q = mf_u.get_qdim(), s = 1;
      GMM_ASSERT1(Q == 1, "Helmholtz brick is only for scalar field, sorry.");
      const mesh_im &mim = *mims[0];
      const mesh_fem *mf_a = 0;
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      const model_real_plain_vector *A = &(md.real_variable(dl[0]));
      mf_a = md.pmesh_fem_of_variable(dl[0]);
      s = gmm::vect_size(*A);
      if (mf_a) s = s * mf_a->get_qdim() / mf_a->nb_dof();

      if (s == 1) {
	model_real_plain_vector A2(gmm::vect_size(*A));
	for (size_type i=0; i < gmm::vect_size(*A); ++i) // Not valid for 
	  A2[i] = gmm::sqr((*A)[i]); // non lagrangian fem ...
	if (mf_a)
	  asm_Helmholtz(matl[0], mim, mf_u, *mf_a, A2, rg);
	else
	  asm_homogeneous_Helmholtz(matl[0], mim, mf_u, A2, rg);
      } else
	GMM_ASSERT1(false, "Bad format Helmholtz brick coefficient");
    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &,
					   size_type region,
					   nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Helmholtz brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Helmholtz brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 1,
		  "Wrong number of variables for Helmholtz brick");
      
      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      size_type Q = mf_u.get_qdim(), s = 1;
      GMM_ASSERT1(Q == 1, "Helmholtz brick is only for scalar field, sorry.");
      const mesh_im &mim = *mims[0];
      const mesh_fem *mf_a = 0;
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      const model_complex_plain_vector *A = &(md.complex_variable(dl[0]));
      mf_a = md.pmesh_fem_of_variable(dl[0]);
      s = gmm::vect_size(*A);
      if (mf_a) s = s * mf_a->get_qdim() / mf_a->nb_dof();

      if (s == 1) {
	model_complex_plain_vector A2(gmm::vect_size(*A));
	for (size_type i=0; i < gmm::vect_size(*A); ++i) // Not valid for 
	  A2[i] = gmm::sqr((*A)[i]); // non lagrangian fem ...
	if (mf_a)
	  asm_Helmholtz(matl[0], mim, mf_u, *mf_a, A2, rg);
	else
	  asm_homogeneous_Helmholtz(matl[0], mim, mf_u, A2, rg);
      } else
	GMM_ASSERT1(false, "Bad format Helmholtz brick coefficient");
    }

    Helmholtz_brick(void) {
      set_flags("Helmholtz", true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, true /* is complex */);
    }

  };

  size_type add_Helmholtz_brick(model &md, const mesh_im &mim,
				const std::string &varname,
				const std::string &dataname,
				size_type region) {
    pbrick pbr = new Helmholtz_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    return md.add_brick(pbr, model::varnamelist(1, varname),
			model::varnamelist(1, dataname), tl,
			model::mimlist(1, &mim), region);
  }



  // ----------------------------------------------------------------------
  //
  // Fourier-Robin brick
  //
  // ----------------------------------------------------------------------

  struct Fourier_Robin_brick : public virtual_brick {

    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Fourier-Robin brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Fourier-Robin brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 1,
		  "Wrong number of variables for Fourier-Robin brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      size_type Q = mf_u.get_qdim(), s = 1;
      const mesh_im &mim = *mims[0];
      const mesh_fem *mf_a = 0;
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      const model_real_plain_vector *A = &(md.real_variable(dl[0]));
      mf_a = md.pmesh_fem_of_variable(dl[0]);
      s = gmm::vect_size(*A);
      if (mf_a) s = s * mf_a->get_qdim() / mf_a->nb_dof();
      GMM_ASSERT1(s == Q*Q,
		  "Bad format Fourier-Robin brick coefficient");

      if (mf_a)
	asm_qu_term(matl[0], mim, mf_u, *mf_a, *A, rg);
      else
	asm_homogeneous_qu_term(matl[0], mim, mf_u, *A, rg);
    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &,
					   size_type region,
					   nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Fourier-Robin brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Fourier-Robin brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 1,
		  "Wrong number of variables for Fourier-Robin brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      size_type Q = mf_u.get_qdim(), s = 1;
      const mesh_im &mim = *mims[0];
      const mesh_fem *mf_a = 0;
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      const model_complex_plain_vector *A = &(md.complex_variable(dl[0]));
      mf_a = md.pmesh_fem_of_variable(dl[0]);
      s = gmm::vect_size(*A);
      if (mf_a) s = s * mf_a->get_qdim() / mf_a->nb_dof();
      GMM_ASSERT1(s == Q*Q,
		  "Bad format Fourier-Robin brick coefficient");

      if (mf_a)
	asm_qu_term(matl[0], mim, mf_u, *mf_a, *A, rg);
      else
	asm_homogeneous_qu_term(matl[0], mim, mf_u, *A, rg);
    }

    Fourier_Robin_brick(void) {
      set_flags("Fourier Robin condition", true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, true /* is complex */);
    }

  };

  size_type add_Fourier_Robin_brick(model &md, const mesh_im &mim,
				    const std::string &varname,
				    const std::string &dataname,
				    size_type region) {
    pbrick pbr = new Fourier_Robin_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    return md.add_brick(pbr, model::varnamelist(1, varname),
			model::varnamelist(1, dataname), tl,
			model::mimlist(1, &mim), region);
  }

  // ----------------------------------------------------------------------
  //
  // Constraint brick
  //
  // ----------------------------------------------------------------------

  struct have_private_data_brick : public virtual_brick {
    
    model_real_sparse_matrix rB;
    model_complex_sparse_matrix cB;
    model_real_plain_vector rL;
    model_complex_plain_vector cL;

  };

  struct constraint_brick : public have_private_data_brick {
    
    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &vecl,
					size_type, nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Constraint brick has one and only one term");
      GMM_ASSERT1(mims.size() == 0,
		  "Constraint brick need no mesh_im");
      GMM_ASSERT1(vl.size() >= 1 && vl.size() <= 2 && dl.size() <= 1,
		  "Wrong number of variables for constraint brick");

      bool penalized = (vl.size() == 1);
      const model_real_plain_vector *COEFF = 0;

      if (penalized) {
	COEFF = &(md.real_variable(dl[0]));
	GMM_ASSERT1(gmm::vect_size(*COEFF) == 1,
		    "Data for coefficient should be a scalar");
      }

      if (penalized) {
	gmm::mult(gmm::transposed(rB), gmm::scaled(rL, gmm::abs((*COEFF)[0])),
		  vecl[0]);
	gmm::mult(gmm::transposed(rB), gmm::scaled(rB, gmm::abs((*COEFF)[0])),
		  matl[0]);
      } else {
	gmm::copy(rL, vecl[0]);
	gmm::copy(rB, matl[0]);
      }
    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &vecl,
					   size_type,
					   nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Constraint brick has one and only one term");
      GMM_ASSERT1(mims.size() == 0,
		  "Constraint brick need no mesh_im");
      GMM_ASSERT1(vl.size() >= 1 && vl.size() <= 2 && dl.size() <= 1,
		  "Wrong number of variables for constraint brick");
      
      bool penalized = (vl.size() == 1);
      const model_complex_plain_vector *COEFF = 0;

      if (penalized) {
	COEFF = &(md.complex_variable(dl[0]));
	GMM_ASSERT1(gmm::vect_size(*COEFF) == 1,
		    "Data for coefficient should be a scalar");
      }

      if (penalized) {
	gmm::mult(gmm::transposed(cB), gmm::scaled(cL, gmm::abs((*COEFF)[0])),
		  vecl[0]);
	gmm::mult(gmm::transposed(cB), gmm::scaled(cB, gmm::abs((*COEFF)[0])),
		  matl[0]);
      } else {
	gmm::copy(cL, vecl[0]);
	gmm::copy(cB, matl[0]);
      }
    }

    constraint_brick(bool penalized) {
      set_flags(penalized ? "Constraint with penalization brick"
		          : "Constraint with multipliers brick",
		true /* is linear*/,
		true /* is symmetric */, penalized /* is coercive */,
		true /* is real */, true /* is complex */);
    }


  };

  model_real_sparse_matrix &set_private_data_brick_real_matrix
  (model &md, size_type indbrick) {
    pbrick pbr = md.brick_pointer(indbrick);
    md.touch_brick(indbrick);
    have_private_data_brick *p = dynamic_cast<have_private_data_brick *>
      (const_cast<virtual_brick *>(pbr.get()));
    GMM_ASSERT1(p, "Wrong type of brick");
    return p->rB;
  }

  model_real_plain_vector &set_private_data_brick_real_rhs
  (model &md, size_type indbrick) {
    pbrick pbr = md.brick_pointer(indbrick);
    md.touch_brick(indbrick);
    have_private_data_brick *p = dynamic_cast<have_private_data_brick *>
      (const_cast<virtual_brick *>(pbr.get()));
    GMM_ASSERT1(p, "Wrong type of brick");
    return p->rL;
  }

  model_complex_sparse_matrix &set_private_data_brick_complex_matrix
  (model &md, size_type indbrick) {
    pbrick pbr = md.brick_pointer(indbrick);
    md.touch_brick(indbrick);
    have_private_data_brick *p = dynamic_cast<have_private_data_brick *>
      (const_cast<virtual_brick *>(pbr.get()));
    GMM_ASSERT1(p, "Wrong type of brick");
    return p->cB;
  }

  model_complex_plain_vector &set_private_data_brick_complex_rhs
  (model &md, size_type indbrick) {
    pbrick pbr = md.brick_pointer(indbrick);
    md.touch_brick(indbrick);
    have_private_data_brick *p = dynamic_cast<have_private_data_brick *>
      (const_cast<virtual_brick *>(pbr.get()));
    GMM_ASSERT1(p, "Wrong type of brick");
    return p->cL;
  }

  size_type add_constraint_with_penalization
  (model &md, const std::string &varname, scalar_type penalisation_coeff) {
    std::string coeffname = md.new_name("penalization_on_" + varname);
    md.add_fixed_size_data(coeffname, 1);
    if (md.is_complex())
      md.set_complex_variable(coeffname)[0] = penalisation_coeff;
    else
      md.set_real_variable(coeffname)[0] = penalisation_coeff;
    pbrick pbr = new constraint_brick(true);
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    model::varnamelist vl(1, varname);
    model::varnamelist dl(1, coeffname);
    return md.add_brick(pbr, vl, dl, tl, model::mimlist(), size_type(-1));
  }
  
  size_type add_constraint_with_multipliers
  (model &md, const std::string &varname, const std::string &multname) {
    pbrick pbr = new constraint_brick(false);
    model::termlist tl;
    tl.push_back(model::term_description(multname, varname, true));
    model::varnamelist vl(1, varname);
    vl.push_back(multname);
    model::varnamelist dl;
    return md.add_brick(pbr, vl, dl, tl, model::mimlist(), size_type(-1));
  }


  // ----------------------------------------------------------------------
  //
  // Explicit matrix brick
  //
  // ----------------------------------------------------------------------

  struct explicit_matrix_brick : public have_private_data_brick {
    
    virtual void asm_real_tangent_terms(const model &,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &vecl,
					size_type, nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Explicit matrix has one and only one term");
      GMM_ASSERT1(mims.size() == 0, "Explicit matrix need no mesh_im");
      GMM_ASSERT1(vl.size() >= 1 && vl.size() <= 2 && dl.size() == 0,
		  "Wrong number of variables for explicit matrix brick");
      gmm::copy(rB, matl[0]);
    }

    virtual void asm_complex_tangent_terms(const model &,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &vecl,
					   size_type,
					   nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Explicit matrix has one and only one term");
      GMM_ASSERT1(mims.size() == 0, "Explicit matrix need no mesh_im");
      GMM_ASSERT1(vl.size() >= 1 && vl.size() <= 2 && dl.size() == 0,
		  "Wrong number of variables for explicit matrix brick");
      gmm::copy(cB, matl[0]);
    }

    explicit_matrix_brick(bool symmetric_, bool coercive_) {
      set_flags("Explicit matrix brick",
		true /* is linear*/,
		symmetric_ /* is symmetric */, coercive_ /* is coercive */,
		true /* is real */, true /* is complex */);
    }
  };

  size_type add_explicit_matrix
  (model &md, const std::string &varname1, const std::string &varname2,
   bool issymmetric, bool iscoercive) {
    pbrick pbr = new explicit_matrix_brick(issymmetric, iscoercive);
    model::termlist tl;
    tl.push_back(model::term_description(varname1, varname2, issymmetric));
    model::varnamelist vl(1, varname1);
    vl.push_back(varname2);
    model::varnamelist dl;
    return md.add_brick(pbr, vl, dl, tl, model::mimlist(), size_type(-1));
  }

  // ----------------------------------------------------------------------
  //
  // Explicit rhs brick
  //
  // ----------------------------------------------------------------------

  struct explicit_rhs_brick : public have_private_data_brick {
    
    virtual void asm_real_tangent_terms(const model &,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &vecl,
					size_type, nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Explicit rhs has one and only one term");
      GMM_ASSERT1(mims.size() == 0, "Explicit rhs need no mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 0,
		  "Wrong number of variables for explicit rhs brick");
      gmm::copy(rL, vecl[0]);
    }

    virtual void asm_complex_tangent_terms(const model &,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &vecl,
					   size_type,
					   nonlinear_version) const {
      GMM_ASSERT1(vecl.size() == 1 && matl.size() == 1,
		  "Explicit rhs has one and only one term");
      GMM_ASSERT1(mims.size() == 0, "Explicit rhs need no mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 0,
		  "Wrong number of variables for explicit rhs brick");
      gmm::copy(cL, vecl[0]);
      
    }

    explicit_rhs_brick(void) {
      set_flags("Explicit rhs brick",
		true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, true /* is complex */);
    }


  };

  size_type add_explicit_rhs
  (model &md, const std::string &varname) {
    pbrick pbr = new explicit_rhs_brick();
    model::termlist tl;
    tl.push_back(model::term_description(varname));
    model::varnamelist vl(1, varname);
    model::varnamelist dl;
    return md.add_brick(pbr, vl, dl, tl, model::mimlist(), size_type(-1));
  }


  // ----------------------------------------------------------------------
  //
  // Isotropic linearized elasticity brick
  //
  // ----------------------------------------------------------------------

  struct iso_lin_elasticity_brick : public virtual_brick {

    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "isotropic linearized elasticity brick has one and only "
		  "one term");
      GMM_ASSERT1(mims.size() == 1,
		  "isotropic linearized elasticity brick need one and only "
		  "one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() == 2,
		  "Wrong number of variables for isotropic linearized "
		  "elasticity brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      size_type N = m.dim(), Q = mf_u.get_qdim();
      GMM_ASSERT1(Q == N, "isotropic linearized elasticity brick is only "
		  "for vector field of the same dimension as the mesh");
      const mesh_im &mim = *mims[0];
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      const mesh_fem *mf_lambda = md.pmesh_fem_of_variable(dl[0]);
      const model_real_plain_vector *lambda = &(md.real_variable(dl[0]));
      const mesh_fem *mf_mu = md.pmesh_fem_of_variable(dl[1]);
      const model_real_plain_vector *mu = &(md.real_variable(dl[1]));
     
      size_type sl = gmm::vect_size(*lambda);
      if (mf_lambda) sl = sl * mf_lambda->get_qdim() / mf_lambda->nb_dof();
      size_type sm = gmm::vect_size(*mu);
      if (mf_mu) sm = sm * mf_mu->get_qdim() / mf_mu->nb_dof();

      GMM_ASSERT1(sl == 1 && sm == 1, "Bad format of isotropic linearized "
		  "elasticity brick coefficients");
      GMM_ASSERT1(mf_lambda == mf_mu,
		  "The two coefficients should be described on the same "
		  "finite element method.");
      
      if (mf_lambda)
	asm_stiffness_matrix_for_linear_elasticity
	  (matl[0], mim, mf_u, *mf_lambda, *lambda, *mu, rg);
      else
	asm_stiffness_matrix_for_homogeneous_linear_elasticity
	  (matl[0], mim, mf_u, *lambda, *mu, rg);
    }

    iso_lin_elasticity_brick(void) {
      set_flags("isotropic linearized elasticity", true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, false /* is complex */);
    }

  };

  size_type add_isotropic_linearized_elasticity_brick
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &dataname1, const std::string &dataname2,
   size_type region) {
    pbrick pbr = new iso_lin_elasticity_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    model::varnamelist dl(1, dataname1);
    dl.push_back(dataname2);
    return md.add_brick(pbr, model::varnamelist(1, varname), dl, tl,
			model::mimlist(1, &mim), region);
  }

  void compute_isotropic_linearized_Von_Mises_or_Tresca
  (model &md, const std::string &varname, const std::string &dataname_lambda,
   const std::string &dataname_mu, const mesh_fem &mf_vm, 
   model_real_plain_vector &VM, bool tresca) {

    const mesh_fem &mf_u = md.mesh_fem_of_variable(varname);
    const mesh_fem *mf_lambda = md.pmesh_fem_of_variable(dataname_lambda);
    const model_real_plain_vector *lambda=&(md.real_variable(dataname_lambda));
    const mesh_fem *mf_mu = md.pmesh_fem_of_variable(dataname_mu);
    const model_real_plain_vector *mu = &(md.real_variable(dataname_mu));
   
    size_type sl = gmm::vect_size(*lambda);
    if (mf_lambda) sl = sl * mf_lambda->get_qdim() / mf_lambda->nb_dof();
    size_type sm = gmm::vect_size(*mu);
    if (mf_mu) sm = sm * mf_mu->get_qdim() / mf_mu->nb_dof();
    
    GMM_ASSERT1(sl == 1 && sm == 1, "Bad format for Lam� coefficients");
    GMM_ASSERT1(mf_lambda == mf_mu,
		"The two Lam� coefficients should be described on the same "
		"finite element method.");

    if (mf_lambda) {
      getfem::interpolation_von_mises_or_tresca(mf_u, mf_vm,
						md.real_variable(varname), VM,
						*mf_lambda, *lambda,
						*mf_lambda, *mu,
						tresca);
    } else {
      mf_lambda = &(classical_mesh_fem(mf_u.linked_mesh(), 0));
      model_real_plain_vector LAMBDA(mf_lambda->nb_dof(), (*lambda)[0]);
      model_real_plain_vector MU(mf_lambda->nb_dof(), (*mu)[0]);
      getfem::interpolation_von_mises_or_tresca(mf_u, mf_vm,
						md.real_variable(varname), VM,
						*mf_lambda, LAMBDA,
						*mf_lambda, MU,
						tresca);
    }
  }

  // ----------------------------------------------------------------------
  //
  // linearized incompressibility brick  (div u = 0)
  //
  // ----------------------------------------------------------------------

  struct linear_incompressibility_brick : public virtual_brick {
    
    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &,
					size_type region,
					nonlinear_version) const {
      
      GMM_ASSERT1((matl.size() == 1 && dl.size() == 0)
		  || (matl.size() == 2 && dl.size() == 1),
		  "Wrong term and/or data number for Linear incompressibility "
		  "brick.");
      GMM_ASSERT1(mims.size() == 1, "Linear incompressibility brick need one "
		  "and only one mesh_im");
      GMM_ASSERT1(vl.size() == 2, "Wrong number of variables for linear "
		  "incompressibility brick");

      bool penalized = (dl.size() == 1);
      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh_fem &mf_p = md.mesh_fem_of_variable(vl[1]);
      const mesh_im &mim = *mims[0];
      const model_real_plain_vector *A = 0, *COEFF = 0;
      const mesh_fem *mf_data = 0;

      if (penalized) {
	COEFF = &(md.real_variable(dl[0]));
	mf_data = md.pmesh_fem_of_variable(dl[0]);
	size_type s = gmm::vect_size(*A);
	if (mf_data) s = s * mf_data->get_qdim() / mf_data->nb_dof();
	GMM_ASSERT1(s == 1, "Bad format for the penalization parameter");
      }

      mesh_region rg(region);
      mim.linked_mesh().intersect_with_mpi_region(rg);

      asm_stokes_B(matl[0], mim, mf_u, mf_p, rg);

      if (penalized) {
	if (mf_data) {
	  asm_mass_matrix_param(matl[1], mim, mf_p, *mf_data, *A, rg);
	  gmm::scale(matl[1], scalar_type(-1));
	}
	else {
	  asm_mass_matrix(matl[1], mim, mf_p, rg);
	  gmm::scale(matl[1], -(*A)[0]);
	}
      }

    }

    linear_incompressibility_brick(void) {
      set_flags("Linear incompressibility brick",
		true /* is linear*/,
		true /* is symmetric */, false /* is coercive */,
		true /* is real */, false /* is complex */);
    }


  };

  size_type add_linear_incompressibility
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &multname, size_type region,
   const std::string &dataname) {
    pbrick pbr = new linear_incompressibility_brick();
    model::termlist tl;
    tl.push_back(model::term_description(multname, varname, true));
    model::varnamelist vl(1, varname);
    vl.push_back(multname);
    model::varnamelist dl;
    if (dataname.size()) {
      dl.push_back(dataname);
      tl.push_back(model::term_description(multname, multname, true));
    }
    return md.add_brick(pbr, vl, dl, tl, model::mimlist(1, &mim), region);
  }



  // ----------------------------------------------------------------------
  //
  // Mass brick
  //
  // ----------------------------------------------------------------------

  struct mass_brick : public virtual_brick {

    virtual void asm_real_tangent_terms(const model &md,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &,
					size_type region,
					nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Mass brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Mass brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() <= 1,
		  "Wrong number of variables for mass brick");

      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      const mesh_im &mim = *mims[0];
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      
      const mesh_fem *mf_rho = 0;
      const model_real_plain_vector *rho = 0;

      if (dl.size()) {
	mf_rho = md.pmesh_fem_of_variable(dl[0]);
	rho = &(md.real_variable(dl[0]));
	size_type sl = gmm::vect_size(*rho);
	if (mf_rho) sl = sl * mf_rho->get_qdim() / mf_rho->nb_dof();
	GMM_ASSERT1(sl == 1, "Bad format of mass brick coefficient");
      }
      
      if (dl.size() && mf_rho) {
	asm_mass_matrix_param(matl[0], mim, mf_u, *mf_rho, *rho, rg);
      } else {
	asm_mass_matrix(matl[0], mim, mf_u, rg);
	if (dl.size()) gmm::scale(matl[0], (*rho)[0]);
      }
    }

    virtual void asm_complex_tangent_terms(const model &md,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &,
					   size_type region,
					   nonlinear_version) const {
      GMM_ASSERT1(matl.size() == 1,
		  "Mass brick has one and only one term");
      GMM_ASSERT1(mims.size() == 1,
		  "Mass brick need one and only one mesh_im");
      GMM_ASSERT1(vl.size() == 1 && dl.size() <= 1,
		  "Wrong number of variables for mass brick");
      
      const mesh_fem &mf_u = md.mesh_fem_of_variable(vl[0]);
      const mesh &m = mf_u.linked_mesh();
      const mesh_im &mim = *mims[0];
      mesh_region rg(region);
      m.intersect_with_mpi_region(rg);
      
      const mesh_fem *mf_rho = 0;
      const model_complex_plain_vector *rho = 0;
      
      if (dl.size()) {
	mf_rho = md.pmesh_fem_of_variable(dl[0]);
	rho = &(md.complex_variable(dl[0]));
	size_type sl = gmm::vect_size(*rho);
	if (mf_rho) sl = sl * mf_rho->get_qdim() / mf_rho->nb_dof();
	GMM_ASSERT1(sl == 1, "Bad format of mass brick coefficient");
      }
      
      if (dl.size() && mf_rho) {
	asm_mass_matrix_param(matl[0], mim, mf_u, *mf_rho, *rho, rg);
      } else {
	asm_mass_matrix(matl[0], mim, mf_u, rg);
	if (dl.size()) gmm::scale(matl[0], (*rho)[0]);
      }
    }

    mass_brick(void) {
      set_flags("Mass brick", true /* is linear*/,
		true /* is symmetric */, true /* is coercive */,
		true /* is real */, true /* is complex */);
    }

  };

  size_type add_mass_brick
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &dataname_rho,  size_type region) {
    pbrick pbr = new mass_brick;
    model::termlist tl;
    tl.push_back(model::term_description(varname, varname, true));
    model::varnamelist dl;
    if (dataname_rho.size())
      dl.push_back(dataname_rho);
    return md.add_brick(pbr, model::varnamelist(1, varname), dl, tl,
			model::mimlist(1, &mim), region);
  }


  // ----------------------------------------------------------------------
  //
  //
  // Standard time dispatchers
  //
  //
  // ----------------------------------------------------------------------


  // ----------------------------------------------------------------------
  //
  // theta-method dispatcher
  //
  // ----------------------------------------------------------------------

  class theta_method_dispatcher : public virtual_dispatcher {

  public :

    typedef model::assembly_version nonlinear_version;

    virtual void asm_real_tangent_terms(const model &md, pbrick pbr,
					const model::varnamelist &vl,
					const model::varnamelist &dl,
					const model::mimlist &mims,
					model::real_matlist &matl,
					model::real_veclist &vectl,
					size_type region,
					nonlinear_version version) const {
      scalar_type theta = real_params[0];


      // Cas lin�aire

      // La brique a �t� appel�e si n�cessaire par udpdate_brick.
      // rendre les coefficients et calculer le second membre suppl�mentaire



      // Cas non-lin�aire (ne pas oublier de g�rer `version`
      // Cr�ation �ventuelle variables temporaire
      // Appel brique pour le second membre
      // Cr�ation du second membre suppl�mentaire
      // Appel brique pour le membre courant
      pbr->asm_real_tangent_terms(md, vl, dl, mims, matl, vectl,
				  region, version);

      // rendre les coefficients (seulement deux coefficients).




      // il faut lister les termes ... ca serait plut�t le boulot de la brique ...  la brique stocke le r�sultat de l'appel pour une brique lin�aire et demande un param�tre multiplicatif pour la suite ainsi que la formule de modification du r�sidu.

      // pour une brique non lin�aire ... ca se corse dans certain cas, mais rien � stocker ...
      for (size_type i = 0; i < matl.size(); ++i) {

	

      }
      


    }

    virtual void asm_complex_tangent_terms(const model &md, pbrick pbr,
					   const model::varnamelist &vl,
					   const model::varnamelist &dl,
					   const model::mimlist &mims,
					   model::complex_matlist &matl,
					   model::complex_veclist &vectl,
					   size_type region,
					   nonlinear_version version) const {
      pbr->asm_complex_tangent_terms(md, vl, dl, mims, matl, vectl,
				  region, version);
    }
    
  };
  
 


  // ----------------------------------------------------------------------
  //
  // midpoint dispatcher ... to be done
  //
  // ----------------------------------------------------------------------




}  /* end of namespace getfem.                                             */
