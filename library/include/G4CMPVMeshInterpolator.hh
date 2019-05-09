/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

// $Id$
//
// G4CMPVMeshInterpolator:  Base interface class for BiLinear and TriLinear
// interpolators, used by G4CMPMeshElectricField.  This base class allows
// use of a single pointer with a type selected at runtime for the field
// model.  The actual interpolation functionality is implemented entirely
// in the concrete subclasses.

#ifndef G4CMPVMeshInterpolator_h 
#define G4CMPVMeshInterpolator_h 

#include "G4Types.hh"
#include "G4ThreeVector.hh"
#include <array>
#include <vector>


class G4CMPVMeshInterpolator {
protected:
  // This class CANNOT be instantiated directly!
  G4CMPVMeshInterpolator() : TetraIdx(0), staleCache(true) {;}

public:
  virtual ~G4CMPVMeshInterpolator() {;}

  // Subclasses MUST implement this function to return duplicate of self
  virtual G4CMPVMeshInterpolator* Clone() const = 0;

public:
  // Replace values at mesh points without rebuilding tables
  void UseValues(const std::vector<G4double>& v);

  // Subclasses MUST implement these functions for their dimensionality

  // Replace existing mesh vectors and tetrahedra table
  // NOTE: Both 2D and 3D versions are give, subclasses should implement one
  void UseMesh(const std::vector<std::array<G4double,3> >& /*xyz*/,
	       const std::vector<G4double>& /*v*/,
	       const std::vector<std::array<G4int,4> >& /*tetra*/) {;}

  void UseMesh(const std::vector<std::array<G4double,2> >& /*xyz*/,
	       const std::vector<G4double>& /*v*/,
	       const std::vector<std::array<G4int,3> >& /*tetra*/) {;}

  // Evaluate mesh at arbitrary location, optionally suppressing errors
  virtual G4double GetValue(const G4double pos[], G4bool quiet=false) const = 0;
  virtual G4ThreeVector GetGrad(const G4double pos[], G4bool quiet=false) const = 0;

  // Write out mesh coordinates and tetrahedra table to text files
  virtual void SavePoints(const G4String& fname) const = 0;
  virtual void SaveTetra(const G4String& fname) const = 0;

protected:		// Data members available to subclasses directly
  std::vector<G4double> V;		// Values at mesh points
  // NOTE: Subclasses must define dimensional mesh coords and tetrahera

  mutable G4int TetraIdx;		// Last tetrahedral index used
  mutable G4bool staleCache;		// Flag if cache must be discarded
  G4int TetraStart;			// Start of tetrahedral searches

  
};

// SPECIAL:  Provide a way to write out array data directly (not in STL!)

template <typename T, size_t N>
inline std::ostream& operator<<(std::ostream& os, const std::array<T,N>& arr) {
  for (const T& ai: arr) os << ai << " ";
  return os;
}

#endif	/* G4CMPVMeshInterpolator_h */
