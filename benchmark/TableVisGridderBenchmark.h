#include <askap/gridding/SphFuncVisGridder.h>
#include <benchmark/benchmark.h>
#include <askap/gridding/ATCAIllumination.h>
#include <askap/gridding/DiskIllumination.h>
#include <askap/scimath/fitting/Params.h>
#include <askap/measurementequation/ComponentEquation.h>
#include <askap/dataaccess/DataIteratorStub.h>
#include <casacore/casa/aips.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/measures/Measures/MPosition.h>
#include <casacore/casa/Quanta/Quantum.h>
#include <casacore/casa/Quanta/MVPosition.h>
#include <casacore/casa/BasicSL/Constants.h>
#include <askap/askap/AskapError.h>
#include <askap/gridding/VisGridderFactory.h>


#include <boost/shared_ptr.hpp>

using namespace askap::scimath;

namespace askap {
namespace synthesis {
class TableVisGridderBenchmark {
public:
  boost::shared_ptr<SphFuncVisGridder> itsSphFunc;

  accessors::IDataSharedIter idi;
  boost::shared_ptr<Axes> itsAxes;
  boost::shared_ptr<casa::Array<imtype>> itsModel;
  boost::shared_ptr<casa::Array<imtype>> itsModelPSF;
  boost::shared_ptr<casa::Array<imtype>> itsModelWeights;

  void setup() {
    idi = accessors::IDataSharedIter(new accessors::DataIteratorStub(1));

    Params ip;
    ip.add("flux.i.cena", 100.0);
    ip.add("direction.ra.cena", 0.5);
    ip.add("direction.dec.cena", -0.3);
    ip.add("shape.bmaj.cena", 0.0);
    ip.add("shape.bmin.cena", 0.0);
    ip.add("shape.bpa.cena", 0.0);

    ComponentEquation ce(ip, idi);
    ce.predict();

    //        itsBox.reset(new BoxVisGridder());
    itsSphFunc.reset(new SphFuncVisGridder());
    boost::shared_ptr<IBasicIllumination> illum(
        new DiskIllumination(120.0, 10.0));
    // illumination models can easily be shared between gridders if parameters,
    // like dish size and blockage are the same
    // itsAWProject.reset(new AWProjectVisGridder(illum, 10000.0, 9, 1e-3, 1,
    // 128, 1)); itsAProjectWStack.reset(new AProjectWStackVisGridder(illum,
    // 10000.0, 9, 0., 1, 128, 1)); itsWProject.reset(new
    // WProjectVisGridder(10000.0, 9, 1e-3, 1, 128, 0, "")); itsWStack.reset(new
    // WStackVisGridder(10000.0, 9));

    double cellSize = 10 * casa::C::arcsec;

    casa::Matrix<double> xform(2, 2, 0.);
    xform.diagonal().set(1.);

    itsAxes.reset(new Axes());
    itsAxes->addDirectionAxis(casa::DirectionCoordinate(
        casa::MDirection::J2000, casa::Projection(casa::Projection::SIN), 0.,
        0., cellSize, cellSize, xform, 256., 256.));

    itsModel.reset(new casa::Array<imtype>(casa::IPosition(4, 512, 512, 1, 1)));
    itsModel->set(0.0);
    itsModelPSF.reset(
        new casa::Array<imtype>(casa::IPosition(4, 512, 512, 1, 1)));
    itsModelPSF->set(0.0);
    itsModelWeights.reset(
        new casa::Array<imtype>(casa::IPosition(4, 512, 512, 1, 1)));
    itsModelWeights->set(0.0);
  }

  void grid() {
    itsSphFunc->initialiseGrid(*itsAxes, itsModel->shape(), true);
    itsSphFunc->grid(*idi);
    itsSphFunc->finaliseGrid(*itsModelPSF);
    itsSphFunc->initialiseGrid(*itsAxes, itsModel->shape(), false);
    itsSphFunc->grid(*idi);
    itsSphFunc->finaliseGrid(*itsModel);
    itsSphFunc->finaliseWeights(*itsModelWeights);
  }

  void degrid() {
    itsSphFunc->initialiseDegrid(*itsAxes, *itsModel);
    itsSphFunc->degrid(*idi);
  }
};
} // namespace synthesis
} // namespace askap

static void BM_Spherical_Forward(benchmark::State &state) {
  askap::synthesis::TableVisGridderBenchmark gridderBench;
  gridderBench.setup();
  for (auto _ : state) {
    gridderBench.grid();
  }
}
BENCHMARK(BM_Spherical_Forward);

static void BM_Spherical_Reverse(benchmark::State &state) {
  askap::synthesis::TableVisGridderBenchmark gridderBench;
  gridderBench.setup();
  gridderBench.grid();
  for (auto _ : state) {
    gridderBench.degrid();
  }
}
BENCHMARK(BM_Spherical_Reverse);

