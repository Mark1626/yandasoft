/// @file
///
/// @brief Generic operations-specific calibration
/// @details This class is intended for the types of calibration which 
/// cannot follow ASKAP's predict-forward approach, i.e. which require 
/// observations of various fields done in some special way. It is intended
/// for experimentation with calibration as well as some operation-specific
/// tasks like baseline and pointing refinements.  
///
/// @copyright (c) 2007 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Max Voronkov <Maxim.Voronkov@csiro.au>

// ASKAPsoft includes
#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

ASKAP_LOGGER(logger, ".OpCalImpl");


// ASKAPsoft includes

#include <askap/opcal/OpCalImpl.h>
#include <askap/dataaccess/TableDataSource.h>
#include <askap/dataaccess/ParsetInterface.h>
#include <askap/calibaccess/CalParamNameHelper.h>
#include <askap/calibaccess/JonesIndex.h>
#include <askap/dataaccess/TableDataSource.h>
#include <askap/dataaccess/ParsetInterface.h>

#include <askap/scimath/fitting/LinearSolver.h>
#include <askap/scimath/fitting/GenericNormalEquations.h>
#include <askap/measurementequation/NoXPolGain.h>
#include <askap/measurementequation/CalibrationME.h>
#include <askap/measurementequation/PreAvgCalMEBase.h>
#include <askap/measurementequation/ImageFFTEquation.h>
#include <askap/measurementequation/ComponentEquation.h>
#include <askap/measurementequation/SynthesisParamsHelper.h>
#include <askap/measurementequation/MEParsetInterface.h>
#include <askap/measurementequation/ImagingEquationAdapter.h>

#include <casacore/casa/BasicSL.h>

// std includes

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <complex>


namespace askap {

namespace synthesis {

/// @brief Constructor from ParameterSet
/// @details The parset is used to construct the internal state. We could
/// also support construction from a python dictionary (for example).
/// @param comms communication object 
/// @param parset ParameterSet for inputs
/// @note We don't use parallel aspect at this stage, the code expects a single rank if compiled with MPI.
OpCalImpl::OpCalImpl(askap::askapparallel::AskapParallel& comms, const LOFAR::ParameterSet& parset) : MEParallelApp(comms,parset),
   itsScanStats(parset.getFloat("maxtime",-1.), parset.getInt("maxcycles",-1))
{
   ASKAPCHECK(!comms.isParallel(), "This application is not intended to be used in parallel mode (at this stage)");
   if (itsScanStats.timeLimit() > 0.) {
       ASKAPLOG_INFO_STR(logger, "Chunks will be limited to "<<itsScanStats.timeLimit()<<" seconds"); 
   }  
   if (itsScanStats.cycleLimit() > 0) {
       ASKAPLOG_INFO_STR(logger, "Chunks will be limited to "<<itsScanStats.cycleLimit()<<" correlator cycles in length"); 
   }     
}


/// @brief main entry point
void OpCalImpl::run()
{
  inspectData();
  ASKAPLOG_INFO_STR(logger, "Found "<<itsScanStats.size()<<" chunks in the supplied data");
  runCalibration();                   
  // process results
  if (itsHighLevelSolver) {
      ASKAPLOG_INFO_STR(logger, "Calibration completed for all scans, calling user-defined solver");
      ASKAPDEBUGASSERT(itsScanStats.size() == itsCalData.nrow());      
      itsHighLevelSolver->process(itsScanStats,itsCalData);
  } else {
      ASKAPLOG_INFO_STR(logger, "High-level solver is not defined, just printing the summary below");
      ASKAPLOG_INFO_STR(logger, "#no time_since_start(min) beamID fieldID scanID amp1 phase1 amp2 phase2 ... ampN phaseN (phase in degrees)");
      
      float firstTime = 0;
      for (casacore::uInt row=0; row<itsCalData.nrow(); ++row) {
           std::string result;
           ASKAPDEBUGASSERT(itsScanStats.size() > row);
           const double timeCentroid = itsScanStats[row].startTime() + 0.5*(itsScanStats[row].endTime() - itsScanStats[row].startTime());
           if (row == 0) {
               firstTime = timeCentroid;
           }
           result += utility::toString<casacore::uInt>(row)+" "+
                  utility::toString<double>((timeCentroid - firstTime)/60.)+" "+
                  utility::toString<casacore::uInt>(itsScanStats[row].beam()) + " "+
                  utility::toString<casacore::uInt>(itsScanStats[row].fieldID()) + " "+
                  utility::toString<casacore::uInt>(itsScanStats[row].scanID());
       
           for (casacore::uInt ant=0; ant<itsCalData.ncolumn(); ++ant) {
                if (itsCalData(row,ant).gainDefined()) {
                    const casacore::Complex thisAntGain = itsCalData(row,ant).gain();
                    result += " " + utility::toString<double>(std::abs(thisAntGain)) + " " + utility::toString<double>(std::arg(thisAntGain)/casacore::C::pi*180.);
                } else {
                    result += " flagged flagged";
                }
           }
           ASKAPLOG_INFO_STR(logger, "result: "<<result);
      }
  }
}
   
   
/// @brief gather scan statistics
/// @details This method iterates over data for all supplied MSs and fills itsScanStats at the end.
/// Optional parameters describing how to break long observations are taken from the parset.
void OpCalImpl::inspectData()
{
   const std::vector<std::string> msList = parset().getStringVector("dataset");
   for (std::vector<std::string>::const_iterator ci = msList.begin(); ci!=msList.end(); ++ci) {
        
        const size_t sizeBefore = itsScanStats.size();
        accessors::TableDataSource ds(*ci, accessors::TableDataSource::MEMORY_BUFFERS,dataColumn());
        accessors::IDataSelectorPtr sel=ds.createSelector();
        sel << parset();
        accessors::IDataConverterPtr conv=ds.createConverter();
        conv->setFrequencyFrame(getFreqRefFrame(), "Hz");
        conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));
        conv->setEpochFrame(); // time in seconds since 0 MJD
        accessors::IDataSharedIter it=ds.createIterator(sel, conv);
        ASKAPLOG_INFO_STR(logger, "Inspecting "<<*ci);
        itsScanStats.inspect(*ci, it);
        ASKAPLOG_INFO_STR(logger, "   - found "<<itsScanStats.size() - sizeBefore<<" chunks");
   }                 
}

