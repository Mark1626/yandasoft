/// @file
///
/// WienerPreconditioner: Precondition the normal equations
///                       by applying a Wiener filter
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
/// original code by Urvashi Rau <rurvashi@aoc.nrao.edu>
///
#ifndef SYN_WEINER_PRECONDITIONER_H_
#define SYN_WEINER_PRECONDITIONER_H_

#include <casacore/casa/aips.h>
#include <casacore/casa/Arrays/Vector.h>
//#include <casacore/lattices/Lattices/ArrayLattice.h>
#include <askap/scimath/fitting/Axes.h>

#include <Common/ParameterSet.h>
#include <boost/shared_ptr.hpp>

#include <askap/measurementequation/IImagePreconditioner.h>
#include <askap/measurementequation/GaussianTaperCache.h>

namespace askap
{
  namespace synthesis
  {
    /// @brief Precondition the normal equations via a Wiener filter
    ///
    /// @details It constructs a Wiener filter from the PSF
    /// and applies it to the PSF and current Residual image
    /// @ingroup measurementequation
    class WienerPreconditioner : public IImagePreconditioner
    {
    public:

      /// @brief default constructor with zero noise power
      /// @note do we really need it?
      WienerPreconditioner();

      /// @brief constructor with explicitly defined noise power
      /// @param[in] noisepower parameter of the
      /// @param[in] normalise if true, PSF is normalised during filter construction
      WienerPreconditioner(float noisepower, bool normalise);

      /// @brief constructor with explicitly defined robustness
      /// @details In this version, the noise power is calculated from
      /// the robustness parameter
      /// @param[in] robustness robustness parameter (roughly matching Briggs' weighting)
      /// @note Normalisation of PSF is always used when noise power is defined via robustness
      explicit WienerPreconditioner(float robustness);

      /// @brief copy constructor
      /// @param[in] other an opject to copy from
      WienerPreconditioner(const WienerPreconditioner &other);

      /// @brief Clone this object
      /// @return shared pointer to a cloned copy
      virtual IImagePreconditioner::ShPtr clone();

      /// @brief Apply preconditioning to Image Arrays
      /// @details This is the actual method, which does preconditioning.
      /// It is applied to the PSF as well as the current residual image.
      /// @param[in] psf array with PSF
      /// @param[in] dirty array with dirty image
      /// @return true if psf and dirty have been altered
      virtual bool doPreconditioning(casacore::Array<float>& psf,
                                     casacore::Array<float>& dirty,
                                     casacore::Array<float>& pcf) const;

      /// @brief static factory method to create preconditioner from a parset
      /// @details
      /// @param[in] parset subset of parset file (with preconditioner.Wiener. removed)
      /// @return shared pointer
      static boost::shared_ptr<WienerPreconditioner> createPreconditioner(const LOFAR::ParameterSet &parset,
                                                                          const bool useCachedPcf = false);

    protected:
      /// @brief enable Filter tapering
      /// @details Wiener filter can optionally be tapered in the image domain, so it is not extended over
      /// the whole field of view.
      /// @param[in] fwhm full width at half maximum of the taper given in image cells
      void enableTapering(double fwhm);

    private:
      /// @brief assignment operator, to ensure it is not called
      /// @param[in] other object to copy from
      /// @return reference to itself
      WienerPreconditioner& operator=(const WienerPreconditioner &other);

      /// @brief calculate a threshold to use to clean up the PCF
      /// @details The preconditioner function can have a rumble of low-level
      /// Fourier components due to various issues (sharp edges in the image,
      /// tapering, de-warping). These need to be dealt with before forming the
      /// Wiener filter. One approach to this is thresholding out small values.
      /// @param[in] input array from which to determine threshold
      /// @return threshold value
      float pcfThreshold(casacore::Matrix<casacore::Complex>& pcf) const;

      /// @brief Parameter of the filter
      /// @details Depending on the mode, it can either be the noise power value directly or the
      /// robustness parameter roughly matching Briggs' weighting.
      float itsParameter;

      /// @brief Use normalised PSF in filter construction?
      bool itsDoNormalise;

      /// @brief true, if parameter is robustness, false if it is the noise power
      bool itsUseRobustness;

      /// @brief cache for intermediate Wiener filtering PCF weight array
      // Declare as statics so that the time-consuming PCF weights can be stored for all preconditioners.
      // It needs to be controllable though, for times when different solvers have different PSFs (e.g.
      // when different frequencies are deconvolved on the same MPI rank.
      static bool itsUseCachedPcf;
      static double itsAveWgtSum;
      static casacore::Matrix<float> itsPcf;

      /// @brief cache for Wiener filter array
      // do not make this static, since in general different solvers will require different preconditioning.
      mutable casacore::Matrix<casacore::Complex> itsWienerfilter;

      /// @brief gaussian taper in the image domain (in pixels)
      /// @details fwhm is stored inside cache class.
      boost::shared_ptr<GaussianTaperCache> itsTaperCache;
   };

  }
}
#endif
