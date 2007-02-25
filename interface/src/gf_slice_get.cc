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
#include <getfem/getfem_export.h>

using namespace getfemint;

static void fmt_pt_povray(std::ofstream &f, const getfem::base_node &pt) {
  char s[100];
  if (pt.size() == 0) GMM_THROW(getfemint_error, "empty point");
  sprintf(s, "<%g,%g,%g>",pt[0],pt.size() > 1 ? pt[1] : 0., pt.size() > 2 ? pt[2] : 0.);
  f << s;
}

static void fmt_pt_povray(std::ofstream &f, const getfem::base_node &pt, const getfem::base_node &n_) {
  getfem::base_node n = 1./gmm::vect_norm2(n_) * n_;
  fmt_pt_povray(f,pt); f << ","; fmt_pt_povray(f,n);
}

static void
export_slice_to_povray(std::ofstream &f, const getfem::stored_mesh_slice& sl) {
  //time_t t = time(NULL);
  //f << "# exported for getfem " << ctime(&t) << "\n";
  f << "mesh {\n";
  size_type igncnt = 0;
  const getfem::mesh &m = sl.linked_mesh();
  for (size_type ic=0; ic < sl.nb_convex(); ++ic) {
    for (getfem::mesh_slicer::cs_simplexes_ct::const_iterator it=sl.simplexes(ic).begin();
	 it != sl.simplexes(ic).end(); ++it) {      
      if (it->dim() == 2) {
	const getfem::slice_node &A = sl.nodes(ic)[it->inodes[0]];
	const getfem::slice_node &B = sl.nodes(ic)[it->inodes[1]];
	const getfem::slice_node &C = sl.nodes(ic)[it->inodes[2]];
	getfem::slice_node::faces_ct common_faces = (A.faces & B.faces & C.faces);
	size_type fnum = 0;
	for (; common_faces.any(); ++fnum) if (common_faces[fnum]) break;
	if (fnum < m.structure_of_convex(sl.convex_num(ic))->nb_faces()) {
	  f << "smooth_triangle {";
	  fmt_pt_povray(f,A.pt,m.normal_of_face_of_convex(sl.convex_num(ic),fnum,A.pt_ref));
	  fmt_pt_povray(f,B.pt,m.normal_of_face_of_convex(sl.convex_num(ic),fnum,B.pt_ref));
	  fmt_pt_povray(f,C.pt,m.normal_of_face_of_convex(sl.convex_num(ic),fnum,C.pt_ref));
	  f << "}\n";
	} else {
	  f << "triangle {";
	  fmt_pt_povray(f,A.pt);
	  fmt_pt_povray(f,B.pt);
	  fmt_pt_povray(f,C.pt);
	  f << "}\n";
	}
      } else ++igncnt;
    }
  }
  f << "}\n";
  if (igncnt) cout << igncnt << " simplexes of dim != 2 ignored\n";
}

static std::string get_vtk_dataset_name(getfemint::mexargs_in &in, int count) {
  std::string s;
  if (in.remaining() && in.front().is_string()) {
    s = in.pop().to_string();
  } else {
    std::stringstream name; name << "dataset" << count;
    s = name.str();
  }
  for (size_type i=0; i < s.length(); ++i)
    if (!isalnum(s[i])) s[i] = '_';
  return s;
}

static std::string get_dx_dataset_name(getfemint::mexargs_in &in) {
  std::string s;
  if (in.remaining() && in.front().is_string()) {
    s = in.pop().to_string();
  }
  for (size_type i=0; i < s.length(); ++i)
    if (!isalnum(s[i])) s[i] = '_';
  return s;
}

