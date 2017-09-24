/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPChargeCloud.cc
/// \brief Implementation of the G4CMPChargeCloud class
///   Generates a collection of points distributed in a sphere around a
///   given center.  The distribution is uniform in angle, falls off
///   linearly with radius.  Size of cloud determined by lattice structure
///   (eight per unit cell for diamond).  If enclosing volume is specified
///   sphere will be "folded" inward at bounding surfaces.
///
// $Id$

#include "G4CMPChargeCloud.hh"
#include "G4LatticePhysical.hh"
#include "G4LogicalVolume.hh"
#include "G4RandomDirection.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"
#include "Randomize.hh"
#include <math.h>


// Constructors and destructor

G4CMPChargeCloud::G4CMPChargeCloud(const G4LatticeLogical* lat,
				   const G4VSolid* solid)
  : verboseLevel(0), theLattice(lat), theSolid(solid) {;}

G4CMPChargeCloud::G4CMPChargeCloud(const G4LatticePhysical* lat,
				   const G4VSolid* solid)
  : G4CMPChargeCloud(lat->GetLattice(), solid) {;}

G4CMPChargeCloud::G4CMPChargeCloud(const G4VSolid* solid)
  : G4CMPChargeCloud((const G4LatticeLogical*)nullptr, solid) {;}

G4CMPChargeCloud::G4CMPChargeCloud(const G4LatticeLogical* lat,
				   const G4VPhysicalVolume* vol)
  : verboseLevel(0), theLattice(lat) {
  UseVolume(vol);
}

G4CMPChargeCloud::G4CMPChargeCloud(const G4LatticePhysical* lat,
				   const G4VPhysicalVolume* vol)
  : G4CMPChargeCloud(lat->GetLattice(), vol) {;}

G4CMPChargeCloud::G4CMPChargeCloud(const G4VPhysicalVolume* vol)
  : G4CMPChargeCloud((const G4LatticeLogical*)nullptr, vol) {;}

G4CMPChargeCloud::~G4CMPChargeCloud() {;}


// Extract shape from placement volume

void G4CMPChargeCloud::UseVolume(const G4VPhysicalVolume* vol) {
  theSolid = vol ? vol->GetLogicalVolume()->GetSolid() : nullptr;
}


// Fill list of positions around specified center, within optional volume

const std::vector<G4ThreeVector>& 
G4CMPChargeCloud::Generate(G4int npos, const G4ThreeVector& center) {
  if (verboseLevel) {
    G4cout << "G4CMPChargeCloud::Generate " << npos << " @ " << center
	   << G4endl;
  }

  theCloud.clear();
  theCloud.reserve(npos);

  G4double radius = GetRadius(npos);	// Radius of cloud for average density

  for (G4int i=0; i<npos; i++) {
    theCloud.push_back(GeneratePoint(radius)+center);
    if (theSolid) AdjustToVolume(theCloud.back());
  }

  return theCloud;
}


// Compute radius of cloud for average density matching unit cell

G4double G4CMPChargeCloud::GetRadius(G4int npos) const {
  // FIXME:  Approximate unit cell as cubic for now
  G4double cellVol = (theLattice ? (theLattice->GetBasis(0).mag() *
				    theLattice->GetBasis(1).mag() *
				    theLattice->GetBasis(2).mag())
		      : 1.*nm*nm*nm );		// No lattice, use 1 nm^3

  if (verboseLevel>2) {
    G4cout << "Lattice " << theLattice->GetName() << " has basis vectors"
	   << " with lengths " << theLattice->GetBasis(0).mag()/nm
	   << " " << theLattice->GetBasis(1).mag()/nm
	   << " " << theLattice->GetBasis(2).mag()/nm << " nm" << G4endl;
  }

  // FIXME:  Set unit cell occupancy from lattice symmetry group
  const G4double AtomsPerCell = 8;

  G4double length = cbrt(cellVol*npos/AtomsPerCell);	// Number of cells

  if (verboseLevel>1) {
    G4cout << "G4CMPChargeCloud unit cell " << cbrt(cellVol)/nm << " nm "
	   << " diameter " << length/nm << " nm for " << npos << " charges"
	   << G4endl;
  }

  return length/2.;		// Radius is half of cubic side length
}


// Generate random point in specified sphere

G4ThreeVector G4CMPChargeCloud::GeneratePoint(G4double rmax) const {
  G4double rndm = G4UniformRand();
  G4double r = rmax*(1.-sqrt(1.-rndm*rndm));	// Linear from r=0 to rmax

  return r*G4RandomDirection();
}


// Adjust specified point to be inside volume

void G4CMPChargeCloud::AdjustToVolume(G4ThreeVector& point) const {
  if (!theSolid) return;			// Avoid unnecessary work

  // Keep adjusting point until interior to volume
  G4ThreeVector norm;
  G4double dist = 0.;

  G4int ntries = 100;				// Avoid infinite loops
  while (--ntries > 0 && theSolid->Inside(point) == kOutside) {
    norm = theSolid->SurfaceNormal(point);
    dist = theSolid->DistanceToIn(point);

    point -= 2.*dist*norm;	// Reflect point through boundary surface
  }
}