/// @brief perform calibration for every scan
/// @details This method runs calibration procedure for each scan in itsScanStats, initialises and
/// fills itsCalData
void OpCalImpl::runCalibration()
{   
   const casacore::uInt nAnt = parset().getUint("nant",12);
   ASKAPCHECK(nAnt > 0, "Expect a positive number of antennas");
   itsCalData.resize(itsScanStats.size(), nAnt);
   // ensure each element is undefined although this is not necessary, strictly speaking
   for (casacore::uInt scan=0; scan < itsCalData.nrow(); ++scan) {
        for (casacore::uInt ant=0; ant < itsCalData.ncolumn(); ++ant) {
             itsCalData(scan,ant).invalidate();
        }
   }
   
   // now obtain calibration information for every "scan"
   // first, build a list of all names to have more structured access to the data (in the hope of having a speed up)
   // the iteration over scans is expected to be a relatively cheap operation, so we iterate multiple times if necessary
   
   // the key is the name of the dataset, the value is the set of beams used in that dataset 
   std::map<std::string, std::set<casacore::uInt> > namesAndBeams;
   for (ScanStats::const_iterator ci = itsScanStats.begin(); ci != itsScanStats.end(); ++ci) {
        ASKAPDEBUGASSERT(ci->isValid());
        // map::insert returns a pair of iterator and bool showing whether a new element was inserted or an just iterator
        // on existing element was returned. We take only the iterator because we don't care whether the element is new or not
        std::map<std::string, std::set<casacore::uInt> >::iterator thisElemIt = 
                namesAndBeams.insert(std::pair<std::string, std::set<casacore::uInt> >(ci->name(),std::set<casacore::uInt>())).first; 
        ASKAPDEBUGASSERT(thisElemIt != namesAndBeams.end());
        thisElemIt->second.insert(ci->beam());
   }
   for (std::map<std::string, std::set<casacore::uInt> >::const_iterator nameIt = namesAndBeams.begin(); 
                     nameIt != namesAndBeams.end(); ++nameIt) {
        ASKAPLOG_INFO_STR(logger, "Performing "<<nameIt->second.size()<<" beam calibration for "<<nameIt->first);
        // note, scans are expected to be in the order of cycles because this is the way we find them
        processOne(nameIt->first, nameIt->second);
   }
} 

