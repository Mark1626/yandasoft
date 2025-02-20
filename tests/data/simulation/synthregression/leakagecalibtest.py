# regression tests of calibrator
# some fixed parameters are given in calibratortest_template.in

from synthprogrunner import *

#from askapdev.rbuild import setup
#from askapdev.rbuild.dependencies import Dependency

#dep = Dependency(silent=False)
#dep.DEPFILE = "../../dependencies"
#dep.add_package()

#import askap.parset

def analyseResult(spr):
   '''
      spr - synthesis program runner (to run imageStats)

      throws exceptions if something is wrong, otherwise just
      returns
   '''
   src_offset = 0.004/math.pi*180.
   psf_peak=[-172.5,-45]
   true_peak=sinProjection(psf_peak,src_offset,src_offset)
   stats = spr.imageStats('image.field1.restored')
   print("Statistics for restored image: ",stats)
   disterr = getDistance(stats,true_peak[0],true_peak[1])*3600.
   if disterr > 8:
      raise RuntimeError("Offset between true and expected position exceeds 1 cell size (8 arcsec), d=%f, true_peak=%s" % (disterr,true_peak))
   # as polarisation conversion is now fixed in the component-based measurement equation we have exactly the same flux value as in the simulation parset
   if abs(stats['peak']-1.)>0.1:
      raise RuntimeError("Peak flux in the image is notably different from 1 Jy, F=%f" % stats['peak'])

   stats = spr.imageStats('image.field1')
   print("Statistics for modelimage: ",stats)
   disterr = getDistance(stats,true_peak[0],true_peak[1])*3600.
   if disterr > 8:
      raise RuntimeError("Offset between true and expected position exceeds 1 cell size (8 arcsec), d=%f, true_peak=%s" % (disterr,true_peak))

   stats = spr.imageStats('psf.field1')
   print("Statistics for psf image: ",stats)
   disterr = getDistance(stats,psf_peak[0],psf_peak[1])*3600.
   if disterr > 8:
      raise RuntimeError("Offset between true and expected position exceeds 1 cell size (8 arcsec), d=%f, true_peak=%s" % (disterr,true_peak))

   stats = spr.imageStats('weights.field1')
   print("Statistics for weight image: ",stats)
   if abs(stats['rms']-stats['peak'])>0.1 or abs(stats['rms']-stats['median'])>0.1 or abs(stats['peak']-stats['median'])>0.1:
      raise RuntimeError("Weight image is expected to be constant for WProject and WStack gridders")

   stats = spr.imageStats('residual.field1')
   print("Statistics for residual image: ",stats)
   if stats['rms']>0.01 or abs(stats['median'])>0.0001:
      raise RuntimeError("Residual image has too high rms or median. Please verify")


def loadParset(fname):
    """
       Helper method to read parset file with gains into a python dictionary. Most likely we wouldn't need this method
       if importing of py-parset was a bit more straightforward.

       fname - file name of the parset file to read

       Return: dictionary with leakages (name is the key, value is the complex value).

       For simplicity we assume that both real and imaginary parts are given as this is the case for all parset files
       this helper method is likely to be used for.
    """
    res = {}
    f = open(fname)
    try:
       for line in f:
          if line.startswith("leakage"):
              parts = line.split()
              if len(parts)!=3:
                  raise RuntimeError("Expect 3 parts in the line of parset file, you have %s" % (parts,))
              if parts[1]!="=":
                  raise RuntimeError("Value and key are supposed to be separated by '=', you have %s" % (parts,))
              if not parts[2].startswith('[') or not parts[2].endswith(']'):
                  raise RuntimeErrror("Value is supposed to be in square brackets, you have <%s>" % parts[2])
              values = parts[2][1:-1].split(",")
              if len(values)!=2:
                  raise RuntimeError("Two numbers are expected, you have %s" % (values,))
              res[parts[0]] = float(values[0])+(1j)*float(values[1])
    finally:
       f.close()
    return res

def runTests(solverType):
    """
    Runs tests for a given calibration solver type.
    """
    spr = SynthesisProgramRunner(template_parset = 'leakagecalibtest_template.in')
    spr.addToParset("Csimulator.corrupt = false")
    spr.runSimulator()

    spr.initParset()
    spr.runImager()
    analyseResult(spr)

    spr.addToParset("Ccalibrator.solver = " + solverType)
    spr.runCalibrator()

    # here result.dat should be close to (1.,0) within 0.03 or so
    res_gains = loadParset("result.dat")
    for k,v in list(res_gains.items()):
       if abs(v)>0.003:
          raise RuntimeError("Gain parameter %s has a value of %s which is notably different from (1,0)" % (k,v))

    # now repeat the simulation, but with corruption of visibilities
    spr.initParset()
    spr.addToParset("Csimulator.corrupt = true")
    spr.addToParset("Csimulator.corrupt.leakage = true")
    spr.runSimulator()

    # calibrate again
    spr.addToParset("Ccalibrator.solver = " + solverType)
    spr.runCalibrator()

    # gains should now be close to rndgains.in
    res_gains = loadParset("result.dat")
    orig_gains = loadParset("rndgainsandleakages.in")
    for k,v in list(res_gains.items()):
       if k not in orig_gains:
          raise RuntimeError("Gain parameter %s found in the result is missing in the model!" % k)
       orig_val = orig_gains[k]
       #print "Testing %s: has a value of %s, model value %s, difference %s" % (k,v,orig_val,abs(v-orig_val))
       if abs(v-orig_val)>0.04:
          raise RuntimeError("Gain parameter %s has a value of %s which is notably different from model value %s" % (k,v,orig_val))

runTests("SVD")
runTests("LSQR")
