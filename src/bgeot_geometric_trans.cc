/*===========================================================================

 Copyright (C) 2000-2016 Yves Renard

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


#include "getfem/dal_singleton.h"
#include "getfem/dal_tree_sorted.h"
#include "getfem/bgeot_geometric_trans.h"
#include "getfem/bgeot_poly_composite.h"
#include "getfem/getfem_torus.h"

namespace bgeot {

  DEFINE_STATIC_THREAD_LOCAL(std::vector<scalar_type>, __aux1)
  DEFINE_STATIC_THREAD_LOCAL(std::vector<scalar_type>, __aux2)
  DEFINE_STATIC_THREAD_LOCAL(std::vector<scalar_type>, __aux3)
  DEFINE_STATIC_THREAD_LOCAL(std::vector<int>, __ipvt_aux)
  
  // Optimized lu_factor for small square matrices
  size_type lu_factor(scalar_type *A, std::vector<int> &ipvt,
		      size_type N) {
    size_type info(0), i, j, jp, N_1 = N-1;
      
    if (N) {
      for (j = 0; j < N_1; ++j) {
	auto it = A + (j*(N+1));
	scalar_type max = gmm::abs(*it); jp = j;
	for (i = j+1; i < N; ++i) {
	  scalar_type ap = gmm::abs(*(++it));
	  if (ap > max) { jp = i; max = ap; }
	}
	ipvt[j] = int(jp + 1);
	
	if (max == scalar_type(0)) { info = j + 1; break; }
        if (jp != j) {
	  auto it1 = A+jp, it2 = A+j;
	  for (i = 0; i < N; ++i, it1+=N, it2+=N) std::swap(*it1, *it2);
	}
	it = A + (j*(N+1)); max = *it++;
	for (i = j+1; i < N; ++i) *it++ /= max; 
	auto it22 = A + (j*N + j+1), it11 = it22;
	auto it3 = A + ((j+1)*N+j);
	for (size_type l = j+1; l < N; ++l) {
	  it11 += N;
	  auto it1 = it11, it2 = it22;
	  scalar_type a = *it3; it3 += N;
	  for (size_type k = j+1; k < N; ++k) *it1++ -= *it2++ * a;
	}

      }
      ipvt[N-1] = int(N);
    }
    return info;
  }

  static void lower_tri_solve(const scalar_type *T, scalar_type *x, int N,
			      bool is_unit) {
    scalar_type x_j;
    for (int j = 0; j < N; ++j) {
      auto itc = T + j*N, it = itc+(j+1), ite = itc+N;
      auto itx = x + j;
      if (!is_unit) *itx /= itc[j];
      x_j = *itx++;
      for (; it != ite ; ++it, ++itx) *itx -= x_j * (*it);
    }
  }

  static void upper_tri_solve(const scalar_type *T, scalar_type *x, int N,
			      bool is_unit) {
    scalar_type x_j;
    for (int j = N - 1; j >= 0; --j) {
      auto itc = T + j*N, it = itc, ite = itc+j;
      auto itx = x;
      if (!is_unit) x[j] /= itc[j];
      for (x_j = x[j]; it != ite ; ++it, ++itx) *itx -= x_j * (*it);
    }
  }

  static void lu_solve(const scalar_type *LU, const std::vector<int> &ipvt, 
		       scalar_type *x, scalar_type *b, int N) {
    std::copy(b, b+N, x);
    for(int i = 0; i < N; ++i)
      { int perm = ipvt[i]-1; if(i != perm) std::swap(x[i], x[perm]); }
    bgeot::lower_tri_solve(LU, x, N, true);
    bgeot::upper_tri_solve(LU, x, N, false);
  }

  scalar_type lu_det(const scalar_type *LU, const std::vector<int> &ipvt,
		     size_type N) {
    scalar_type det(1);
    for (size_type j = 0; j < N; ++j) det *= *(LU+j*(N+1));
    for(int i = 0; i < int(N); ++i) if (i != ipvt[i]-1) { det = -det; }
    return det;
  }

  scalar_type lu_det(const scalar_type *A, size_type N) {
    if (N == 1) {
      return *A;
    } else if (N == 2) {
      return (*A) * (A[3]) - (A[1]) * (A[2]);
    } else {
      size_type NN = N*N;
      if (__aux1.size() < NN) __aux1.resize(N*N);
      std::copy(A, A+NN, __aux1.begin());
      __ipvt_aux.resize(N);
      lu_factor(&(*(__aux1.begin())), __ipvt_aux, N);
      return lu_det(&(*(__aux1.begin())), __ipvt_aux, N);
    }
  }

  void lu_inverse(const scalar_type *LU, const std::vector<int> &ipvt,
		  scalar_type *A, size_type N) {
    __aux2.resize(N); gmm::clear(__aux2);
    __aux3.resize(N);
    for(size_type i = 0; i < N; ++i) {
      __aux2[i] = scalar_type(1);
      bgeot::lu_solve(LU, ipvt, A+i*N, &(*(__aux2.begin())), int(N));
      __aux2[i] = scalar_type(0);
    }
  }

  scalar_type lu_inverse(scalar_type *A, size_type N, bool doassert) {
    if (N == 1) {
      scalar_type det = *A;
      GMM_ASSERT1(det != scalar_type(0), "Non invertible matrix");
      *A = scalar_type(1)/det;
      return det;
    } else if (N == 2) {
      scalar_type a = *A, b = A[2], c = A[1], d = A[3];
      scalar_type det = a * d - b * c;
      GMM_ASSERT1(det != scalar_type(0), "Non invertible matrix");
      *A++ =  d/det;  *A++ /= -det; *A++ /= -det;  *A =  a/det;
      return det;
    } else {
      size_type NN = N*N;
      if (__aux1.size() < NN) __aux1.resize(NN);
      std::copy(A, A+NN, __aux1.begin());
      __ipvt_aux.resize(N);
      size_type info = lu_factor(&(*(__aux1.begin())), __ipvt_aux, N);
      if (doassert) GMM_ASSERT1(!info, "Non invertible matrix, pivot = "<<info);
      if (!info) lu_inverse(&(*(__aux1.begin())), __ipvt_aux, A, N);
      return lu_det(&(*(__aux1.begin())), __ipvt_aux, N);
    }
  }



  void geometric_trans::compute_K_matrix
    (const base_matrix &G, const base_matrix &pc, base_matrix &K) const {
    // gmm::mult(G, pc, K);
    // Faster than the lapack call on my config ...
    size_type N=gmm::mat_nrows(G), P=gmm::mat_nrows(pc), Q=gmm::mat_ncols(pc);
    if (N && P && Q) {
      auto itK = K.begin();
      for (size_type j = 0; j < Q; ++j) {
	auto itpc_j = pc.begin() + j*P, itG_b = G.begin();
	for (size_type i = 0; i < N; ++i, ++itG_b) {
	  auto itG = itG_b, itpc = itpc_j;
	  register scalar_type a = *(itG) * (*itpc);
	  for (size_type k = 1; k < P; ++k)
	    { itG += N; a += *(itG) * (*++itpc); }
	  *itK++ = a;
	}
      }
    } else gmm::clear(K);
  }

  const base_node& geotrans_interpolation_context::xref() const {
    if (!have_xref()) {
      if (pspt_) xref_ = (*pspt_)[ii_];
      else GMM_ASSERT1(false, "Missing xref");
    }
    return xref_;
  }

  const base_node& geotrans_interpolation_context::xreal() const {
    if (!have_xreal()) {
      if (have_pgp()) {
        xreal_ = pgp_->transform(ii_, G());
      } else xreal_ = pgt()->transform(xref(),G());
    }
    return xreal_;
  }



  void geotrans_interpolation_context::compute_J(void) const {
    GMM_ASSERT1(have_G() && have_pgt(), "Unable to compute J\n");
    size_type P = pgt_->structure()->dim();
    const base_matrix &KK = K();
    if (P != N()) {
      B_factors.base_resize(P, P);
      gmm::mult(gmm::transposed(KK), KK, B_factors);
      ipvt.resize(P);
      bgeot::lu_factor(&(*(B_factors.begin())), ipvt, P);
      // gmm::abs below because on flat convexes determinant could be -1e-27.
      J_ = ::sqrt(gmm::abs(bgeot::lu_det(&(*(B_factors.begin())), ipvt, P)));
    }
    else {
      // J_ = gmm::abs(gmm::lu_det(KK));
      if (P <= 2) {
	auto it = KK.begin();
      	if (P == 1) J_ = gmm::abs(*it);
      	else J_ = gmm::abs((*it) * (it[3]) - (it[1]) * (it[2]));
      } else {
      	B_factors.base_resize(P, P); // store factorization for B computation
      	gmm::copy(gmm::transposed(KK), B_factors);
      	ipvt.resize(P);
	bgeot::lu_factor(&(*(B_factors.begin())), ipvt, P);
     	J_ = gmm::abs(bgeot::lu_det(&(*(B_factors.begin())), ipvt, P));
      }
    }
    have_J_ = true;
  }

  const base_matrix& geotrans_interpolation_context::K() const {
    if (!have_K()) {
      GMM_ASSERT1(have_G() && have_pgt(), "Unable to compute K\n");
      size_type P = pgt_->structure()->dim();
      K_.base_resize(N(), P);
      if (have_pgp()) {
	pgt_->compute_K_matrix(*G_, pgp_->grad(ii_), K_);
      } else {
	PC.base_resize(pgt_->nb_points(), P);
        pgt_->poly_vector_grad(xref(), PC);
	pgt_->compute_K_matrix(*G_, PC, K_);
      }
      have_K_ = true;
    }
    return K_;
  }

  const base_matrix& geotrans_interpolation_context::B() const {
    if (!have_B()) {
      const base_matrix &KK = K();
      size_type P = pgt_->structure()->dim(), N_ = gmm::mat_nrows(KK);
      B_.base_resize(N_, P);
      if (P != N_) {
	if (!have_J_) compute_J();
	PC.base_resize(P, P);
        gmm::lu_inverse(B_factors, ipvt, PC);
        gmm::mult(KK, PC, B_);
      } else if (P == 1) {
        scalar_type det = KK(0, 0);
        GMM_ASSERT1(det != scalar_type(0), "Non invertible matrix");
        B_(0, 0) = scalar_type(1)/det;
        J_ = gmm::abs(det);
      } else if (P == 2) {
        const scalar_type *p = &(KK(0,0));
        scalar_type det = (*p) * (*(p+3)) - (*(p+1)) * (*(p+2));
        GMM_ASSERT1(det != scalar_type(0), "Non invertible matrix");
        scalar_type *q = &(B_(0,0));
        *q++ =  (*(p+3)) / det;  *q++ = -(*(p+2)) / det;
        *q++ = -(*(p+1)) / det;  *q++ =  (*p) / det;
        J_ = gmm::abs(det);
      } else {
        scalar_type det = J();
        GMM_ASSERT1(det != scalar_type(0), "Non invertible matrix");
	bgeot::lu_inverse(&(*(B_factors.begin())), ipvt, &(*(B_.begin())), P);
      }
      have_B_ = true;
    }
    return B_;
  }

  const base_matrix& geotrans_interpolation_context::B3() const {
    if (!have_B3()) {
      const base_matrix &BB = B();
      size_type P=gmm::mat_ncols(BB), N_=gmm::mat_nrows(BB);
      B3_.base_resize(N_*N_, P*P);
      for (short_type i = 0; i < P; ++i)
        for (short_type j = 0; j < P; ++j)
          for (short_type k = 0; k < N_; ++k)
            for (short_type l = 0; l < N_; ++l)
              B3_(k + N_*l, i + P*j) = BB(k, i) * BB(l, j);
      have_B3_ = true;
    }
    return B3_;
  }

  const base_matrix& geotrans_interpolation_context::B32() const {
    if (!have_B32()) {
      const base_matrix &BB = B();
      size_type P=gmm::mat_ncols(BB), N_=gmm::mat_nrows(BB);
      B32_.base_resize(N_*N_, P);
      if (!pgt()->is_linear()) {
        base_matrix B2(P*P, P), Htau(N_, P*P);
        if (have_pgp()) {
          gmm::mult(G(), pgp_->hessian(ii_), Htau);
        } else {
          /* very inefficient of course... */
	  PC.base_resize(pgt()->nb_points(), P*P);
          pgt()->poly_vector_hess(xref(), PC);
          gmm::mult(G(), PC, Htau);
        }
        for (short_type i = 0; i < P; ++i)
          for (short_type j = 0; j < P; ++j)
            for (short_type k = 0; k < P; ++k)
              for (short_type l = 0; l < N_; ++l)
                B2(i + P*j, k) += Htau(l, i + P*j) * BB(l,k);
        gmm::mult(B3(), B2, B32_);
      } else gmm::clear(B32_);
      have_B32_ = true;
    }
    return B32_;
  }

  void geotrans_interpolation_context::set_xref(const base_node& P) {
    xref_ = P;
    if (pgt_ && !pgt()->is_linear())
      { have_K_ = have_B_ = have_B3_ = have_B32_ = have_J_ = false; }
    xreal_.resize(0); ii_ = size_type(-1); pspt_ = 0;
  }


  base_node geometric_trans::transform(const base_node &pt,
                                       const base_matrix &G) const {
    size_type N = G.nrows(), k = nb_points();
    base_node P(N); base_vector val(k);
    poly_vector_val(pt, val);
    base_matrix::const_iterator git = G.begin();
    for (size_type l = 0; l < k; ++l) {
      scalar_type a = val[l];
      base_node::iterator pit = P.begin(), pite = P.end();
      for (; pit != pite; ++git, ++pit) *pit += a * (*git);
    }
    return P;
  }

  void geometric_trans::fill_standard_vertices(void) {
    vertices_.resize(0);
    for (size_type ip = 0; ip < nb_points(); ++ip) {
      bool vertex = true;
      for (size_type i = 0; i < cvr->points()[ip].size(); ++i)
        if (gmm::abs(cvr->points()[ip][i]) > 1e-10
            && gmm::abs(cvr->points()[ip][i]-1.0) > 1e-10)
          { vertex = false; break; }
      if (vertex) vertices_.push_back(ip);
    }
    assert(vertices_.size() >= dim());
  }

  /* ******************************************************************** */
  /* Instantied geometric transformations.                                */
  /* ******************************************************************** */

  template <class FUNC>
  struct igeometric_trans : public geometric_trans {

    std::vector<FUNC> trans;

    virtual void poly_vector_val(const base_node &pt, base_vector &val) const {
      val.resize(nb_points());
      for (size_type k = 0; k < nb_points(); ++k)
        val[k] = to_scalar(trans[k].eval(pt.begin()));
    }

    virtual void poly_vector_val(const base_node &pt, const convex_ind_ct &ind_ct,
                                 base_vector &val) const {
      size_type nb_funcs=ind_ct.size();
      val.resize(nb_funcs);
      for (size_type k = 0; k < nb_funcs; ++k)
        val[k] = to_scalar(trans[ind_ct[k]].eval(pt.begin()));
    }

    virtual void poly_vector_grad(const base_node &pt, base_matrix &pc) const {
      FUNC PP;
      pc.base_resize(nb_points(),dim());
      for (size_type i = 0; i < nb_points(); ++i)
        for (dim_type n = 0; n < dim(); ++n) {
          PP = trans[i];
          PP.derivative(n);
          pc(i, n) = to_scalar(PP.eval(pt.begin()));
        }
    }

    virtual void poly_vector_grad(const base_node &pt,
				  const convex_ind_ct &ind_ct,
                                  base_matrix &pc) const {
      FUNC PP;
      size_type nb_funcs=ind_ct.size();
      pc.base_resize(nb_funcs,dim());
      for (size_type i = 0; i < nb_funcs; ++i)
        for (dim_type n = 0; n < dim(); ++n) {
          PP = trans[ind_ct[i]];
          PP.derivative(n);
          pc(i, n) = to_scalar(PP.eval(pt.begin()));
        }
    }

    virtual void poly_vector_hess(const base_node &pt, base_matrix &pc) const {
      FUNC PP, QP;
      pc.base_resize(nb_points(),dim()*dim());
      for (size_type i = 0; i < nb_points(); ++i)
        for (dim_type n = 0; n < dim(); ++n) {
          QP = trans[i]; QP.derivative(n);
          for (dim_type m = 0; m <= n; ++m) {
            PP = QP; PP.derivative(m);
            pc(i, n*dim()+m) = pc(i, m*dim()+n) = to_scalar(PP.eval(pt.begin()));
          }
        }
    }

  };

  typedef igeometric_trans<base_poly> poly_geometric_trans;
  typedef igeometric_trans<polynomial_composite> comppoly_geometric_trans;

  /* ******************************************************************** */
  /* transformation on simplex.                                           */
  /* ******************************************************************** */

  struct simplex_trans_ : public poly_geometric_trans {
    void calc_base_func(base_poly &p, size_type i, short_type K) const {
      dim_type N = dim();
      base_poly l0(N, 0), l1(N, 0);
      power_index w(short_type(N+1));
      l0.one(); l1.one(); p = l0;
      for (short_type nn = 0; nn < N; ++nn) l0 -= base_poly(N, 1, nn);

      w[0] = K;
      for (int nn = 1; nn <= N; ++nn) {
        w[nn]=short_type(floor(0.5+(((cvr->points())[i])[nn-1]*double(K))));
        w[0]=short_type(w[0]-w[nn]);
      }

      for (short_type nn = 0; nn <= N; ++nn)
        for (short_type j = 0; j < w[nn]; ++j)
          if (nn == 0)
            p *= (l0 * (scalar_type(K) / scalar_type(j+1)))
               - (l1 * (scalar_type(j) / scalar_type(j+1)));
          else
            p *= (base_poly(N, 1, short_type(nn-1)) * (scalar_type(K) / scalar_type(j+1)))
               - (l1 * (scalar_type(j) / scalar_type(j+1)));
    }

    simplex_trans_(dim_type nc, short_type k) {
      cvr = simplex_of_reference(nc, k);
      size_type R = cvr->structure()->nb_points();
      is_lin = (k == 1);
      complexity_ = k;
      trans.resize(R);
      for (size_type r = 0; r < R; ++r) calc_base_func(trans[r], r, k);
      fill_standard_vertices();
    }
  };

  static pgeometric_trans
  PK_gt(gt_param_list &params,
        std::vector<dal::pstatic_stored_object> &dependencies) {
    GMM_ASSERT1(params.size() == 2, "Bad number of parameters : "
                << params.size() << " should be 2.");
    GMM_ASSERT1(params[0].type() == 0 && params[1].type() == 0,
                "Bad type of parameters");
    int n = int(::floor(params[0].num() + 0.01));
    int k = int(::floor(params[1].num() + 0.01));
    GMM_ASSERT1(n >= 0 && n < 100 && k >= 0 && k <= 150 &&
                double(n) == params[0].num() && double(k) == params[1].num(),
                "Bad parameters");
    dependencies.push_back(simplex_of_reference(dim_type(n), dim_type(k)));
    return std::make_shared<simplex_trans_>(dim_type(n), dim_type(k));
  }

  /* ******************************************************************** */
  /* direct product transformation                                        */
  /* ******************************************************************** */

  struct cv_pr_t_ : public poly_geometric_trans {
    cv_pr_t_(const poly_geometric_trans *a, const poly_geometric_trans *b) {
      cvr = convex_ref_product(a->convex_ref(), b->convex_ref());
      is_lin = false;
      complexity_ = a->complexity() * b->complexity();

      size_type n1 = a->nb_points(), n2 = b->nb_points();
      trans.resize(n1 * n2);
      for (size_type i1 = 0; i1 < n1; ++i1)
        for (size_type i2 = 0; i2 < n2; ++i2) {
          trans[i1 + i2 * n1] = a->trans[i1];
          trans[i1 + i2 * n1].direct_product(b->trans[i2]);
        }
      for (size_type i2 = 0; i2 < b->nb_vertices(); ++i2)
        for (size_type i1 = 0; i1 < a->nb_vertices(); ++i1)
          vertices_.push_back(a->vertices()[i1] + b->vertices()[i2] * n1);
    }
  };

  static pgeometric_trans product_gt(gt_param_list &params,
                  std::vector<dal::pstatic_stored_object> &dependencies) {
    GMM_ASSERT1(params.size() == 2, "Bad number of parameters : "
                << params.size() << " should be 2.");
    GMM_ASSERT1(params[0].type() == 1 && params[1].type() == 1,
                "Bad type of parameters");
    pgeometric_trans a = params[0].method();
    pgeometric_trans b = params[1].method();
    dependencies.push_back(a); dependencies.push_back(b);
    dependencies.push_back(convex_ref_product(a->convex_ref(),
                                              b->convex_ref()));
    const poly_geometric_trans *aa
      = dynamic_cast<const poly_geometric_trans *>(a.get());
    const poly_geometric_trans *bb
      = dynamic_cast<const poly_geometric_trans *>(b.get());
    GMM_ASSERT1(aa && bb, "The product of geometric transformations "
                "is only defined for polynomial ones");
    return std::make_shared<cv_pr_t_>(aa, bb);
  }

  /* ******************************************************************** */
  /* linear direct product transformation.                                */
  /* ******************************************************************** */

  struct cv_pr_tl_ : public poly_geometric_trans {
    cv_pr_tl_(const poly_geometric_trans *a, const poly_geometric_trans *b) {
      GMM_ASSERT1(a->is_linear() && b->is_linear(),
                  "linear product of non-linear transformations");
      cvr = convex_ref_product(a->convex_ref(), b->convex_ref());
      is_lin = true;
      complexity_ = std::max(a->complexity(), b->complexity());

      trans.resize(a->nb_points() * b->nb_points());
      std::fill(trans.begin(), trans.end(), null_poly(dim()));

      std::stringstream name;
      name << "GT_PK(" << int(dim()) << ",1)";
      pgeometric_trans pgt_ = geometric_trans_descriptor(name.str());
      const poly_geometric_trans *pgt
      = dynamic_cast<const poly_geometric_trans *>(pgt_.get());

      for (size_type i = 0; i <= dim(); ++i)
        trans[cvr->structure()->ind_dir_points()[i]]
          = pgt->trans[i];
      for (size_type i2 = 0; i2 < b->nb_vertices(); ++i2)
        for (size_type i1 = 0; i1 < a->nb_vertices(); ++i1)
          vertices_.push_back(a->vertices()[i1]
                              + b->vertices()[i2] * a->nb_points());
    }
  };

  static pgeometric_trans linear_product_gt(gt_param_list &params,
        std::vector<dal::pstatic_stored_object> &dependencies) {
    GMM_ASSERT1(params.size() == 2, "Bad number of parameters : "
                << params.size() << " should be 2.");
    GMM_ASSERT1(params[0].type() == 1 && params[1].type() == 1,
                "Bad type of parameters");
    pgeometric_trans a = params[0].method();
    pgeometric_trans b = params[1].method();
    dependencies.push_back(a); dependencies.push_back(b);
    dependencies.push_back(convex_ref_product(a->convex_ref(),
                                              b->convex_ref()));
    const poly_geometric_trans *aa
      = dynamic_cast<const poly_geometric_trans *>(a.get());
    const poly_geometric_trans *bb
      = dynamic_cast<const poly_geometric_trans *>(b.get());
    GMM_ASSERT1(aa && bb, "The product of geometric transformations "
                "is only defined for polynomial ones");
    return std::make_shared<cv_pr_tl_>(aa, bb);
  }

  /* ******************************************************************** */
  /* parallelepiped transformation.                                       */
  /* ******************************************************************** */

  static pgeometric_trans QK_gt(gt_param_list &params,
        std::vector<dal::pstatic_stored_object> &) {
    GMM_ASSERT1(params.size() == 2, "Bad number of parameters : "
                << params.size() << " should be 2.");
    GMM_ASSERT1(params[0].type() == 0 && params[1].type() == 0,
                "Bad type of parameters");
    int n = int(::floor(params[0].num() + 0.01));
    int k = int(::floor(params[1].num() + 0.01));
    GMM_ASSERT1(n > 0 && n < 100 && k >= 0 && k <= 150 &&
                double(n) == params[0].num() && double(k) == params[1].num(),
                "Bad parameters");
    std::stringstream name;
    if (n == 1)
      name << "GT_PK(1," << k << ")";
    else
      name << "GT_PRODUCT(GT_QK(" << n-1 << "," << k << "),GT_PK(1,"
           << k << "))";
    return geometric_trans_descriptor(name.str());
  }

  static pgeometric_trans prism_gt(gt_param_list &params,
        std::vector<dal::pstatic_stored_object> &) {
    GMM_ASSERT1(params.size() == 2, "Bad number of parameters : "
                << params.size() << " should be 2.");
    GMM_ASSERT1(params[0].type() == 0 && params[1].type() == 0,
                "Bad type of parameters");
    int n = int(::floor(params[0].num() + 0.01));
    int k = int(::floor(params[1].num() + 0.01));
    GMM_ASSERT1(n > 0 && n < 100 && k >= 0 && k <= 150 &&
                double(n) == params[0].num() && double(k) == params[1].num(),
                "Bad parameters");
    std::stringstream name;
    name << "GT_PRODUCT(GT_PK(" << n-1 << "," << k << "),GT_PK(1,"
         << k << "))";
    return geometric_trans_descriptor(name.str());
  }

  static pgeometric_trans linear_qk(gt_param_list &params,
        std::vector<dal::pstatic_stored_object> &) {
    GMM_ASSERT1(params.size() == 1, "Bad number of parameters : "
                << params.size() << " should be 1.");
    GMM_ASSERT1(params[0].type() == 0, "Bad type of parameters");
    int n = int(::floor(params[0].num() + 0.01));
    return parallelepiped_linear_geotrans(n);
  }


  /* ******************************************************************** */
  /*	Incomplete Q2 geometric transformation for n=2 or 3.              */
  /* ******************************************************************** */
  /* By Yao Koutsawa  <yao.koutsawa@tudor.lu> 2012-12-10                  */

  struct Q2_incomplete_trans_: public poly_geometric_trans  {
    Q2_incomplete_trans_(dim_type nc) {
      cvr = Q2_incomplete_reference(nc);
      size_type R = cvr->structure()->nb_points();
      is_lin = false;
      complexity_ = 2;
      trans.resize(R);
      
      if (nc == 2) {
        std::stringstream s
          ( "1 - 2*x^2*y - 2*x*y^2 + 2*x^2 + 5*x*y + 2*y^2 - 3*x - 3*y;"
            "4*(x^2*y - x^2 - x*y + x);"
            "2*x*y*y - 2*x*x*y + 2*x*x - x*y - x;"
            "4*(x*y*y - x*y - y*y + y);"
            "4*(x*y - x*y*y);"
            "2*x*x*y - 2*x*y*y - x*y + 2*y*y - y;"
            "4*(x*y - x*x*y);"
            "2*x*x*y + 2*x*y*y - 3*x*y;");
        
        for (int i = 0; i < 8; ++i)
          trans[i] = bgeot::read_base_poly(2, s);
      } else {
        std::stringstream s
          ("1 + 2*x^2*y*z + 2*x*y^2*z + 2*x*y*z^2"
             " - 2*x^2*y - 2*x^2*z - 2*x*y^2 - 2*y^2*z - 2*y*z^2 - 2*x*z^2 - 7*x*y*z"
             " + 2*x^2 + 2*y^2 + 2*z^2 + 5*y*z + 5*x*z + 5*x*y - 3*x - 3*y - 3*z;"
           "4*( - x^2*y*z + x*y*z + x^2*z - x*z + x^2*y - x*y - x^2 + x);"
           "2*x^2*y*z - 2*x*y^2*z - 2*x*y*z^2"
             " - 2*x^2*y - 2*x^2*z + 2*x*y^2 + 2*x*z^2 + 3*x*y*z + 2*x^2 - x*y - x*z - x;"
           "4*( - x*y^2*z + x*y^2 + y^2*z + x*y*z - x*y - y^2 - y*z + y);"
           "4*(x*y^2*z - x*y^2 - x*y*z + x*y);"
           " - 2*x^2*y*z + 2*x*y^2*z - 2*x*y*z^2"
             " + 2*x^2*y - 2*x*y^2 - 2*y^2*z + 2*y*z^2 + 3*x*y*z - x*y + 2*y^2 - y*z - y;"
           "4*(x^2*y*z - x^2*y - x*y*z + x*y);"
           " - 2*x^2*y*z - 2*x*y^2*z + 2*x*y*z^2 + 2*x^2*y + 2*x*y^2 + x*y*z - 3*x*y;"
           "4*( - x*y*z^2 + x*z^2 + y*z^2 + x*y*z - x*z - y*z - z^2 + z);"
           "4*(x*y*z^2 - x*y*z - x*z^2 + x*z);"
           "4*(x*y*z^2 - x*y*z - y*z^2 + y*z);"
           "4*( - x*y*z^2 + x*y*z);"
           " - 2*x^2*y*z - 2*x*y^2*z + 2*x*y*z^2"
             " + 2*x^2*z + 2*y^2*z - 2*x*z^2 - 2*y*z^2 + 3*x*y*z - x*z - y*z + 2*z^2 - z;"
           "4*(x^2*y*z - x^2*z - x*y*z + x*z);"
           " - 2*x^2*y*z + 2*x*y^2*z - 2*x*y*z^2 + 2*x^2*z + 2*x*z^2 + x*y*z - 3*x*z;"
           "4*(x*y^2*z - y^2*z - x*y*z + y*z);"
           "4*( - x*y^2*z + x*y*z);"
           "2*x^2*y*z - 2*x*y^2*z - 2*x*y*z^2 + 2*y^2*z + 2*y*z^2 + x*y*z - 3*y*z;"
           "4*( - x^2*y*z + x*y*z);"
           "2*x^2*y*z + 2*x*y^2*z + 2*x*y*z^2 - 5*x*y*z;");
        
        for (int i = 0; i < 20; ++i)
          trans[i] = bgeot::read_base_poly(3, s);
      }
      fill_standard_vertices();
    }
  };
  
  static pgeometric_trans
    Q2_incomplete_gt(gt_param_list& params,
                     std::vector<dal::pstatic_stored_object> &dependencies) {
    GMM_ASSERT1(params.size() == 1, "Bad number of parameters : "
		<< params.size() << " should be 1.");
    GMM_ASSERT1(params[0].type() == 0, "Bad type of parameters");
    int n = int(::floor(params[0].num() + 0.01));
    GMM_ASSERT1(n == 2 || n == 3, "Bad parameter, expected value 2 or 3");
    
    dependencies.push_back(Q2_incomplete_reference(dim_type(n)));
    return std::make_shared<Q2_incomplete_trans_>(dim_type(n));
  }
  
  pgeometric_trans Q2_incomplete_geotrans(dim_type nc) {
    std::stringstream name;
    name << "GT_Q2_INCOMPLETE(" << nc << ")";
    return geometric_trans_descriptor(name.str());
  }


  /* ******************************************************************** */
  /*    Misc function.                                                    */
  /* ******************************************************************** */

  /* norm of returned vector is the ratio between the face surface on
     the reference element and the face surface on the real element.
     IT IS NOT UNITARY

     pt is the position of the evaluation point on the reference element
  */
  base_small_vector compute_normal(const geotrans_interpolation_context& c,
                                   size_type face) {
    GMM_ASSERT1(c.G().ncols() == c.pgt()->nb_points(), "dimensions mismatch");
    base_small_vector un(c.N());
    gmm::mult(c.B(), c.pgt()->normals()[face], un);
    return un;
  }

  /*
    return the local basis (i.e. the normal in the first column, and the
    tangent vectors in the other columns
  */
  base_matrix
  compute_local_basis(const geotrans_interpolation_context& c,
                      size_type face) {
    GMM_ASSERT1(c.G().ncols() == c.pgt()->nb_points(), "dimensions mismatch");
    base_small_vector up = c.pgt()->normals()[face];
    size_type P = c.pgt()->structure()->dim();

    base_matrix baseP(P, P);
    gmm::copy(gmm::identity_matrix(), baseP);
    size_type i0 = 0;
    for (size_type i=1; i < P; ++i)
      if (gmm::abs(up[i])>gmm::abs(up[i0])) i0=i;
    if (i0) gmm::copy(gmm::mat_col(baseP, 0), gmm::mat_col(baseP, i0));
    gmm::copy(up, gmm::mat_col(baseP, 0));

    base_matrix baseN(c.N(), P);
    gmm::mult(c.B(), baseP, baseN);

    /* Modified Gram-Schmidt */
    for (size_type k=0; k < P; ++k) {
      for (size_type l=0; l < k; ++l) {
        gmm::add(gmm::scaled(gmm::mat_col(baseN,l),
                             -gmm::vect_sp(gmm::mat_col(baseN,l),
                                           gmm::mat_col(baseN,k))),
                 gmm::mat_col(baseN,k));
      }
      gmm::scale(gmm::mat_col(baseN,k),
                 1./gmm::vect_norm2(gmm::mat_col(baseN,k)));
    }
    /* TODO: for cases where P < N, complete the basis */

    /* Ensure that the baseN is direct */
    if (c.N() == P && c.N()>1 && gmm::lu_det(baseN) < 0) {
      gmm::scale(gmm::mat_col(baseN,1),-1.);
    }
    return baseN;
  }




  /* ******************************************************************** */
  /*    Naming system                                                     */
  /* ******************************************************************** */

  struct geometric_trans_naming_system
    : public dal::naming_system<geometric_trans> {
    geometric_trans_naming_system() :
      dal::naming_system<geometric_trans>("GT") {
      add_suffix("PK", PK_gt);
      add_suffix("QK", QK_gt);
      add_suffix("PRISM", prism_gt);
      add_suffix("PRODUCT", product_gt);
      add_suffix("LINEAR_PRODUCT", linear_product_gt);
      add_suffix("LINEAR_QK", linear_qk);
      add_suffix("Q2_INCOMPLETE", Q2_incomplete_gt);
    }
  };

  void add_geometric_trans_name
    (std::string name, dal::naming_system<geometric_trans>::pfunction f) {
    dal::singleton<geometric_trans_naming_system>::instance().add_suffix(name,
									 f);
  }

  pgeometric_trans geometric_trans_descriptor(std::string name) {
    size_type i=0;
    return dal::singleton<geometric_trans_naming_system>::instance().method(name, i);
  }

  std::string name_of_geometric_trans(pgeometric_trans p) {
    auto &instance = dal::singleton<geometric_trans_naming_system>::instance();
    const torus_geom_trans *pgt_torus = dynamic_cast<const torus_geom_trans *>(p.get());
    if (pgt_torus) return instance.shorter_name_of_method(pgt_torus->get_original_transformation());
    return instance.shorter_name_of_method(p);
  }

  /* Fonctions pour la ref. directe.                                     */

  pgeometric_trans simplex_geotrans(size_type n, short_type k) {
    static pgeometric_trans pgt = 0;
    static size_type d = size_type(-2);
    static short_type r = short_type(-2);
    if (d != n || r != k) {
      std::stringstream name;
      name << "GT_PK(" << n << "," << k << ")";
      pgt = geometric_trans_descriptor(name.str());
      d = n; r = k;
    }
    return pgt;
  }

  pgeometric_trans parallelepiped_geotrans(size_type n, short_type k) {
    static pgeometric_trans pgt = 0;
    static size_type d = size_type(-2);
    static short_type r = short_type(-2);
    if (d != n || r != k) {
      std::stringstream name;
      name << "GT_QK(" << n << "," << k << ")";
      pgt = geometric_trans_descriptor(name.str());
      d = n; r = k;
    }
    return pgt;
  }

  static std::string name_of_linear_qk_trans(size_type dim) {
    switch (dim) {
    case 1: return "GT_PK(1,1)";
    default: return std::string("GT_LINEAR_PRODUCT(")
                           + name_of_linear_qk_trans(dim-1)
                           + std::string(",GT_PK(1,1))");
    }
  }

  pgeometric_trans parallelepiped_linear_geotrans(size_type n) {
    static pgeometric_trans pgt = 0;
    static size_type d = size_type(-2);
    if (d != n) {
      std::stringstream name(name_of_linear_qk_trans(n));
      pgt = geometric_trans_descriptor(name.str());
      d = n;
    }
    return pgt;
  }

  pgeometric_trans prism_linear_geotrans(size_type n) {
    static pgeometric_trans pgt = 0;
    static size_type d = size_type(-2);
    if (d != n) {
      std::stringstream name;
      name << "GT_LINEAR_PRODUCT(GT_PK(" << (n-1) << ", 1), GT_PK(1,1))";
      pgt = geometric_trans_descriptor(name.str());
      d = n;
    }
    return pgt;
  }

  pgeometric_trans linear_product_geotrans(pgeometric_trans pg1,
                                           pgeometric_trans pg2) {
    std::stringstream name;
    name << "GT_LINEAR_PRODUCT(" << name_of_geometric_trans(pg1) << ","
         << name_of_geometric_trans(pg2) << ")";
    return geometric_trans_descriptor(name.str());
  }

  pgeometric_trans prism_geotrans(size_type n, short_type k) {
    static pgeometric_trans pgt = 0;
    static size_type d = size_type(-2);
    static short_type r = short_type(-2);
    if (d != n || r != k) {
      std::stringstream name;
      name << "GT_PRISM(" << n << "," << k << ")";
      pgt = geometric_trans_descriptor(name.str());
      d = n; r = k;
    }
    return pgt;
  }

  pgeometric_trans product_geotrans(pgeometric_trans pg1,
                                    pgeometric_trans pg2) {
    static pgeometric_trans pgt = 0;
    static pgeometric_trans pg1_ = 0;
    static pgeometric_trans pg2_ = 0;
    if (pg1 != pg1_ || pg2 != pg2_) {
      std::stringstream name;
      name << "GT_PRODUCT(" << name_of_geometric_trans(pg1) << ","
           << name_of_geometric_trans(pg2) << ")";
      pgt = geometric_trans_descriptor(name.str());
      pg1_ = pg1; pg2_ = pg2;
    }
    return pgt;
  }

  /* ********************************************************************* */
  /*       Precomputation on geometric transformations.                    */
  /* ********************************************************************* */

  DAL_DOUBLE_KEY(pre_geot_key_, pgeometric_trans, pstored_point_tab);

  geotrans_precomp_::geotrans_precomp_(pgeometric_trans pg,
                                       pstored_point_tab ps)
    : pgt(pg), pspt(ps)
  { DAL_STORED_OBJECT_DEBUG_CREATED(this, "Geotrans precomp"); }

  void geotrans_precomp_::init_val() const {
    c.clear();
    c.resize(pspt->size(), base_vector(pgt->nb_points()));
    for (size_type j = 0; j < pspt->size(); ++j)
      pgt->poly_vector_val((*pspt)[j], c[j]);
  }

  void geotrans_precomp_::init_grad() const {
    dim_type N = pgt->dim();
    pc.clear();
    pc.resize(pspt->size(), base_matrix(pgt->nb_points() , N));
    for (size_type j = 0; j < pspt->size(); ++j)
      pgt->poly_vector_grad((*pspt)[j], pc[j]);
  }

  void geotrans_precomp_::init_hess() const {
    base_poly P, Q;
    dim_type N = pgt->structure()->dim();
    hpc.clear();
    hpc.resize(pspt->size(), base_matrix(pgt->nb_points(), gmm::sqr(N)));
    for (size_type j = 0; j < pspt->size(); ++j)
      pgt->poly_vector_hess((*pspt)[j], hpc[j]);
  }

  base_node geotrans_precomp_::transform(size_type i,
                                         const base_matrix &G) const {
    if (c.empty()) init_val();
    size_type N = G.nrows(), k = pgt->nb_points();
    base_node P(N);
    base_matrix::const_iterator git = G.begin();
    for (size_type l = 0; l < k; ++l) {
      scalar_type a = c[i][l];
      base_node::iterator pit = P.begin(), pite = P.end();
      for (; pit != pite; ++git, ++pit) *pit += a * (*git);
    }
    return P;
  }

  pgeotrans_precomp geotrans_precomp(pgeometric_trans pg,
                                     pstored_point_tab pspt,
                                     dal::pstatic_stored_object dep) {
    dal::pstatic_stored_object_key pk= std::make_shared<pre_geot_key_>(pg,pspt);
    dal::pstatic_stored_object o = dal::search_stored_object(pk);
    if (o) return std::dynamic_pointer_cast<const geotrans_precomp_>(o);
    pgeotrans_precomp p = std::make_shared<geotrans_precomp_>(pg, pspt);
    dal::add_stored_object(pk, p, pg, pspt, dal::AUTODELETE_STATIC_OBJECT);
    if (dep) dal::add_dependency(p, dep);
    return p;
  }

  void delete_geotrans_precomp(pgeotrans_precomp pgp)
  { dal::del_stored_object(pgp); }

}  /* end of namespace bgeot.                                            */