/// @brief process one dataset
/// @details The method obtains calibration solutions for a given set of beams in one dataset. Note, 
/// it would fail if the dataset contains beams not present in the given set (it has to define parameters
/// to solve up front). The calibration solution is stored in itsCalData. 
/// @param[in] ms name of the dataset
/// @param[in] beams set of beams to process
void OpCalImpl::processOne(const std::string &ms, const std::set<casacore::uInt> &beams)
{  
  accessors::TableDataSource ds(ms, accessors::TableDataSource::DEFAULT,dataColumn());
  accessors::IDataSelectorPtr sel=ds.createSelector();
  sel << parset();
  accessors::IDataConverterPtr conv=ds.createConverter();
  conv->setFrequencyFrame(getFreqRefFrame(), "Hz");
  conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));
  // ensure that time is counted in seconds since 0 MJD
  conv->setEpochFrame();
  accessors::IDataSharedIter it=ds.createIterator(sel, conv);
  if (!it.hasMore()) {
      ASKAPLOG_INFO_STR(logger, "No data seem to be available for "<<ms<<", ignoring");
      return; 
  }
  
  boost::shared_ptr<IMeasurementEquation const> perfectME = makePerfectME();
  
  std::map<casacore::uInt, size_t> thisChunkBeams;
  size_t chunkCounter = 0, cycle = 0;
  for (;it.hasMore();it.next(),++cycle) {
       std::map<casacore::uInt, size_t> thisCycleBeams = matchScansForAllBeams(ms,cycle);
       bool sameCycle = (thisCycleBeams.size() == thisChunkBeams.size());
       bool someScanMatchesBoth = false;
       // iterate over all beams present in the dataset, adjust sameCycle and sameScanMatchesBoth 
       for (std::set<casacore::uInt>::const_iterator ci = beams.begin(); ci!=beams.end(); ++ci) {
            const std::map<casacore::uInt, size_t>::const_iterator thisCycleElem = thisCycleBeams.find(*ci);
            const std::map<casacore::uInt, size_t>::const_iterator thisChunkElem = thisChunkBeams.find(*ci);
            // each tested beam should either be present in both or in none 
            if ((thisCycleElem == thisCycleBeams.end()) != (thisChunkElem == thisChunkBeams.end())) {
                sameCycle = false;
            } else if (thisCycleElem != thisCycleBeams.end()) {
                ASKAPDEBUGASSERT(thisChunkElem != thisChunkBeams.end());
                if (thisCycleElem->second != thisChunkElem->second) {
                    sameCycle = false;                        
                } else {
                    // we don't break the loop after the first sameCycle=false to ensure later
                    // that no scan matches both old chunk and new cycle together with sameCycle=false
                    someScanMatchesBoth = true;
                }
            }
       }
       if (!sameCycle) {
          ASKAPCHECK(!someScanMatchesBoth, "Same scan is found to match two chunks of calibration data, cycle="<<cycle<<" ms="<<ms);
          // this is a new chunk
          if (++chunkCounter > 1) {
              // process already accumulated data
              solveOne(thisChunkBeams);
          }
          thisChunkBeams = thisCycleBeams;
          ASKAPLOG_INFO_STR(logger, "Solution interval "<<chunkCounter<<" - setting up the model and the measurement equation");
    
          ASKAPDEBUGASSERT(itsModel);
          itsModel->reset();
          casacore::uInt maxBeamID = 0;
          for (std::map<casacore::uInt,size_t>::const_iterator ci = thisChunkBeams.begin(); ci!=thisChunkBeams.end(); ++ci) {
               const std::set<casacore::uInt> flaggedAntsXX = itsScanStats[ci->second].flaggedAntennas(casacore::Stokes::XX, itsCalData.ncolumn());
               const std::set<casacore::uInt> flaggedAntsYY = itsScanStats[ci->second].flaggedAntennas(casacore::Stokes::YY, itsCalData.ncolumn());
               for (casacore::uInt ant=0; ant < itsCalData.ncolumn(); ++ant) {
                    const std::string xxParam = accessors::CalParamNameHelper::paramName(ant, ci->first, casacore::Stokes::XX);
                    itsModel->add(xxParam, casacore::Complex(1., 0.));
                    const std::string yyParam = accessors::CalParamNameHelper::paramName(ant, ci->first, casacore::Stokes::YY);
                    itsModel->add(yyParam, casacore::Complex(1., 0.));            
                    if (flaggedAntsXX.find(ant) != flaggedAntsXX.end()) {
                        itsModel->fix(xxParam);
                        ASKAPLOG_INFO_STR(logger, "No data for XX polarisation for antenna "<<ant);
                    }
                    if (flaggedAntsYY.find(ant) != flaggedAntsYY.end()) {
                        itsModel->fix(yyParam);
                        ASKAPLOG_INFO_STR(logger, "No data for YY polarisation for antenna "<<ant);
                    }
               }
               if (maxBeamID < ci->first) {
                   maxBeamID = ci->first;
               }
          }
          itsME.reset(new CalibrationME<NoXPolGain, PreAvgCalMEBase>());
          ASKAPDEBUGASSERT(itsME);          
          // explicit initialisation of pre-averaging buffer to avoid issues described in ASKAPSDP-1747
          itsME->initialise(itsCalData.ncolumn(), maxBeamID + 1, 1);
       }
       // accumulate data
       ASKAPDEBUGASSERT(it.hasMore());
       itsME->accumulate(*it,perfectME);
  }
  // finish processing of the last cycle
  ASKAPDEBUGASSERT(chunkCounter >=1);
  solveOne(thisChunkBeams);
  ASKAPLOG_INFO_STR(logger, "Processed "<<cycle<<" cycles split into "<<chunkCounter<<" solution intervals, number of beams is "<<beams.size());
}

