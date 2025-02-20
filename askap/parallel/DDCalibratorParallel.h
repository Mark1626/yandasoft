/// @file DDCalibratorParallel.cc
///
/// DDCalibratorParallel: part of the specialised tool to do direction-dependent
/// calibration. It is closely based on CalibratorParallel, with support for
/// parallel applications using the measurement equation classes.
///
/// @copyright (c) 2019 CSIRO
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>
/// @author Daniel Mitchell <daniel.mitchell@csiro.au>
///
#ifndef DD_CALIBRATOR_PARALLEL_H
#define DD_CALIBRATOR_PARALLEL_H

// own includes
#include <askap/askapparallel/AskapParallel.h>
#include <askap/parallel/MEParallelApp.h>
#include <Common/ParameterSet.h>
#include <askap/gridding/IVisGridder.h>
#include <askap/measurementequation/IMeasurementEquation.h>
#include <askap/dataaccess/SharedIter.h>
#include <askap/scimath/fitting/Solver.h>
#include <askap/calibaccess/ICalSolutionSource.h>
#include <askap/dataaccess/TimeChunkIteratorAdapter.h>


// std includes
#include <string>
#include <vector>

// boost includes
#include <boost/shared_ptr.hpp>

namespace askap
{
  namespace synthesis
  {
    /// @brief Support for parallel algorithms implementing calibration
    ///
    /// @details Provides calibration using the measurement equation framework.
    ///
    /// The control parameters are specified in a parset file. For example:
    /// @code
    ///    Cddcalibrator.datacolumnset                = DATACOL     # default is DATA
    ///    Cddcalibrator.dataset                      = [data/spw_1/sim.ms]
    ///    #Feed                                      = 5
    ///    
    ///    Cddcalibrator.sources.names                = [10uJy]
    ///    Cddcalibrator.sources.10uJy.direction      = [12h30m00.000, -45.00.00.000, J2000]
    ///    Cddcalibrator.sources.10uJy.model          = 10uJy.model
    ///    
    ///    Cddcalibrator.gridder                      = WProject
    ///    Cddcalibrator.gridder.WProject.wmax        = 8000
    ///    Cddcalibrator.gridder.WProject.nwplanes    = 64
    ///    Cddcalibrator.gridder.WProject.oversample  = 1
    ///    Cddcalibrator.gridder.WProject.cutoff      = 0.001
    ///
    /// @endcode
    /// @ingroup parallel
    class DDCalibratorParallel : public MEParallelApp
    {
  public:

      /// @brief Constructor from ParameterSet
      /// @details The parset is used to construct the internal state. We could
      /// also support construction from a python dictionary (for example).
      /// The command line inputs are needed solely for MPI - currently no
      /// application specific information is passed on the command line.
      /// @param[in] comms communication object
      /// @param[in] parset ParameterSet for inputs
      DDCalibratorParallel(askap::askapparallel::AskapParallel& comms,
          const LOFAR::ParameterSet& parset);

      /// @brief Calculate the normal equations (runs in the prediffers)
      /// @details ImageFFTEquation and the specified gridder (set in the parset
      /// file) are used in conjunction with CalibrationME to calculate
      /// the generic normal equations. The image parameters used in the uncorrupted
      /// measurement equation are defined in the parset file.
      virtual void calcNE();

      /// @brief Solve the normal equations (runs in the solver)
      /// @details Parameters of the calibration problem are solved for here
      virtual void solveNE();

      /// @brief Write the results (runs in the solver)
      /// @details The solution (calibration parameters) is written into
      /// an external file in the parset file format.
      /// @param[in] postfix a string to be added to the file name
      virtual void writeModel(const std::string &postfix = std::string());

      /// @brief helper method to extract next chunk flag
      /// @details This method is a reverse operation to that of setNextChunkFlag. It
      /// extracts the flag from the metadata attached to the normal equations and
      /// returns it.
      /// @note false is returned if no appropriate metadata element is found or the normal
      /// equations object does not support metadata.
      /// @return true, if the flag is set
      bool getNextChunkFlag() const;

      /// @brief helper method to remove the next chunk flag
      void removeNextChunkFlag();

