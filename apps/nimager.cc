#include "askap/askap/SignalCounter.h"
#include "askap/measurementequation/SynthesisParamsHelper.h"
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
#include <askap/askap/SignalManagerSingleton.h>
#include <askap/askap/StatReporter.h>
#include <askap/parallel/ImagerParallel.h>
#include <askap/parallel/ThreadImager.hh>
#include <askap/profile/AskapProfiler.h>

ASKAP_LOGGER(logger, ".nimager");

/* Not able to completely get rid of MPI, so we are just going to set the
 * process to 1 and not set nworkergroups
 */
class NImagerApp : public askap::Application {
public:
  virtual int run(int argc, char *argv[]) {

    // Can't seem to completely get rid of MPI
    askap::askapparallel::AskapParallel comms(argc,
                                              const_cast<const char **>(argv));

    try {
      askap::StatReporter stats;
      LOFAR::ParameterSet subset(config().makeSubset("Cimager."));

      boost::scoped_ptr<askap::ProfileSingleton::Initialiser> profiler;

      /* Imager and cimager use this step to get number of workers,
       this imager is going to be without MPI, may use this for number of
       threads or GPU cores in the future?
       */
      //  const int nWorkerGroups = subset.getInt32("nworkergroups", 1);
      {
        ASKAPLOG_INFO_STR(logger, "Not setting nworkergroups");
        for (LOFAR::ParameterSet::iterator it = subset.begin();
             it != subset.end(); ++it) {
          it->second = LOFAR::ParameterValue(comms.substitute(it->second));
        }

        LOFAR::ParameterSet fullset(
            askap::synthesis::ImagerParallel::autoSetParameters(comms, subset));

        askap::synthesis::ImagerParallel imager(comms, fullset);

        LOFAR::ParameterSet tmpset = config();
        // replace the original Cimager param set with the updated set.
        tmpset.subtractSubset("Cimager.");
        tmpset.adoptCollection(fullset.makeSubset("", "Cimager."));
        ASKAPLOG_INFO_STR(logger, "Parset parameters:\n" << tmpset);

        const double targetPeakResidual =
            askap::synthesis::SynthesisParamsHelper::convertQuantity(
                fullset.getString("threshold.majorcycle", "-1Jy"), "Jy");
        const bool writeAtMajorCycle =
            fullset.getBool("Images.writeAtMajorCycle", false);

        askap::SignalCounter sigcount;
        askap::SignalManagerSingleton::instance()->registerHandler(SIGUSR1,
                                                                   &sigcount);
        const int nCycles = fullset.getInt32("ncycles", 0);
        if (nCycles == 0) {
          imager.calcNE();
        } else {
          for (int cycle = 0; cycle < nCycles; ++cycle) {

            imager.calcNE();
            imager.solveNE();

            if (imager.params()->has("peak_residual")) {
              const double peak_residual =
                  imager.params()->scalarValue("peak_residual");
              ASKAPLOG_INFO_STR(logger, "Major Cycle "
                                            << cycle
                                            << " Reached peak residual of "
                                            << peak_residual << " after solve");

              if (peak_residual < targetPeakResidual) {

                ASKAPLOG_INFO_STR(
                    logger, "It is below the major cycle threshold of "
                                << targetPeakResidual << " Jy. Stopping.");

                ASKAPLOG_INFO_STR(logger, "Broadcasting final model");
                imager.broadcastModel();
                ASKAPLOG_INFO_STR(logger, "Broadcasting final model - done");
                break;

                // we have reached a peak residual after the

              } else {
                if (targetPeakResidual < 0) {
                  ASKAPLOG_INFO_STR(logger,
                                    "Major cycle flux threshold is not used.");
                } else {
                  ASKAPLOG_INFO_STR(
                      logger, "It is above the major cycle threshold of "
                                  << targetPeakResidual << " Jy. Continuing.");
                }
              }
            }
          }
          imager.calcNE();
        }

        askap::SignalManagerSingleton::instance()->removeHandler(SIGUSR1);
        imager.writeModel();
      }

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