/// @brief make uncorrupted measurement equation
/// @details This method uses parset parameters and makes uncorrupted (i.e. ideal) measurement equation
/// @return shared pointer to the measurement equation
boost::shared_ptr<IMeasurementEquation> OpCalImpl::makePerfectME() const
{
   ASKAPLOG_DEBUG_STR(logger, "Constructing measurement equation corresponding to the uncorrupted model");
   boost::shared_ptr<scimath::Params> perfectModel(new scimath::Params);
   readModels(perfectModel);
   ASKAPCHECK(perfectModel, "Uncorrupted model don't seem to be defined");
   if (SynthesisParamsHelper::hasImage(perfectModel)) {
       ASKAPCHECK(!SynthesisParamsHelper::hasComponent(perfectModel),
                  "Image + component case has not yet been implemented");
       // have to create an image-specific equation        
       boost::shared_ptr<ImagingEquationAdapter> ieAdapter(new ImagingEquationAdapter);
       ASKAPCHECK(gridder(), "Gridder not defined");
       ieAdapter->assign<ImageFFTEquation>(*perfectModel, gridder());
       return ieAdapter;
   }
       
   // model is a number of components, don't need an adapter here
   // it doesn't matter which iterator is passed below. It is not used
   boost::shared_ptr<ComponentEquation> compEq(new ComponentEquation(*perfectModel,accessors::IDataSharedIter()));
   return compEq;
}
 

/// @brief solve ME for one interval
/// @details This method is called from processOne when data corresponding to the given solution 
/// interval has been accumulated. It solves the measurement equation and stores the result in
/// itsCalData
/// @param[in] beammap map of beam IDs to scan indices (into itsScanStats)
void OpCalImpl::solveOne(const std::map<casacore::uInt, size_t>& beammap)
{
  const casacore::uInt refAnt = parset().getUint("refant",0);
  ASKAPCHECK(refAnt < itsCalData.ncolumn(), "Reference antenna index exceeds the number of antennas");
  // at this stage, deal with just XX polarisation
  
  ASKAPASSERT(itsME);
  ASKAPASSERT(itsModel);
  const casacore::uInt nIter = parset().getUint("niter",20);
  scimath::LinearSolver solver;
  for (casacore::uInt iter = 0; iter < nIter; ++iter) {
       ASKAPLOG_DEBUG_STR(logger, "Calibration iteration "<<(iter+1));
       itsME->setParameters(*itsModel);
       scimath::GenericNormalEquations gne;
       itsME->calcEquations(gne);
       solver.init();
       solver.addNormalEquations(gne);
       solver.setAlgorithm("SVD");
       scimath::Quality q;
       solver.solveNormalEquations(*itsModel,q);
       if (iter == 0) {
           ASKAPLOG_INFO_STR(logger, "Solved normal equations, quality: "<<q);
       } else {
           ASKAPLOG_DEBUG_STR(logger, "Solved normal equations, quality: "<<q);
       }
       // phase rotation
       for (std::map<casacore::uInt, size_t>::const_iterator beamIt = beammap.begin(); beamIt != beammap.end(); ++beamIt) {
            const std::string refGain = accessors::CalParamNameHelper::paramName(refAnt, beamIt->first, casacore::Stokes::XX);
            const casacore::Complex refPhaseTerm = casacore::polar(1.f, -std::arg(itsModel->complexValue(refGain)));
            
            for (casacore::uInt ant=0; ant<itsCalData.ncolumn(); ++ant) {
                 const std::string parname = accessors::CalParamNameHelper::paramName(ant, beamIt->first, casacore::Stokes::XX);
                 if (itsModel->isFree(parname)) {
                     itsModel->update(parname, itsModel->complexValue(parname) * refPhaseTerm);
                 } else {
                     if (iter+1 == nIter) {
                         ASKAPLOG_INFO_STR(logger, "Parameter "<<parname<<" is fixed - not rotating");
                     }
                 }
            }
       }
  }
   
  // store the solution
  const std::vector<std::string> parlist = itsModel->freeNames();
  for (std::vector<std::string>::const_iterator it = parlist.begin(); it != parlist.end(); ++it) {
       const casacore::Complex val = itsModel->complexValue(*it);           
       const std::pair<accessors::JonesIndex, casacore::Stokes::StokesTypes> paramType = 
             accessors::CalParamNameHelper::parseParam(*it);
       if (paramType.second == casacore::Stokes::XX) {      
           std::map<casacore::uInt, size_t>::const_iterator thisBeamIt = beammap.find(paramType.first.beam());
           ASKAPASSERT(thisBeamIt != beammap.end());
           ASKAPASSERT(paramType.first.antenna() < static_cast<int>(itsCalData.ncolumn()));
           ASKAPASSERT(thisBeamIt->second < itsCalData.nrow());
           itsCalData(thisBeamIt->second, paramType.first.antenna()).setGain(val);
       }
  }   
}