      /// @brief initalise measurement equation and model
      /// @details This method is indended to be called if this object is reused to
      /// get more than one solution. It initialises the model and normal equations.
      /// It is called from constructor, so if only one solution is required the constructor
      /// is sufficient.
      /// @param[in] parset ParameterSet for inputs
      void init(const LOFAR::ParameterSet& parset);

      /// @brief Performs phase rotation.
      /// @note To be called after all cycles completed in the major loop.
      void doPhaseReferencing();

  protected:
      /// @brief initialise the class to iterate over next portion of data
      /// @details This method signals to the iterator adapter to switch to the
      /// next chunk of data. It also checks whether more data are available.
      /// @note This method is intended to be called from workers (which have
      /// the iterator initialised). An exception is thrown if the iterator
      /// adapter is not initialised
      /// @return true, if more data chunks are available
      bool nextChunk() const;

      /// @brief helper method to set next chunk flag
      /// @details In the current design, iteration over the data is done by workers.
      /// However, maser needs to make the decision whether more iterations are required,
      /// i.e. whether a new chunk of the data is available. We carry this information from
      /// worker to maser with the normal equations using metadata. This method encodes the
      /// given value of the flag in the normal equations class.
      /// @note Nothing is done if the normal equations object does not support metadata.
      /// An exception is thrown if this method is called from the maser. We could've join
      /// this method and nextChunk, but it would require making this method non-const
      /// @param[in] flag flag value to set
      void setNextChunkFlag(const bool flag);

      /// @brief create measurement equation
      /// @details This method initialises itsEquation with shared pointer to a proper type.
      /// It uses internal flags to create a correct type (i.e. polarisation calibration or
      /// just antenna-based gains). Parameters are passed directly to the constructor of
      /// CalibrationME template.
      /// @param[in] dsi data shared iterator
      /// @param[in] perfectME uncorrupted measurement equation
      void createCalibrationME(const accessors::IDataSharedIter &dsi,
                const boost::shared_ptr<IMeasurementEquation const> &perfectME);

      /// @brief helper method to rotate all phases
      /// @details This method rotates the phases of all gains in itsModel
      /// to have the phase of itsRefGain exactly 0. This operation does
      /// not seem to be necessary for SVD solvers, however it simplifies
      /// "human eye" analysis of the results (otherwise the phase degeneracy
      /// would make the solution different from the simulated gains).
      /// @note The method throws exception if itsRefGain is not among
      /// the parameters of itsModel
      void rotatePhases();

      /// @brief helper method to extract solution time from NE.
      /// @details To be able to time tag the calibration solutions we add
      /// start and stop times extracted from the dataset as metadata to normal
      /// equations. It allows us to send these times to the master, which
      /// ultimately writes the calibration solution. Otherwise, these times
      /// could only be obtained in workers who deal with the actual data.
      /// @return solution time (seconds since 0 MJD)
      /// @note if no start/stop time metadata are present in the normal equations
      /// this method returns 0.
      double solutionTime() const;

      /// @brief helper method to update channel offset
      /// @details To be able to process a subset of channels we specify the offset
      /// in the model. However, this offset needs to be reset per worker in the
      /// parallel case for the correct operation. This method encapsulates the required
      /// code of setting the channel offset to the value of itsStartChan
      void setChannelOffsetInModel() const;

  private:
      /// @brief read the model from parset file and populate itsPerfectModel
      /// @details This method is common between several classes and probably
      /// should be pushed up in the class hierarchy
      inline void readModels() const { SynParallel::readModels(itsPerfectModel); }

      /// Calculate normal equations for one data set
      /// @param[in] ms Name of data set
      /// @param[in] discard Discard old equation?
      void calcOne(const std::string& dataset, bool discard=true);

      // @brief Checks whether the Linear solver is needed to be used.
      bool useLinearSolver() const;

      /// uncorrupted model
      askap::scimath::Params::ShPtr itsPerfectModel;

      /// @brief name of the parameter taken as a reference
      /// @details empty string means no referencing is required
      std::string itsRefGainXX;
      std::string itsRefGainYY;

