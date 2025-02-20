/// @file GridKernel.h
//
/// GridKernel: kernels for gridding and degridding
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
#ifndef ASKAP_SYNTHESIS_GRIDKERNEL_H_
#define ASKAP_SYNTHESIS_GRIDKERNEL_H_

// System includes
#include <string>

// ASKAPsoft includes
#include <casacore/casa/aips.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/BasicSL/Complex.h>

namespace askap {
    namespace synthesis {
        /// @brief Holder for gridding kernels
        ///
        /// @ingroup gridding
        class GridKernel {
            public:
                /// Information about gridding options
                static std::string info();

                /// Gridding kernel
                static void grid(casacore::Matrix<casacore::Complex>& grid,
                        casacore::Matrix<casacore::Complex>& convFunc,
                        const casacore::Complex& cVis, const int iu,
                        const int iv, const int support);

                /// Degridding kernel
                static void degrid(casacore::Complex& cVis,
                        const casacore::Matrix<casacore::Complex>& convFunc,
                        const casacore::Matrix<casacore::Complex>& grid,
                        const int iu, const int iv,
                        const int support);

        };
    }
}
#endif