template <typename T> static void 
interpolate_convex_data(const getfem::stored_mesh_slice *sl,
			const garray<T> &u, getfemint::mexargs_out& out) {
  assert(u.dim(u.ndim()-1) == sl->linked_mesh().convex_index().last_true()+1);
  array_dimensions ad; 
  for (unsigned i=0; i < u.ndim()-1; ++i) ad.push_back(u.dim(i));
  ad.push_back(sl->nb_points());
  garray<T> w = out.pop().create_array(ad, T());
  size_type q = u.size() / u.dim(u.ndim()-1);
  size_type pos = 0;
  for (size_type i=0; i < sl->nb_convex(); ++i) {
    for (unsigned j=0; j < q; ++j) {
      T v = u[(sl->convex_num(i))*q + j];
      for (unsigned k=0; k < sl->nodes(i).size(); ++k) {
	w[pos++] = v;
      }
    }
  }
  assert(pos == w.size());
}

/*MLABCOM

  FUNCTION [...] = gf_slice_get(slice SL, [operation [, args]])

  @RDATTR SLICE:GET('dim')
  @GET SLICE:GET('area')
  @GET SLICE:GET('cvs')
  @RDATTR SLICE:GET('nbpts')
  @GET SLICE:GET('pts')
  @RDATTR SLICE:GET('nbsplxs')
  @GET SLICE:GET('splxs')
  @GET SLICE:GET('edges')
  @GET SLICE:GET('interpolate_convex_data')
  @GET SLICE:GET('linked mesh')
  @GET SLICE:GET('export to vtk')
  @GET SLICE:GET('export to pov')
  @GET SLICE:GET('export to dx')
  @GET SLICE:GET('memsize')

  $Id$
MLABCOM*/


