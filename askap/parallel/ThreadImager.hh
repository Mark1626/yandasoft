#ifndef ASKAP_THREAD_IMAGER_H
#define ASKAP_THREAD_IMAGER_H

#include <Common/ParameterSet.h>
#include <askap/askap/StatReporter.h>

namespace askap {
namespace cp {
class ThreadImager {
public:
  ThreadImager(LOFAR::ParameterSet &parset, StatReporter &stats);
  ~ThreadImager();
  void run(void);

private:
  LOFAR::ParameterSet &itsParset;
  StatReporter &itsStats;
};
}; // namespace cp
}; // namespace askap

#endif
