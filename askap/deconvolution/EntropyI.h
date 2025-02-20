/// @file EntropyI.h
/// @brief Base class for a deconvolver
/// @details This interface class defines a deconvolver used to estimate an
/// image from a dirty image, psf optionally using a mask and a weights image.
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

#ifndef ASKAP_SYNTHESIS_ENTROPYI_H
#define ASKAP_SYNTHESIS_ENTROPYI_H

#include <askap/deconvolution/EntropyBase.h>

namespace askap {

    namespace synthesis {

        // Base class
        template<class T>
        class EntropyI : public EntropyBase<T> {
            public:

                typedef boost::shared_ptr<EntropyI<T> > ShPtr;

                enum GRADTYPE {H = 0, C, F, J };

                // calculate the entropy for the whole image
                virtual T entropy(const casacore::Array<T>& model);

                // calculate the entropy for the whole image
                virtual T entropy(const casacore::Array<T>& model, const casacore::Array<T>& mask);

                // calculate the gradient entropy for the whole image
                virtual void gradEntropy(casacore::Array<T>& gradH, casacore::Array<T>& rHess,
                                         const casacore::Array<T>& model);

                // calculate the gradient entropy for the whole image
                virtual void gradEntropy(casacore::Array<T>& gradH, casacore::Array<T>& rHess,
                                         const casacore::Array<T>& model, const casacore::Array<T>& mask);

            protected:

        };

    } // namespace synthesis

} // namespace askap

#include <askap/deconvolution/EntropyI.tcc>

#endif
