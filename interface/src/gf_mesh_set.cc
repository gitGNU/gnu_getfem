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

#include <getfemint_misc.h>
#include <getfemint_mesh.h>

using namespace getfemint;

static void check_empty_mesh(const getfem::mesh *pmesh)
{
  if (pmesh->dim() == bgeot::dim_type(-1) || pmesh->dim() == 0) {
    THROW_ERROR( "mesh object has an invalid dimension");
  }
}

static void set_region(getfem::mesh &mesh, getfemint::mexargs_in& in)
{
  unsigned boundary_num  = in.pop().to_integer(1,100000);
  iarray v               = in.pop().to_iarray(2,-1);

  getfem::mesh_region &rg = mesh.region(boundary_num);
  /* loop over the edges of mxEdge */
  for (size_type j=0; j < v.getn(); j++) {
    size_type cv = size_type(v(0,j))-config::base_index();
    
    size_type f  = size_type(v(1,j))-config::base_index();
    if (!mesh.convex_index().is_in(cv)) {
      THROW_BADARG( "Invalid convex number '" << cv+config::base_index() << "' at column " << j+config::base_index());
    }
    if (f >= mesh.structure_of_convex(cv)->nb_faces()) {
      THROW_BADARG( "Invalid face number '" << f+config::base_index() << "' at column " << j+config::base_index());
    }
    if (f == size_type(-1)) rg.add(cv);
    else                    rg.add(cv, f);
  }
}

static void intersect_regions(getfem::mesh &mesh, getfemint::mexargs_in& in) {
  unsigned ir1 = in.pop().to_integer(1,100000);
  unsigned ir2 = in.pop().to_integer(1,100000);
  getfem::mesh_region &r1 = mesh.region(ir1);
  getfem::mesh_region &r2 = mesh.region(ir2);
  r1 = getfem::mesh_region::intersection(r1, r2);
}

static void merge_regions(getfem::mesh &mesh, getfemint::mexargs_in& in) {
  unsigned ir1 = in.pop().to_integer(1,100000);
  unsigned ir2 = in.pop().to_integer(1,100000);
  getfem::mesh_region &r1 = mesh.region(ir1);
  getfem::mesh_region &r2 = mesh.region(ir2);
  r1 = getfem::mesh_region::merge(r1, r2);
}

static void substract_regions(getfem::mesh &mesh, getfemint::mexargs_in& in) {
  unsigned ir1 = in.pop().to_integer(1,100000);
  unsigned ir2 = in.pop().to_integer(1,100000);
  getfem::mesh_region &r1 = mesh.region(ir1);
  getfem::mesh_region &r2 = mesh.region(ir2);
  r1 = getfem::mesh_region::substract(r1, r2);
}


/*MLABCOM
  FUNCTION [x] = gf_mesh_set(mesh M, operation [, args])

    General function for modification of a mesh object.

    @SET MESH:SET('pts')
    @SET MESH:SET('add point')
    @SET MESH:SET('del point')
    @SET MESH:SET('add convex')
    @SET MESH:SET('del convex')
    @SET MESH:SET('del convex of dim')
    @SET MESH:SET('translate')
    @SET MESH:SET('transform')
    @SET MESH:SET('merge')
    @SET MESH:SET('optimize structure')
    @SET MESH:SET('refine')
    @SET MESH:SET('region')
    @SET MESH:SET('region_merge')
    @SET MESH:SET('region_intersect')
    @SET MESH:SET('region_substract')
    @SET MESH:SET('delete region')
  MLABCOM*/
