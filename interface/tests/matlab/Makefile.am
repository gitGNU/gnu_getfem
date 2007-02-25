SUBDIRS = private

if BUILDMEX
TESTS = $(srcdir)/check_all.sh
else
TESTS = 
endif


EXTRA_DIST= \
	check_all.sh \
	axrot_matrix.m \
	check_all.m \
	check_all.sh \
	check_asm.m \
	check_bricks.m \
	check_fem.m \
	check_geotrans.m \
	check_integ.m \
	check_mesh_fem.m \
	check_oo.m \
	check_plot.m \
	check_slices.m \
	check_solve.m \
	check_spmat.m \
	check_workspace.m \
	demo_bilaplacian.m \
	demo_laplacian.m \
	demo_laplacian1D.m \
	demo_nonlinear_elasticity.m \
	demo_nonlinear_elasticity_anim.m \
	demo_plasticity.m \
	demo_refine.m \
	demo_stokes_2D_poiseuille.m \
	demo_stokes_2D_poiseuille_arc.m \
	demo_stokes_2D_tube.m \
	demo_stokes_3D_tank.m \
	demo_stokes_3D_tank_alt.m \
	demo_stokes_3D_tank_draw.m \
	demo_tripod.m \
	demo_tripod_alt.m \
	demo_tripod_slice_anim.m \
	demo_wave2D.m \
	demo_wave2D_alt.m \
	test_argyris.m \
	tripod_anim.m \
	tutorial1.m \
	\
	private/assert.m \
	private/asserterr.m

TESTS_ENVIRONMENT = \
	MATLABPATH=@abs_top_builddir@/src:@abs_top_builddir@/tests/matlab:@abs_top_srcdir@/src:@abs_top_srcdir@/tests/matlab \
	MATLAB_ROOT=@MATLAB_ROOT@
	PATH=@abs_top_builddir@/src:$(PATH)
#	LD_LIBRARY_PATH=$(prefix)/lib:@GETFEMLIB@:@abs_top_builddir@/src:$(LD_LIBRARY_PATH) 

toolboxdir=@TOOLBOXDIR@/tests
toolbox_SCRIPTS=$(EXTRA_DIST)