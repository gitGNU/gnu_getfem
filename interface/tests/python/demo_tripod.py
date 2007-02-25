from getfem import *
from numarray import *

print 'This is the "modern" tripod demo, which uses the getfem model bricks'
print 'importing the mesh..',
m=Mesh('import','gid','../meshes/tripod.GiD.msh')
print 'done!'
mfu=MeshFem(m,3) # displacement
mfp=MeshFem(m,1) # pressure
mfd=MeshFem(m,1) # data
mfe=MeshFem(m,1) 
mim=MeshIm(m, Integ('IM_TETRAHEDRON(5)'))
degree = 2
linear = True
incompressible = False # ensure that degree > 1 when incompressible is on..

mfu.set_fem(Fem('FEM_PK(3,%d)' % (degree,)));
mfe.set_fem(Fem('FEM_PK_DISCONTINUOUS(3,%d,0.01)' % (degree,)))
mfd.set_fem(Fem('FEM_PK(3,0)'))
mfp.set_fem(Fem('FEM_PK_DISCONTINUOUS(3,0)'));

print 'nbcvs=%d, nbpts=%d, qdim=%d, fem = %s, nbdof=%d' % \
      (m.nbcvs(), m.nbpts(), mfu.qdim(), mfu.fem()[0].char(), mfu.nbdof())

P=m.pts()
print 'test', P[1,:]
ctop=(abs(P[1,:] - 13) < 1e-6);
cbot=(abs(P[1,:] + 10) < 1e-6);
pidtop=numarray.compress(ctop, range(0, m.nbpts()))
pidbot=numarray.compress(cbot, range(0, m.nbpts()))

ftop=m.faces_from_pid(pidtop)
fbot=m.faces_from_pid(pidbot)
NEUMANN_BOUNDARY = 1
DIRICHLET_BOUNDARY = 2

m.set_region(NEUMANN_BOUNDARY,ftop)
m.set_region(DIRICHLET_BOUNDARY,fbot)

E=1e3
Nu=0.3
Lambda = E*Nu/((1+Nu)*(1-2*Nu))
Mu =E/(2*(1+Nu))


if linear:
  b0 = MdBrick('isotropic_linearized_elasticity',mim,mfu)
  b0.set_param('lambda', Lambda);
  b0.set_param('mu', Mu);
  if (incompressible):
    b1 = MdBrick('linear incompressibility term', b0, mfp)
  else:
    b1 = b0;
else:
  # large deformation with a linearized material law.. not
  # a very good choice!
  if (incompressible):
    b0 = MdBrick('nonlinear elasticity',mim, mfu, 'Mooney Rivlin')
    b1 = MdBrick('nonlinear elasticity incompressibility term',b0,mfp)
    b0.set_param('params',[Lambda,Mu])
  else:
    b0 = MdBrick('nonlinear elasticity',mim, mfu, 'SaintVenant Kirchhoff');
    #b0 = MdBrick('nonlinear elasticity',mim, mfu, 'Ciarlet Geymonat');
    b1 = b0;
    b0.set_param('params',[Lambda,Mu]);


b2 = MdBrick('source term', b1, 1)
b2.set_param('source_term', [0,-10,0])
b3 = MdBrick('dirichlet', b2, 2, mfu, 'penalized')

mds=MdState(b3)
print 'running solve...'
b3.solve(mds, 'noisy', 'lsolver','superlu')
print 'solve done!'


VM=b0.von_mises(mds, mfe)
U=mds.state()[0:mfu.nbdof()]

# post-processing
sl=Slice(('boundary',), mfu, degree)

print 'Von Mises range: ', VM.min(), VM.max()

# export results to VTK (you can use http://mayavi.sourceforge.net/ to view these results )
# i.e. with  "mayavi -d tripod.vtk -m BandedSurfaceMap -f WarpVector"
sl.export_to_vtk('tripod.vtk', 'ascii',mfe,  VM,'Von Mises Stress', mfu, U, 'Displacement')

print 'You can view the tripod with (for example) mayavi:'
print 'mayavi -d ./tripod.vtk -f WarpVector -m BandedSurfaceMap'

mfu.save('tripod.mf','with_mesh')
U.tofile('tripod.U')

mfe.save('tripod.mfe')
VM.tofile('tripod.VM')
#memstats()