void gf_mesh_set(getfemint::mexargs_in& in, getfemint::mexargs_out& out)
{
  if (in.narg() < 2) {
    THROW_BADARG( "Wrong number of input arguments");
  }

  getfem::mesh *pmesh = in.pop().to_mesh();
  std::string cmd            = in.pop().to_string();
  /*@SET [IDX]=MESH:SET('pts', @mat P)
    Replace the coordinates of the mesh points with those given in P.
    @*/
  if (check_cmd(cmd, "pts", in, out, 1, 1, 0, 1)) {
    darray P = in.pop().to_darray(pmesh->dim(), 
				  pmesh->points().index().last_true()+1);
    for (dal::bv_visitor i(pmesh->points().index()); !i.finished(); ++i) {
      for (unsigned k=0; k < pmesh->dim(); ++k)
	pmesh->points()[i][k] = P(k,i);
    }
  } if (check_cmd(cmd, "add point", in, out, 1, 1, 0, 1)) {
    /*@SET [IDX]=MESH:SET('add point', @mat PT)
    Insert new points in the mesh and return their #id.

    PT should be an [n x m] matrix , where n is the mesh dimension,
    and m is the number of points that will be added to the mesh. On
    output, IDX contains the indices of these new points.
    <Par>
    Remark: if some points are already part of the mesh (with a small
    tolerance of approximately 1e-8), they won't be inserted again,
    and IDX will contain the previously assigned indices of the
    points. @*/
    check_empty_mesh(pmesh);
    darray v = in.pop().to_darray(pmesh->dim(), -1);
    iarray w = out.pop().create_iarray_h(v.getn());
    for (size_type j=0; j < v.getn(); j++) {
      w[j] = pmesh->add_point(v.col_to_bn(j)) + config::base_index();
    }
  } else if (check_cmd(cmd, "del point", in, out, 1, 1, 0, 0)) {
    /*@SET MESH:SET('del point', @ivec PIDLST)
    Removes one or more points from the mesh.

    PIDLST should contain the 
    point #id, such as the one returned by the 'add point' command.
    @*/
    check_empty_mesh(pmesh);
    iarray v = in.pop().to_iarray();
    
    for (size_type j=0; j < v.size(); j++) {
      id_type id = v[j]-config::base_index();
      if (pmesh->is_point_valid(id)) {
	THROW_ERROR( "Can't remove point " << id+config::base_index()
		     << ": a convex is still attached to it.");
      }
      pmesh->sup_point(id);
    }
  } else if (check_cmd(cmd, "add convex", in, out, 2, 2, 0, 1)) {
    /*@SET IDX = MESH:SET('add convex', @geotrans CVTR, @mat CVPTS)
    Add a new convex into the mesh.

    The convex structure (triangle, prism,...) is given by CVTR
    (obtained with GEOTRANS:INIT('...')), and its points are given by
    the columns of CVPTS. On return, IDX contains the convex number.
    CVPTS might be a 3-dimensional array in order to insert more than
    one convex (or a two dimensional array correctly shaped according
    to Fortran ordering). 
    @*/
    check_empty_mesh(pmesh);
    bgeot::pgeometric_trans pgt = in.pop().to_pgt();
    darray v = in.pop().to_darray(pmesh->dim(), pgt->nb_points(), -1);
    iarray w = out.pop().create_iarray_h(v.getp());

    std::vector<getfemint::id_type> qp(pgt->nb_points());
    /* loop over convexes */
    for (size_type k=0; k < v.getp(); k++) {
      /* loop over convex points */
      for (size_type j=0; j < v.getn(); j++) {
	qp[j] = pmesh->add_point(v.col_to_bn(j,k));
      }
      id_type cv_id = pmesh->add_convex(pgt, qp.begin());
      w[k] = cv_id+config::base_index();
    }
  } else if (check_cmd(cmd, "del convex", in, out, 1, 1, 0, 0)) {
    /*@SET MESH:SET('del convex', IDX)
    Remove one or more convexes from the mesh.

    IDX should contain the convexes #ids, such as the ones returned bu
    MESH:SET('add convex'). @*/
    check_empty_mesh(pmesh);
    iarray v = in.pop().to_iarray();
    
    for (size_type j=0; j < v.size(); j++) {
      id_type id = v[j]-config::base_index();
      if (pmesh->convex_index().is_in(id)) {
	pmesh->sup_convex(id);
      } else {
	THROW_ERROR("can't delete convex " << id+config::base_index() << ", it is not part of the mesh");
      }
    }
  } else if (check_cmd(cmd, "del convex of dim", in, out, 1, 1, 0, 0)) {  
    /*@SET MESH:SET('del convex of dim', @ivec DIM)
      Remove all convexes of dimension listed in DIM.

      For example MESH:SET('del convex of dim', [1,2]) remove all line
      segments, triangles and quadrangles.
      @*/
    dal::bit_vector bv = in.pop().to_bit_vector(NULL, 0);
    for (dal::bv_visitor_c cv(pmesh->convex_index()); !cv.finished(); ++cv) {
      if (bv.is_in(pmesh->structure_of_convex(cv)->dim())) pmesh->sup_convex(cv);
    }
  } else if (check_cmd(cmd, "translate", in, out, 1, 1, 0, 0)) {
    /*@SET MESH:SET('translate', @vec V)
    Translates each point of the mesh from V.
    @*/
    check_empty_mesh(pmesh);
    darray v = in.pop().to_darray(pmesh->dim(),1);
    pmesh->translation(v.col_to_bn(0));
  } else if (check_cmd(cmd, "transform", in, out, 1, 1, 0, 0)) {
    /*@SET MESH:SET('transform', @mat T)
    Applies the matrix T to each point of the mesh.

    Note that T is not required to be a NxN matrix (with
    N=MESH:GET('dim')). Hence it is possible to transform a 2D mesh
    into a 3D one (and reciprocally).
    @*/
    check_empty_mesh(pmesh);
    darray v = in.pop().to_darray(-1,-1); //pmesh->dim());
    pmesh->transformation(v.row_col_to_bm());
  } else if (check_cmd(cmd, "boundary", in, out, 2, 2, 0, 0) ||
	     check_cmd(cmd, "region", in, out, 2, 2, 0, 0)) {
    /*@SET MESH:SET('region', @int rnum, @dmat CVFLST) 
    Assigns the region number rnum to the convex faces stored in each
    column of the matrix CVFLST. 

    The first row of CVFLST contains a convex number, and the second
    row contains a face number in the convex (or @MATLAB{0}@PYTHON{-1}
    for the whole convex -- regions are usually used to store a list
    of convex faces, but you may also use them to store a list of
    convexes).
    @*/
    set_region(*pmesh, in);
  } else if (check_cmd(cmd, "region_intersect", in, out, 2, 2, 0, 0)) {
    /*@SET MESH:SET('region_intersect', @int R1, @int R2) 
    Replace the region number R1 with its intersection with region
    number R2. 
    @*/
    intersect_regions(*pmesh,in);
  } else if (check_cmd(cmd, "region_merge", in, out, 2, 2, 0, 0)) {
    /*@SET MESH:SET('region_merge', @int R1, @int R2) 
      Merge region number R2 into region number R1.
    @*/
    merge_regions(*pmesh,in);
  } else if (check_cmd(cmd, "region_substract", in, out, 2, 2, 0, 0)) {
    /*@SET MESH:SET('region_substract', @int R1, @int R2) 
    Replace the region number R1 with its difference with region number R2. 
    @*/
    substract_regions(*pmesh,in);
  } else if (check_cmd(cmd, "delete boundary", in, out, 1, 1, 0, 0) || 
	     check_cmd(cmd, "delete region", in, out, 1, 1, 0, 0)) {
    /*@SET MESH:SET('delete region', @ivec RLST)
      Remove the regions whose #ids are listed in RLST
      @*/
    dal::bit_vector lst = in.pop().to_bit_vector(&pmesh->regions_index(),0);
    pmesh->sup_region(1);
    for (dal::bv_visitor b(lst); !b.finished(); ++b)
      pmesh->sup_region(b);
  } else if (check_cmd(cmd, "merge", in, out, 1, 1, 0, 0)) {

    /*@SET MESH:SET('merge', @mesh M2) 
    Merge with the mesh M2.

    Overlapping points won't be duplicated. If M2 is a mesh_fem
    object, its linked mesh will be used. @*/
    const getfem::mesh *pmesh2 = in.pop().to_const_mesh();
    for (dal::bv_visitor cv(pmesh2->convex_index()); !cv.finished(); ++cv)
      pmesh->add_convex_by_points(pmesh2->trans_of_convex(cv), pmesh2->points_of_convex(cv).begin());
  } else if (check_cmd(cmd, "optimize structure", in, out, 0, 0, 0, 0)) {
    /*@SET MESH:SET('optimize structure')
    Reset point and convex numbering.

    After optimisation, the points (resp. convexes) will be consecutively numbered from @MATLAB{1 to MESH:GET('max pid') (resp. MESH:GET('max cvid'))}@PYTHON{0 to MESH:GET('max pid')-1 (resp. MESH:GET('max cvid')-1)}.
    @*/
    pmesh->optimize_structure();
  } else if (check_cmd(cmd, "refine", in, out, 0, 1, 0, 0)) {
    /*@SET MESH:SET('refine' [, @ivec CVLST])
    Use a Bank strategy for mesh refinement. 

    If CVLST is not given, the whole mesh is refined. Note that the regions, and the finite element methods and integration methods of the mesh_fem and mesh_im objects linked to this mesh will be automagically refined.
    @*/
    dal::bit_vector bv = pmesh->convex_index(); 
    if (in.remaining()) bv = in.pop().to_bit_vector(&pmesh->convex_index());
    pmesh->Bank_refine(bv);
  } else bad_cmd(cmd);
}