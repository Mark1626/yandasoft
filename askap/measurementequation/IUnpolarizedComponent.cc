/// @file
///
/// @brief An unpolarized component (stokes Q,U and V always give 0)
/// @details
///     This is a structural type describing an unpolarized component.
///     It implements calculate methods of the base interface via new 
///     calculate methods, which don't require pol parameter
///     and always return stokes I. Having a separate
///     type allows to avoid unnecessary loops in polarization in
///     ComponentEquation, by testing the type with dynamic_cast. 
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>
/// 

// own includes
#include <askap/measurementequation/IUnpolarizedComponent.h>
#include <askap/measurementequation/IParameterizedComponent.h>

/// @brief calculate visibilities for this component
/// @details This variant of the method calculates just the visibilities
/// (without derivatives) for a number of frequencies. The result is stored 
/// to the provided buffer, which is resized to twice the given 
/// number of spectral points. Complex values are stored as two consequtive 
/// double values. The first one is a real part, the second is imaginary part.
/// @param[in] uvw  baseline spacings (in metres)
/// @param[in] freq vector of frequencies to do calculations for
/// @param[in] pol required polarization 
/// @param[out] result an output buffer used to store values
void askap::synthesis::IUnpolarizedComponent::calculate(
                    const casacore::RigidVector<casacore::Double, 3> &uvw,
                    const casacore::Vector<casacore::Double> &freq,
                    casacore::Stokes::StokesTypes pol,
                    std::vector<double> &result) const
{
  if (stokesIndex(pol)) {
      // Q,U or V requested
      result.resize(2*freq.nelements(),0.);
  } else {
      // stokes I requested
      calculate(uvw,freq,result);
  }
}                    
  
/// @brief calculate visibilities and derivatives for this component
/// @details This variant of the method does simultaneous calculations of
/// the values and derivatives. The result is written to the provided buffer.
/// See the another version of this method for sizes/description of the buffer
/// structure.
/// @param[in] uvw  baseline spacings (in metres)
/// @param[in] freq vector of frequencies to do calculations for
/// @param[in] pol required polarization 
/// @param[out] result an output buffer used to store values
void askap::synthesis::IUnpolarizedComponent::calculate(
                    const casacore::RigidVector<casacore::Double, 3> &uvw,
                    const casacore::Vector<casacore::Double> &freq,
                    casacore::Stokes::StokesTypes pol,
                    std::vector<casacore::AutoDiff<double> > &result) const
{
  if (stokesIndex(pol)) {
      // Q,U or V requested
      const askap::synthesis::IParameterizedComponent *pc = 
             dynamic_cast<const askap::synthesis::IParameterizedComponent*>(this);
      if (pc != NULL) {
          result.resize(2*freq.nelements(), casacore::AutoDiff<double>(0.,
                        pc->nParameters()));
      }        
      result.resize(2*freq.nelements(), casacore::AutoDiff<double>(0.,0));
  } else {
      // stokes I requested
      calculate(uvw,freq,result);
  }
}                                        