/// @brief helper method to search for the scans corresponding to a cycle
/// @details This method searches for a matching scan to the given cycle, beam and name and
/// returns its index. itsScanStats.size() is returned if no match is found.
/// @param[in] name name key of the dataset to match
/// @param[in] beam beam ID to match
/// @param[in] cycle cycle to match
/// @return index of the matching scan or itsScanStats.size() if no match is found
size_t OpCalImpl::matchScan(const std::string &name, casacore::uInt beam, casacore::uInt cycle) const
{
  size_t index = 0;
  for (ScanStats::const_iterator ci = itsScanStats.begin(); ci != itsScanStats.end(); ++ci,++index) {
       ASKAPDEBUGASSERT(ci->isValid());
       if ((ci->name() == name) && (ci->beam() == beam) && (cycle >= ci->startCycle()) && (cycle <= ci->endCycle())) {
           break;
       }
  }
  ASKAPDEBUGASSERT(index <= itsScanStats.size()); 
  return index;
} 

/// @brief helper method to search for scans for a number of beams at once
/// @details This method matches cycle in the given dataset to scans. We treat individual beams as
/// separate observations (or scans) here, therefore the same cycle can match a number of beams.
/// The method returns a map between beams and scan indices. The result is the same as calling
/// matchScan for every available beam but is obtained in a more optimal fashion. 
/// @param[in] name name key of the dataset 
/// @param[in] cycle cycle to match
/// @return a map of beam IDs to scan indices
std::map<casacore::uInt, size_t> OpCalImpl::matchScansForAllBeams(const std::string &name, casacore::uInt cycle) const
{
  std::map<casacore::uInt, size_t> result;
  size_t index = 0;
  for (ScanStats::const_iterator ci = itsScanStats.begin(); ci != itsScanStats.end(); ++ci,++index) {
       ASKAPDEBUGASSERT(ci->isValid());
       if ((ci->name() == name) && (cycle >= ci->startCycle()) && (cycle <= ci->endCycle())) {
           const bool wasNew = result.insert(std::pair<casacore::uInt, size_t>(ci->beam(), index)).second;
           ASKAPCHECK(wasNew, "Found a second scan matching cycle="<<cycle<<" and beam="<<ci->beam()<<" for "<<name<<
                              ", this shouldn't have happened");
       }
  }
  return result;
}  

/// @brief set high-level solver
/// @details A high-level solver is intended to solve concrete calibration problem. It is 
/// responsible for processing of the actual calibration data obtained by this class.
/// Unless a concrete solver is set, only a summary is printed in the log
/// @param[in] solver shared pointer to the solver class
void OpCalImpl::setHighLevelSolver(const boost::shared_ptr<IGenericCalSolver> &solver)
{
   ASKAPCHECK(solver, "An attempt to call setHighLevelSolver with an uninitialised shared pointer");
   itsHighLevelSolver = solver;
} 



} // namespace synthesis

} // namespace askap

