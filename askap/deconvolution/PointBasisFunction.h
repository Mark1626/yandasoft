/// @file
/// @brief Point source basis function
/// @details Holder for point source function
/// @ingroup Deconvolver
///
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
/// @author Tim Cornwell <tim.cornwell@csiro.au>
///

#ifndef ASKAP_SYNTHESIS_POINTBASISFUNCTION_H
#define ASKAP_SYNTHESIS_POINTBASISFUNCTION_H

#include <string>

#include <boost/shared_ptr.hpp>
#include <casacore/casa/aips.h>
#include <casacore/casa/Arrays/Array.h>

#include <askap/deconvolution/BasisFunction.h>

namespace askap {

    namespace synthesis {

        /// @brief Point source basis function
        /// @details This is the simplest Basis Function, holding a
        /// single point at the centre of the image. We don't use this for
        /// anything other than testing since the overhead is too large.
        /// @ingroup Deconvolver

        template<typename T>
        class PointBasisFunction : public BasisFunction<T> {

            public:
                typedef boost::shared_ptr<PointBasisFunction<T> > ShPtr;

                /// @brief Construct empty
                /// @details Construct a point source basis function. This has only
                /// one plane.
                PointBasisFunction();

                /// @brief Construct from a specified shape
                /// @details Construct a point source basis function. This has only
                /// one plane.
                /// param[in] shape Shape of desired basis function on the first two axes.
                explicit PointBasisFunction(const casacore::IPosition& shape);

                /// @brief Construct from a specified shape
                /// @details Construct a point source basis function. This has only
                /// one plane.
                /// param[in] shape Shape of desired basis function on the first two axes.
                virtual void initialise(const casacore::IPosition& shape);
        };

    } // namespace synthesis

} // namespace askap

#include <askap/deconvolution/PointBasisFunction.tcc>

#endif
