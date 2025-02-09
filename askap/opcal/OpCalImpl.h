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

#ifndef SYNTHESIS_OP_CAL_IMPL_H
#define SYNTHESIS_OP_CAL_IMPL_H

// casa includes
#include <casacore/casa/Arrays/Matrix.h>

// ASKAPsoft includes
#include <askap/askapparallel/AskapParallel.h>
#include <Common/ParameterSet.h>
#include <askap/opcal/ScanStats.h>
#include <askap/opcal/GenericCalInfo.h>
#include <askap/opcal/IGenericCalSolver.h>

#include <askap/scimath/fitting/Params.h>
#include <askap/measurementequation/PreAvgCalMEBase.h>
#include <askap/measurementequation/IMeasurementEquation.h>
#include <askap/parallel/MEParallelApp.h>

// std includes
#include <set>
#include <map>

// boost
#include <boost/shared_ptr.hpp>

namespace askap {

namespace synthesis {

/// @brief Generic operations-specific calibration
/// @details This class is intended for the types of calibration which 
/// cannot follow ASKAP's predict-forward approach, i.e. which require 
/// observations of various fields done in some special way. It is intended
/// for experimentation with calibration as well as some operation-specific
/// tasks like baseline and pointing refinements.  
/// @ingroup opcal
class OpCalImpl : public MEParallelApp {
public:
   /// @brief Constructor from ParameterSet
   /// @details The parset is used to construct the internal state. We could
   /// also support construction from a python dictionary (for example).
   /// @param comms communication object 
   /// @param parset ParameterSet for inputs
   /// @note We don't use parallel aspect at this stage, the code expects a single rank if compiled with MPI.
   OpCalImpl(askap::askapparallel::AskapParallel& comms, const LOFAR::ParameterSet& parset);


   /// @brief main entry point
   void run();
   
   /// @brief set high-level solver
   /// @details A high-level solver is intended to solve concrete calibration problem. It is 
   /// responsible for processing of the actual calibration data obtained by this class.
   /// Unless a concrete solver is set, only a summary is printed in the log
   /// @param[in] solver shared pointer to the solver class
   void setHighLevelSolver(const boost::shared_ptr<IGenericCalSolver> &solver); 
   
protected:
   
   /// @brief gather scan statistics
   /// @details This method iterates over data for all supplied MSs and fills itsScanStats at the end.
   /// Optional parameters describing how to break long observations are taken from the parset.
   void inspectData();
   
   /// @brief perform calibration for every scan
   /// @details This method runs calibration procedure for each scan in itsScanStats, initialises and
   /// fills itsCalData
   void runCalibration();
   
   /// @brief process one dataset
   /// @details The method obtains calibration solutions for a given set of beams in one dataset. Note, 
   /// it would fail if the dataset contains beams not present in the given set (it has to define parameters
   /// to solve up front). The calibration solution is stored in itsCalData. 
   /// @param[in] ms name of the dataset
   /// @param[in] beams set of beams to process
   void processOne(const std::string &ms, const std::set<casacore::uInt> &beams);
   
   /// @brief solve ME for one interval
   /// @details This method is called from processOne when data corresponding to the given solution 
   /// interval has been accumulated. It solves the measurement equation and stores the result in
   /// itsCalData
   /// @param[in] beammap map of beam IDs to scan indices (into itsScanStats)
   void solveOne(const std::map<casacore::uInt, size_t>& beammap);
   
        
   /// @brief helper method to search for the scans corresponding to a cycle
   /// @details This method searches for a matching scan to the given cycle, beam and name and
   /// returns its index. itsScanStats.size() is returned if no match is found.
   /// @param[in] name name key of the dataset to match
   /// @param[in] beam beam ID to match
   /// @param[in] cycle cycle to match
   /// @return index of the matching scan or itsScanStats.size() if no match is found
   size_t matchScan(const std::string &name, casacore::uInt beam, casacore::uInt cycle) const; 
   
   /// @brief helper method to search for scans for a number of beams at once
   /// @details This method matches cycle in the given dataset to scans. We treat individual beams as
   /// separate observations (or scans) here, therefore the same cycle can match a number of beams.
   /// The method returns a map between beams and scan indices. The result is the same as calling
   /// matchScan for every available beam but is obtained in a more optimal fashion.
   /// @param[in] name name key of the dataset 
   /// @param[in] cycle cycle to match
   /// @return a map of beam IDs to scan indices
   std::map<casacore::uInt, size_t> matchScansForAllBeams(const std::string &name, casacore::uInt cycle) const;  
    
   /// @brief make uncorrupted measurement equation
   /// @details This method uses parset parameters and makes uncorrupted (i.e. ideal) measurement equation
   /// @return shared pointer to the measurement equation
   boost::shared_ptr<IMeasurementEquation> makePerfectME() const;
     
   // these methods are the legacy of the design
   virtual void calcNE() {};
   
   virtual void solveNE() {};  
     
   virtual void writeModel(const std::string&) {}  
private:
   
   /// @brief details for every scan
   ScanStats  itsScanStats;
   
   /// @brief calibration data
   /// @details rows are scans (matching itsScanStats.size()), columns are antennas
   casacore::Matrix<GenericCalInfo> itsCalData;
      
   /// @brief measurement equation
   boost::shared_ptr<PreAvgCalMEBase> itsME;
   
   /// @brief high-level solver
   /// @details The process method of this class is called when the results are ready. By default, 
   /// the summary is printed to the log.
   boost::shared_ptr<IGenericCalSolver> itsHighLevelSolver;   
};

} // namespace synthesis

} // namespace askap

#endif // #ifndef SYNTHESIS_OP_CAL_IMPL_H


