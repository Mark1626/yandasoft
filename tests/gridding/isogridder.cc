#include "askap/askap/StatReporter.h"
#include "askap/measurementequation/SynthesisParamsHelper.h"
#include <askap/dataaccess/DataIteratorStub.h>
#include <askap/gridding/WProjectVisGridder.h>
#include <askap/measurementequation/ComponentEquation.h>
#include <askap/scimath/fitting/Axes.h>
#include <askap/scimath/fitting/Params.h>
#include <boost/shared_ptr.hpp>
#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>
#include <fstream>
#include <iostream>

ASKAP_LOGGER(logger, ".isogridder");

using askap::imtype;

const int SIZE = 512;

void serialize(casacore::Array<imtype> arr, std::ostream &out) {
  int idx = 0;
  for (auto val : arr) {
    if (idx % SIZE == 0) {
      out << "\n";
    }
    idx++;
    out << val << " ";
  }
}

void serialize(casacore::Array<imtype> arr, std::string filename) {
  std::fstream file(filename, std::ios::out);
  serialize(arr, file);
}

void run() {

  askap::StatReporter stats;
  
  {

    boost::shared_ptr<askap::synthesis::WProjectVisGridder> itsWProject;

    askap::accessors::IDataSharedIter idi;
    boost::shared_ptr<askap::scimath::Axes> itsAxes;
    boost::shared_ptr<casacore::Array<imtype>> itsModel;
    boost::shared_ptr<casacore::Array<imtype>> itsModelPSF;
    boost::shared_ptr<casacore::Array<imtype>> itsModelWeights;

    idi = askap::accessors::IDataSharedIter(
        new askap::accessors::DataIteratorStub(1));

    askap::scimath::Params ip;
    // Source
    ip.add("flux.i.cena", 100.0);
    ip.add("direction.ra.cena", 0.5);
    ip.add("direction.dec.cena", -0.3);
    ip.add("shape.bmaj.cena", 0.0);
    ip.add("shape.bmin.cena", 0.0);
    ip.add("shape.bpa.cena", 0.0);

    // Component Equation
    askap::synthesis::ComponentEquation ce(ip, idi);
    ce.predict();

    itsWProject.reset(new askap::synthesis::WProjectVisGridder(10000.0, 9, 1e-3,
                                                              1, 128, 0, ""));

    double cellSize = 10 * casacore::C::arcsec;

    casacore::Matrix<double> xform(2, 2, 0.);
    xform.diagonal().set(1.);

    itsAxes.reset(new askap::scimath::Axes());
    itsAxes->addDirectionAxis(casacore::DirectionCoordinate(
        casacore::MDirection::J2000,
        casacore::Projection(casacore::Projection::SIN), 0., 0., cellSize,
        cellSize, xform, 256., 256.));

    itsModel.reset(
        new casacore::Array<imtype>(casacore::IPosition(4, SIZE, SIZE, 1, 1)));
    itsModel->set(0.0);
    itsModelPSF.reset(
        new casacore::Array<imtype>(casacore::IPosition(4, SIZE, SIZE, 1, 1)));
    itsModelPSF->set(0.0);
    itsModelWeights.reset(
        new casacore::Array<imtype>(casacore::IPosition(4, SIZE, SIZE, 1, 1)));
    itsModelWeights->set(0.0);

    std::cout << "Gridding" << std::endl;

    // auto shape = itsModel->shape();

    // serialize(*idi);
    std::cout << "Nrow " << idi->nRow() << std::endl;
    std::cout << "Nchannel " << idi->nChannel() << std::endl;
    std::cout << "NPol " << idi->nPol() << std::endl;
    std::cout << "Frequency " << idi->frequency() << std::endl;

    itsWProject->initialiseGrid(*itsAxes, itsModel->shape(), false);

    // serialize(*itsModel, "before.txt");
    itsWProject->grid(*idi);

    itsWProject->finaliseGrid(*itsModel);
    itsWProject->finaliseWeights(*itsModelWeights);

    // serialize(*itsModel, "after.txt");
    // std::cout << std::endl;
    itsWProject->initialiseGrid(*itsAxes, itsModel->shape(), true);
    itsWProject->grid(*idi);
    itsWProject->finaliseGrid(*itsModelPSF);
    // serialize(*itsModelPSF, "final.txt");
  }
  stats.logSummary();
  stats.logMemorySummary();

  //askap::synthesis::SynthesisParamsHelper::saveImageParameter(ip, "test-model","test-model-image");
}

void setup() {}

int main() {
  run();
  std::cout << "Done" << std::endl;
}
