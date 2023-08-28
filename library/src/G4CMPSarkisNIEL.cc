/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPSarkisNIEL.hh
/// \brief Non-ionizing energy loss calculation from Sarkis 2023.
///
/// Link to the paper:https://journals.aps.org/pra/abstract/10.1103/PhysRevA.107.062811.
//

// 20230721  David Sadek - University of Florida (david.sadek@ufl.edu)

// This ionization model was obtained from the Sarkis paper referenced above for Silicon ONLY so it deos not have (Z,A) dependence. The code will check the effective Z and A of the input material, if the effZ and effA are within +/-1 of Silicon Z and A, the Sarkis model will be used, else, Lindhard(LewinSmith) model will be used for NIEL calculations.


// The model is obtained in the range of ~50 eV to 3 MeV above which Lindahard(LewinSmith) model will be used for NIEL calculations


#include "globals.hh"
#include "G4CMPSarkisNIEL.hh"
#include "G4ExceptionSeverity.hh"
#include "G4Material.hh"
#include "G4Pow.hh"
#include "G4SystemOfUnits.hh"
#include "G4InterpolationManager.hh"
#include "G4DataInterpolation.hh"
#include <fstream>
#include <algorithm>
#include <cmath>


G4CMPSarkisNIEL::G4CMPSarkisNIEL() :  SiZ(14.0), SiA(28.09), fDataDir(G4CMPConfigManager::GetLatticeDir()) {
    
    lVector = G4PhysicsLinearVector(false);
}


G4double G4CMPSarkisNIEL::
PartitionNIEL(G4double energy, const G4Material *material,G4double Zin = 0.,
		G4double Ain = 0.) const {
  if (!material) {
    G4Exception("G4CMPSarkisNIEL", "G4CMP1000", FatalErrorInArgument,
		  "No material passed to partition function");
    return 1.;		// Won't get here after FATAL error
  } 
  // Get effective Z,A of the material
  const G4double Z = GetEffectiveZ(material);
  const G4double A = GetEffectiveA(material) / (g / mole);
    
  // Check if the material is silicon or similar (within +-1 of Z and A of silicon)
    
  if (G4doubleabs(Z - SiZ) <= 1.0 && G4doubleabs(A - SiA) <= 1.0) {
      G4Exception("G4CMPImpactTunlNIEL", "G4CMP1005", JustWarning,
                  "Sarkis model is obtained in the range of 50 eV to 3 MeV. Above 3 MeV, Lindhard model will be used.");
    
    // Sarkis model below 3 MeV 
    if (energy <= 3 * MeV) {
        // Sarkis model function
        fData = fDataDir + "/NIEL/SarkisSiIonYield.txt";                // full path to the data file
        inputFile.Open(fData)                                           // open the data file
        lVector.Retrieve(inputFile, false);                             // load the data from the text file
        
        return lVector.Value(energy, idx);                             // do the interpolation and return the yield value
        
    // Lindhard model above 3 MeV
    } else {
        return G4CMPLewinSmithNIEL::PartitionNIEL(energy, material, Zin, Ain);
    }
  } else {
      G4Exception("G4CMPImpactTunlNIEL", "G4CMP1004", JustWarning, "The input material is not Silicon. The Lindhard model will be used for NIEL calculation.");
      return G4CMPLewinSmithNIEL::PartitionNIEL(energy, material, Zin, Ain);
  }
}