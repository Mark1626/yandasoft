from params import Params

# tclean(vis='sis14_twhya_calibrated_flagged.ms',
#        imagename='first_image',
#        field='5',
#        spw='',
#        specmode='mfs',
#        deconvolver='hogbom',
#        nterms=1,
#        gridder='standard',
#        imsize=[250,250],
#        cell=['0.1arcsec'],
#        weighting='natural',
#        threshold='0mJy',
#        niter=5000,
#        interactive=True,
#        savemodel='modelcolumn')

import os

class Gridder:
    def __init__(self, gridder_type):
        self.params = Params(prefix="Cimager.gridder")
        self.gridder_type = gridder_type

    def snapshot_imaging(self):
        p = self.params
        p.update("snapshotimaging", "false")
        p.update("snapshotimaging.wtolerance", 2600)
        p.update("snapshotimaging.longtrack", "true")
        p.update("snapshotimaging.clipping", 0.01)

    def update(self, key, value):
        self.params.update("%s.%s" % (self.gridder_type, key), value)

    def serialize(self):
        return self.params.serialize()

class WProjectGridder(Gridder):
    def __init__(
        self, params
    ):
        Gridder.__init__(self, "WProject")

    # def serialize(self):


# def imager(
#     vis,
#     imagename,
#     imagetype='fits',
#     shape=512,
#     cellsize='2arcsec',
#     workers=3,
#     nchan_per_core=12,
#     gridder=DefaultGridder,
#     solve=DefaultSolver,
# ):

#     os.system("mpiexec imager -n 4 imager.in > imager-run.log")