      /// @brief flag switching the gain calibration on
      bool itsSolveGains;

      /// @brief flag swtiching the leakage calibration on
      bool itsSolveLeakage;

      /// @brief flag switching the bandpass calibration on
      bool itsSolveBandpass;

      /// @brief chunk size per worker (used in parallel case)
      /// @details zero means that the whole dataset is used
      casacore::uInt itsChannelsPerWorker;

      /// @brief start channel per worker (used in parallel case)
      /// @details This data field is only used if itsChannelPerWorker
      /// is positive. It is expected to be used with substitution
      casacore::uInt itsStartChan;

      /// @brief flag to set gain amplitudes to unity before output to file
      bool itsNormaliseGains;

      /// @brief solution source to store the result
      /// @details This object is initialised by the master. It stores the solution
      /// in parset file, casa table or a database.
      boost::shared_ptr<accessors::ICalSolutionSource> itsSolutionSource;

      /// @brief iterator to be used in workers
      /// @details The adapter wraps around actual iterator over data and can break
      /// iteration at given intervals. This is used to provide time-dependent solution.
      /// This field is initialised upon the first call to calcOne.
      boost::shared_ptr<accessors::TimeChunkIteratorAdapter> itsIteratorAdapter;

      /// @brief optional solution interval in seconds
      /// @details If a positive number is given, the calibration solution will be
      /// obtained for each time chunk with the duration given by this field.
      /// This field is initialised and used only in workers
      double itsSolutionInterval;

      /// @brief shared pointer to measurement equation correspondent to the perfect model
      /// @details It is handy to store the perfect measurement equation, so it is not
      /// recreated every time for each solution interval.
      boost::shared_ptr<IMeasurementEquation const> itsPerfectME;


      /// @brief helper method to update maximal expected numbers for pre-averaging
      /// @details This method updates global maxima based on the current local values. It is handy to
      /// avoid code dublication as well as for the case if we ever decide to trim the pre-aveaging buffer
      /// to exclude fixed parameters.
      /// @param[in] nAnt currently expected number of antennas in the buffer
      /// @param[in] nCal currently expected number of separate calibrators in the buffer -- same as nBeam in cCal
      /// @param[in] nChan currently expected number of frequency channels in the buffer
      void updatePreAvgBufferEstimates(const casacore::uInt nAnt, const casacore::uInt nCal, const casacore::uInt nChan = 1);

      /// @brief maximum number of antennas for pre-averging
      /// @details It is handy to cache the number of antennas expected to be dealt with in pre-averaging.
      /// Deducing it from the data on the fly works too, but does not account for the case when the number of rows
      /// change from integration to integration.
      casacore::uInt itsMaxNAntForPreAvg;

      /// @brief maximum number of separate calibrators for pre-averging
      /// @details It is handy to cache the number of separate calibrators expected to be dealt with in pre-averaging.
      /// Deducing it from the data on the fly works too, but does not account for the case when the number of rows
      /// change from integration to integration.
      casacore::uInt itsMaxNCalForPreAvg;

      /// @brief maximum number of channels for pre-averging
      /// @details It is handy to cache the number of channels expected to be dealt with in pre-averaging.
      /// Deducing it from the data on the fly works too, but does not account for the case when the number of rows
      /// change from integration to integration. It is also used to give a warning if calibration effect is frequency
      /// independent and more than one channel is selected for preaveraging.
      casacore::uInt itsMaxNChanForPreAvg;

      /// @brief flag to solve normal equation in parallel (on the matrix level).
      bool itsMatrixIsParallel;

      // Iteration number in the major loop (for LSQR solver with constraints).
      size_t itsMajorLoopIterationNumber;

      /// @brief Sends the model from workers to master.
      /// @param[in] model The model to send.
      void sendModelToMaster(const scimath::Params &model) const;

      /// @brief Receives the model from workers on master.
      /// @param[in] model The model to receive.
      /// @param[in] rank The worker's rank.
      void receiveModelOnMaster(scimath::Params &model, int rank);
    };

  }
}
#endif // #ifndef DD_CALIBRATOR_PARALLEL_H
