#include "ThreadImager.hh"
#include "askap/askap/StatReporter.h"
#include <askap/askap_synthesis.h>

#include <Common/ParameterSet.h>
#include <askap/askap/AskapError.h>
#include <askap/askap/AskapLogging.h>

ASKAP_LOGGER(logger, ".ThreadImager")

using namespace askap::cp;
using namespace askap;

ThreadImager::ThreadImager(LOFAR::ParameterSet &parset, StatReporter &stats)
    : itsParset(parset), itsStats(stats) {}

ThreadImager::~ThreadImager() {}

void ThreadImager::run(void) {

  // LOFAR::ParameterSet tmpset = config();
  // // replace the original Cimager param set with the updated set.
  // tmpset.subtractSubset("Cimager.");
  // tmpset.adoptCollection(itsParset.makeSubset("", "Cimager."));
  ASKAPLOG_INFO_STR(logger, "Parset parameters:\n" << itsParset);

  // const int nCycles = itsParset.getInt32("ncycles", 0);
  // if (nCycles == 0) {
  //   imager.calcNE();
  // }

  // imager.writeModel();

  ASKAPLOG_INFO_STR(logger, "Run complete");
}