void gf_slice_get(getfemint::mexargs_in& in, getfemint::mexargs_out& out)
{
  if (in.narg() < 2) {
    THROW_BADARG( "Wrong number of input arguments");
  }
  getfemint_mesh_slice *mi_sl = in.pop().to_getfemint_mesh_slice();
  const getfem::stored_mesh_slice *sl = &mi_sl->mesh_slice();
  std::string cmd                  = in.pop().to_string();
  if (check_cmd(cmd, "dim", in, out, 0, 0, 0, 1)) {
    /*@RDATTR SLICE:GET('dim')
      Return the dimension of the slice (2 for a 2D mesh, etc..).
      @*/
    out.pop().from_integer(sl->dim());
  } else if (check_cmd(cmd, "area", in, out, 0, 0, 0, 1)) {
    /*@GET SLICE:GET('area')
      Return the area of the slice.
      @*/
    getfem::slicer_compute_area s; sl->replay(s);
    out.pop().from_scalar(s.area());
  } else if (check_cmd(cmd, "cvs", in, out, 0, 0, 0, 1)) {
    /*@GET CVLST=SLICE:GET('cvs')
      Return the list of convexes of the original mesh contained in the slice.
      @*/
    iarray w = out.pop().create_iarray_h(sl->nb_convex());
    for (size_type i=0; i < sl->nb_convex(); ++i) w[i] = sl->convex_num(i) + config::base_index();
  } else if (check_cmd(cmd, "nbpts", in, out, 0, 0, 0, 1)) {
    /*@RDATTR SLICE:GET('nbpts')
      Return the number of points in the slice.
      @*/
    out.pop().from_integer(sl->nb_points());
  } else if (check_cmd(cmd, "nbsplxs", in, out, 0, 1, 0, 1)) {
    /*@RDATTR SLICE:GET('nbsplxs' [,@int DIM])
      Return the number of simplexes in the slice.
      
      Since the slice may contain points (simplexes of dim 0), segments
      (simplexes of dimension 1), triangles etc., the result is a vector of size
      SLICE:GET('dim')+1 , except if the optional argument DIM is used. 
      @*/

    std::vector<size_type> v; sl->nb_simplexes(v);
    if (in.remaining()) {
      size_type i= in.pop().to_integer(0,100);
      out.pop().from_integer(i < v.size() ? v[i] : 0);
    } else {
      out.pop().from_ivector(v);
    }
  } else if (check_cmd(cmd, "pts", in, out, 0, 0, 0, 1)) {
    /*@GET P=SLICE:GET('pts')
      Return the list of point coordinates.
      @*/
    darray w = out.pop().create_darray(sl->dim(), sl->nb_points());
    for (size_type ic=0, cnt=0; ic < sl->nb_convex(); ++ic) {
      for (getfem::mesh_slicer::cs_nodes_ct::const_iterator it=sl->nodes(ic).begin();
           it != sl->nodes(ic).end(); ++it) {
        for (size_type k=0; k < sl->dim(); ++k)
          w[cnt++] = (*it).pt[k];
      }
    }
  } else if (check_cmd(cmd, "splxs", in, out, 1, 1, 0, 2)) {
    /*@GET [S,CV2SPLX]=SLICE:GET('splxs', @int DIM)
      Return the list of simplexes of dimension DIM.
      
      On output, S has DIM+1 rows, each column contains the point numbers of a
      simplex.  The vector CV2SPLX can be used to find the list of simplexes for
      any convex stored in the slice. For example @MATLAB{S(:,CV2SPLX(4):CV2SPLX(5)-1)}@PYTHON{S[:,CV2SPLX[4]:CV2SPLX[5]]}
      gives the list of simplexes for the fourth convex.
      @*/
    size_type sdim = in.pop().to_integer(0,sl->dim());
    iarray w = out.pop().create_iarray(sdim+1, sl->nb_simplexes(sdim));
    size_type Scnt = 0;
    iarray cv2splx;
    if (out.remaining()) {
      cv2splx = out.pop().create_iarray_h(sl->nb_convex()+1); Scnt = config::base_index();
    }
    for (size_type ic=0, cnt=0, pcnt=0; ic < sl->nb_convex(); ++ic) {
      size_type scnt = 0;
      for (getfem::mesh_slicer::cs_simplexes_ct::const_iterator it=sl->simplexes(ic).begin();
           it != sl->simplexes(ic).end(); ++it) {
        if ((*it).dim() == sdim) {
          for (size_type k=0; k < sdim+1; ++k)
            w[cnt++] = (*it).inodes[k] + pcnt + config::base_index();
	  scnt++; 
	}
      }      
      pcnt += sl->nodes(ic).size();
      if (Scnt) {	
	cv2splx[ic] = Scnt; Scnt+=scnt;
      }
    }
    if (Scnt) cv2splx[sl->nb_convex()] = Scnt;
  } else if (check_cmd(cmd, "edges", in, out, 0, 0, 0, 3)) {
    /*@GET [mat P, ivec E1, ivec E2]=SLICE:GET('edges')
      Return the edges of the linked mesh contained in the slice.
      
      P contains the list of all edge vertices, E1 contains the indices of each
      mesh edge in P, and E2 contains the indices of each "edges" which is on the
      border of the slice. This function is useless except for post-processing
      purposes.
      @*/
    getfem::mesh m; 
    dal::bit_vector slice_edges;
    getfem::mesh_slicer slicer(sl->linked_mesh());
    getfem::slicer_build_edges_mesh action(m,slice_edges); 
    slicer.push_back_action(action); slicer.exec(*sl);

    /* return a point list, a connectivity array, and optionnaly a list of edges with are part of the slice */
    double nan = get_NaN();
    dal::bit_vector bv = m.points().index();
    darray P = out.pop().create_darray(m.dim(), bv.last_true()+1);
    iarray T1 = out.pop().create_iarray(2, m.nb_convex() - slice_edges.card());
    iarray T2 = out.pop().create_iarray(2, slice_edges.card());
    for (size_type j = 0; j < bv.last_true()+1; j++) {
      for (size_type i = 0; i < m.dim(); i++) {
	P(i,j) = (bv.is_in(j)) ? (m.points()[j])[i] : nan;
      }
    }
    iarray::iterator itt1=T1.begin(), itt2=T2.begin();
    for (dal::bv_visitor cv(m.convex_index()); !cv.finished(); ++cv) {
      if (!slice_edges[cv]) {  
	gmm::copy_n(m.ind_points_of_convex(cv).begin(), 2, itt1); 
	itt1[0] += config::base_index(); itt1[1] += config::base_index(); itt1 += 2;
      } else {
	gmm::copy_n(m.ind_points_of_convex(cv).begin(), 2, itt2); 
	itt2[0] += config::base_index(); itt2[1] += config::base_index(); itt2 += 2;
      }
    }
  } else if (check_cmd(cmd, "interpolate_convex_data", in, out, 1, 1, 0, 1)) {
    /*@GET Usl=SLICE:GET('interpolate_convex_data', Ucv)
      Interpolate data given on each convex of the mesh to the slice
      nodes.

      The input array Ucv may have any number of dimensions, but its
      last dimension should be equal to MESH:GET('max cvid').

      Example of use: SLICE:GET('interpolate_convex_data', MESH:GET('quality'))
      @*/
    in.front().check_trailing_dimension(sl->linked_mesh().convex_index().last_true()+1);
    if (in.front().is_complex()) 
      interpolate_convex_data(sl, in.pop().to_darray(), out);
    else interpolate_convex_data(sl, in.pop().to_carray(), out);
  } else if (check_cmd(cmd, "linked mesh", in, out, 0, 0, 0, 1)) {
    /*@GET m=SLICE:GET('linked mesh')
      Return the mesh on which the slice was taken.
      @*/
    out.pop().from_object_id(mi_sl->linked_mesh_id(), MESH_CLASS_ID);
  } else if (check_cmd(cmd, "memsize", in, out, 0, 0, 0, 1)) {
    /*@GET ms=SLICE:GET('memsize')
      Return the amount of memory (in bytes) used by the slice object.
      @*/
    out.pop().from_integer(sl->memsize());
  } else if (check_cmd(cmd, "export to vtk",in, out, 1, -1, 0, 0)) {
    /*@GET SLICE:GET('export to vtk', @str FILENAME ... [, 'ascii'][, 'edges'] ...)
      Export a slice to VTK.

      Following the file name, you may use any of the following options:

        - if 'ascii' is not used, the file will contain binary data (non portable, but fast).

        - if 'edges' is used, the edges of the original mesh will be written instead of the slice content.

      More than one dataset may be written, just list them. Each dataset
      consists of either:

        * a field interpolated on the slice, followed by an optional name.
        * a mesh_fem and a field, followed by an optional name.

      The field might be a scalar field, a vector field or a tensor field.

      examples:
      @SLICE:GET('export to vtk', 'test.vtk', Uslice, 'first_dataset', mf, U2, 'second_dataset')
      @SLICE:GET('export to vtk', 'test.vtk', 'ascii', mf, U2)
      @SLICE:GET('export to vtk', 'test.vtk', 'edges', 'ascii', Uslice)
      @*/
    std::string fname = in.pop().to_string();
    bool ascii = false;
    bool edges = false;
    while (in.remaining() && in.front().is_string()) {
      std::string cmd2 = in.pop().to_string();
      if (cmd_strmatch(cmd2, "ascii"))
        ascii = true;
      else if (cmd_strmatch(cmd2, "edges"))
        edges = true;
      else THROW_BADARG("expecting 'ascii' or 'edges', got " << cmd2);
    }
    getfem::vtk_export exp(fname, ascii);
    getfem::stored_mesh_slice sl_edges;
    const getfem::stored_mesh_slice *vtk_slice = sl;
    getfem::mesh m_edges;
    if (edges) {
      vtk_slice = &sl_edges;
      dal::bit_vector slice_edges;
      getfem::mesh_slicer slicer(sl->linked_mesh());
      getfem::slicer_build_edges_mesh action(m_edges,slice_edges);
      slicer.push_back_action(action); slicer.exec(*sl);
      sl_edges.build(m_edges, getfem::slicer_none());
    }
    exp.exporting(*vtk_slice);
    exp.write_mesh();
    int count = 1;
    if (in.remaining()) {
      do {
        if (in.remaining() >= 2 && in.front().is_mesh_fem()) {
          const getfem::mesh_fem &mf = 
	    *in.pop().to_const_mesh_fem();
          darray U = in.pop().to_darray(); in.last_popped().check_trailing_dimension(mf.nb_dof());
          exp.write_point_data(mf,U,get_vtk_dataset_name(in, count));
        } else if (in.remaining()) {
          darray slU = in.pop().to_darray(); in.last_popped().check_trailing_dimension(vtk_slice->nb_points());
          exp.write_sliced_point_data(slU,get_vtk_dataset_name(in, count));
        } else THROW_BADARG("don't know what to do with this argument")
            count+=1;
      } while (in.remaining());
    }
  } else if (check_cmd(cmd, "export to pov",in, out, 1, 1, 0, 0)) {
    /*@GET SLICE:GET('export to pov', @str FILENAME, ...)
      Export a the triangles of the slice to POV-RAY.
      @*/
    std::string fname = in.pop().to_string();
    std::ofstream f(fname.c_str());
    export_slice_to_povray(f,*sl);
  } else if (check_cmd(cmd, "export to dx", in, out, 1, -1, 0, 0)) {
    /*@GET SLICE:GET('export to dx', @str FILENAME, ...)
      Export a slice to OpenDX.

      Following the file name, you may use any of the following
      options:<Par>

        - if 'ascii' is not used, the file will contain binary data
        (non portable, but fast).
  
        - if 'edges' is used, the edges of the original mesh will be
        written instead of the slice content.

        - if 'append' is used, the opendx file will not be
        overwritten, and the new data will be added at the end of the
        file.

      More than one dataset may be written, just list them. Each dataset
      consists of either:<Par>

        - a field interpolated on the slice (scalar, vector or
        tensor), followed by an optional name.

        - a mesh_fem and a field, followed by an optional name.
      @*/
    std::string fname = in.pop().to_string();
    bool ascii = false;
    bool edges = false;
    bool append = false;
    std::string mesh_name, serie_name;
    while (in.remaining() && in.front().is_string()) {
      std::string cmd2 = in.pop().to_string();
      if (cmd_strmatch(cmd2, "ascii"))
        ascii = true;
      else if (cmd_strmatch(cmd2, "edges"))
        edges = true;
      else if (cmd_strmatch(cmd2, "append"))
        append = true;
      else if (cmd_strmatch(cmd2, "as") && in.remaining())
	mesh_name = in.pop().to_string();
      else if (cmd_strmatch(cmd2, "serie") && in.remaining())
	serie_name = in.pop().to_string();
      else THROW_BADARG("expecting 'ascii' or 'edges' or 'append' or 'as', got " << cmd2);
    }
    getfem::dx_export exp(fname, ascii, append);

    exp.exporting(*sl, mesh_name.c_str());
    exp.write_mesh();
    if (edges) exp.exporting_mesh_edges();
    if (in.remaining()) {
      do {
        if (in.remaining() >= 2 && in.front().is_mesh_fem()) {
          const getfem::mesh_fem &mf = 
	    *in.pop().to_const_mesh_fem();
          darray U = in.pop().to_darray(); in.last_popped().check_trailing_dimension(mf.nb_dof());
          exp.write_point_data(mf,U,get_dx_dataset_name(in));
        } else if (in.remaining()) {
          darray slU = in.pop().to_darray(); in.last_popped().check_trailing_dimension(sl->nb_points());
          exp.write_sliced_point_data(slU,get_dx_dataset_name(in));
        } else THROW_BADARG("don't know what to do with this argument");
        if (serie_name.size()) exp.serie_add_object(serie_name);
      } while (in.remaining());
    }
  } else bad_cmd(cmd);
}
