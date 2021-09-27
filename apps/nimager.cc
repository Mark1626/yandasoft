#include "askap/profile/ProfileSingleton.h"
#include <askap/askap_synthesis.h>

// System includes
#include <fstream>
#include <sstream>
#include <string>
// #include <mpi.h>

// Boost includes
#include <boost/scoped_ptr.hpp>

#include <Common/ParameterSet.h>
#include <askap/askap/Application.h>
#include <askap/askap/AskapError.h>
#include <askap/askap/AskapLogging.h>
#include <askap/askap/StatReporter.h>
#include <askap/parallel/ThreadImager.hh>
#include <askap/profile/AskapProfiler.h>


ASKAP_LOGGER(logger, ".nimager");

class NImagerApp : public askap::Application {
public:
  virtual int run(int argc, char *argv[]) {

    try {
      askap::StatReporter stats;
      LOFAR::ParameterSet subset(config().makeSubset("Cimager."));

      boost::scoped_ptr<askap::ProfileSingleton::Initialiser> profiler;

      /* Imager and cimager use this step to get number of workers,
       this imager is going to be without MPI, may use this for number of
       threads or GPU cores in the future?
       */
      //  const int nWorkerGroups = subset.getInt32("nworkergroups", 1);

      // LOFAR::ParameterSet fullset(ThreadImager::autoSetParameters(comms,
      // subset));
      askap::cp::ThreadImager imager(subset, stats);
      imager.run();
      stats.logSummary();

    } catch (const askap::AskapError &e) {
      ASKAPLOG_FATAL_STR(logger,
                         "Askap error in " << argv[0] << ": " << e.what());
      std::cerr << "Askap error in " << argv[0] << ": " << e.what()
                << std::endl;
      return 1;
    } catch (const std::exception &e) {
      ASKAPLOG_FATAL_STR(logger,
                         "Unexpected error in " << argv[0] << ": " << e.what());
      std::cerr << "Unexpected error in " << argv[0] << ": " << e.what()
                << std::endl;
      return 1;
    }
    return 0;
  }
};

int main(int argc, char *argv[]) {
  NImagerApp app;
  app.addParameter("inputvis", "i", "input measurement set");
  return app.main(argc, argv);
}
