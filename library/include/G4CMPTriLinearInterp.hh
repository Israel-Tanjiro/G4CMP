/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

// $Id$
//
// 20170525  Drop unnecessary empty destructor ("rule of five" pattern)
// 20180525  Add "quiet" flag to suppress "outside of hull" messages
// 20180904  Add constructor to directly load mesh definitions
// 20180926  Add functions to write points, tetrahedra etc. to files.
//		Add starting index for tetrahedral traversal
// 20190226  Provide accessor to replace potentials at mesh points
// 20190404  Change "point" to "point3d" to make way for 2D interpolator.
// 20190508  Move some 2D/3D common features to new base class

#ifndef G4CMPTriLinearInterp_h 
#define G4CMPTriLinearInterp_h 

#include "G4CMPVMeshInterpolator.hh"
#include "G4ThreeVector.hh"
#include <vector>
#include <map>
#include <array>

using point3d = std::array<G4double,3>;
using tetra3d = std::array<G4int,4>;

class G4CMPTriLinearInterp : public G4CMPVMeshInterpolator {
public:
  // Uninitialized version; user MUST call UseMesh()
  G4CMPTriLinearInterp() : G4CMPVMeshInterpolator() {;}

  // Mesh coordinates and values only; uses QHull to generate triangulation
  G4CMPTriLinearInterp(const std::vector<point3d>& xyz,
		       const std::vector<G4double>& v);

  // Mesh points and pre-defined triangulation
  G4CMPTriLinearInterp(const std::vector<point3d>& xyz,
		       const std::vector<G4double>& v,
		       const std::vector<tetra3d>& tetra);

  // Cloning function to allow making type-matched copies
  virtual G4CMPVMeshInterpolator* Clone() const {
    return new G4CMPTriLinearInterp(X, V, Tetrahedra);
  }

  // User initialization or re-initialization
  void UseMesh(const std::vector<point3d>& xyz, const std::vector<G4double>& v);

  void UseMesh(const std::vector<point3d>& xyz,
	       const std::vector<G4double>& v,
	       const std::vector<tetra3d>& tetra);

  // Evaluate mesh at arbitrary location, optionally suppressing errors
  G4double GetValue(const G4double pos[], G4bool quiet=false) const;
  G4ThreeVector GetGrad(const G4double pos[], G4bool quiet=false) const;

  void SavePoints(const G4String& fname) const;
  void SaveTetra(const G4String& fname) const;

private:
  std::vector<point3d > X;
  std::vector<tetra3d> Tetrahedra;
  std::vector<tetra3d> Neighbors;

  mutable std::map<G4int,G4int> qhull2x;	// Used by QHull for meshing
  mutable G4ThreeVector cachedGrad;

  // Lists of tetrahedra with shared vertices, for generating neighbors table
  std::vector<tetra3d> Tetra012;	// Duplicate tetrahedra lists
  std::vector<tetra3d> Tetra013;	// Sorted on vertex triplets
  std::vector<tetra3d> Tetra023;
  std::vector<tetra3d> Tetra123;

  void BuildTetraMesh();	// Builds mesh from pre-initialized 'X' array
  void FillNeighbors();		// Generate Neighbors table from tetrahedra

  // Function pointer for comparison operator to use search for facets
  using TetraComp = G4bool(*)(const tetra3d&, const tetra3d&);

  G4int FindNeighbor(const std::array<G4int,3>& facet, G4int skip) const;
  G4int FindTetraID(const std::vector<tetra3d>& tetras,
		    const tetra3d& wildTetra, G4int skip,
		    TetraComp tLess) const;
  G4int FirstInteriorTetra();	// Lowest tetra index with all facets shared

  void FindTetrahedron(const G4double point[4], G4double bary[4],
		       G4bool quiet=false) const;
  G4int FindPointID(const std::vector<G4double>& point, const G4int id) const;

  void Cart2Bary(const G4double point[4], G4double bary[4]) const;
  G4double BaryNorm(G4double bary[4]) const;
  void BuildT4x3(G4double ET[4][3]) const;
  G4double Det3(const G4double matrix[3][3]) const;
  void MatInv(const G4double matrix[3][3], G4double result[3][3]) const;
};

#endif	/* G4CMPTriLinearInterp */
