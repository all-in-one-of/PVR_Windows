//-*-c++-*--------------------------------------------------------------------//

/*
    This file is part of PVR. Copyright (C) 2012 Magnus Wrenninge

    PVR is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PVR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//----------------------------------------------------------------------------//

/*! \file MitchellInterp.h
  Contains the MitchellInterp class and related functions.
 */

//----------------------------------------------------------------------------//

#ifndef __INCLUDED_PVR_MITCHELLINTERP_H__
#define __INCLUDED_PVR_MITCHELLINTERP_H__

//----------------------------------------------------------------------------//
// Includes
//----------------------------------------------------------------------------//

// System headers

#include <boost/shared_ptr.hpp>

// Library headers

#include <Field3D/FieldInterp.h>

// Project headers

//----------------------------------------------------------------------------//
// Namespaces
//----------------------------------------------------------------------------//

namespace Field3D {

//----------------------------------------------------------------------------//
// MitchellFieldInterp
//----------------------------------------------------------------------------//

template <class Data_T>
class MitchellFieldInterp : public Field3D::FieldInterp<Data_T>
{
 public:
  
  // Typedefs ------------------------------------------------------------------

  typedef boost::intrusive_ptr<MitchellFieldInterp> Ptr;

  // RTTI replacement ----------------------------------------------------------

  typedef MitchellFieldInterp class_type;
  DEFINE_FIELD_RTTI_CONCRETE_CLASS;

  static const char *staticClassName()
  {
    return "MitchellFieldInterp";
  }

  static const char* classType()
  {
    return class_type::ms_classType.name();
  }
  
  // From FieldInterp ----------------------------------------------------------

  virtual Data_T sample(const Field<Data_T> &data, const Imath::V3d &vsP) const
  {
    // Voxel centers are at .5 coordinates
    // NOTE: Don't use contToDisc for this, we're looking for sample
    // point locations, not coordinate shifts.
    FIELD3D_VEC3_T<double> p(vsP - FIELD3D_VEC3_T<double>(0.5));
    
    // Data window
    const Imath::Box3i &dw = data.dataWindow();
  
    // Lower left corner
    Imath::V3i c(static_cast<int>(floor(p.x)) - 1, 
          static_cast<int>(floor(p.y)) - 1, 
          static_cast<int>(floor(p.z)) - 1);

    // Position of filter
    Imath::V3f x(p.x - floor(p.x), p.y - floor(p.y), p.z - floor(p.z));
    
    pvr::Filter::MitchellNetravali filter;

	Data_T values[pvr::Filter::MitchellNetravali::width][pvr::Filter::MitchellNetravali::width][pvr::Filter::MitchellNetravali::width];
    for (int k = c.z, ki = 0; k < c.z + pvr::Filter::MitchellNetravali::width; k++, ki++) {
      for (int j = c.y, ji = 0; j < c.y + pvr::Filter::MitchellNetravali::width; j++, ji++) {
        for (int i = c.x, ii = 0; i < c.x + pvr::Filter::MitchellNetravali::width; i++, ii++) {
          int iIdx = std::min(std::max(i, dw.min.x), dw.max.x);
          int jIdx = std::min(std::max(j, dw.min.y), dw.max.y);
          int kIdx = std::min(std::max(k, dw.min.z), dw.max.z);
          values[ii][ji][ki] = data.value(iIdx, jIdx, kIdx);
        }
      }
    }
    return pvr::Filter::filter3D<Data_T,pvr::Filter::MitchellNetravali>(x.x, x.y, x.z, values, filter);
  }

private:

  // Static data members -------------------------------------------------------

  static TemplatedFieldType<MitchellFieldInterp<Data_T> > ms_classType;

  // Typedefs ------------------------------------------------------------------

  //! Convenience typedef for referring to base class
  typedef RefBase base;

};

//----------------------------------------------------------------------------//
// Static data member instantiation
//----------------------------------------------------------------------------//

FIELD3D_CLASSTYPE_TEMPL_INSTANTIATION(MitchellFieldInterp);

//----------------------------------------------------------------------------//

} // namespace Field3D

//----------------------------------------------------------------------------//

#endif // Include guard

//----------------------------------------------------------------------------//
