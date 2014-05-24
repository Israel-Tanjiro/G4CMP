// $Id$
//
// Wrapper class to process a numerically tabulated electric field mesh
// and use QHull to interpolate the potential and field at arbitrary
// points in the envelope of the mesh.  The input file format is fixed:
// each line consists of four floating-point values, x, y and z in meters,
// and voltage in volts.

#include "G4CMPMeshElectricField.hh"
#include "G4SystemOfUnits.hh"
#include <vector>
#include <fstream>

using std::vector;


G4CMPMeshElectricField::G4CMPMeshElectricField(const G4String& EpotFileName)
  : G4ElectricField() {
  BuildInterp(EpotFileName);
}

G4CMPMeshElectricField::G4CMPMeshElectricField(const G4CMPMeshElectricField &p)
  : G4ElectricField(p), Interp(p.Interp) {;}

G4CMPMeshElectricField& G4CMPMeshElectricField::operator=(const G4CMPMeshElectricField &p) {
  if (this != &p) {				// Only copy if not self
    G4ElectricField::operator=(p);		// Call through to base
    Interp = p.Interp;
  }

  return *this;
}


void G4CMPMeshElectricField::BuildInterp(const G4String& EpotFileName) {
#ifdef G4CMP_DEBUG
  G4cout << "G4CMPMeshElectricField::Constructor: Creating Electric Field " 
	 << EpotFileName << G4endl;
#endif

  vector<vector<G4double> > tempX;

  vector<G4double> temp(4, 0);
  G4double x,y,z,v;

  std::ifstream epotFile(EpotFileName);
  while (epotFile.good() && !epotFile.eof())
  {
    epotFile >> x >> y >> z >> v;
    temp[0] = x*m;
    temp[1] = y*m;
    temp[2] = z*m;
    temp[3] = v*volt;
    tempX.push_back(temp);
  }
  epotFile.close();

  std::sort(tempX.begin(),tempX.end(), CDMS_Efield::vector_comp);

  vector<vector<G4double> > X(tempX.size(),vector<G4double>(3,0));
  vector<G4double> V(tempX.size(),0);

  for (size_t ii = 0; ii < tempX.size(); ++ii)
  {
    X[ii][0] = tempX[ii][0];
    X[ii][1] = tempX[ii][1];
    X[ii][2] = tempX[ii][2];
    V[ii] = tempX[ii][3];
  }

  Interp.UseMesh(X, V);
}


void G4CMPMeshElectricField::GetFieldValue(const G4double Point[4],
				     G4double *Efield) const {
  Interp.GetField(Point,Efield);
}


G4double G4CMPMeshElectricField::GetPotential(const G4double Point[4]) const {
  return Interp.GetPotential(Point);
}


G4bool CDMS_Efield::vector_comp(const vector<G4double>& p1,
				const vector<G4double>& p2) {
  if (p1[0] < p2[0])
    return true;
  else if (p2[0] < p1[0])
    return false;
  else if (p1[1] < p2[1])
    return true;
  else if (p2[1] < p1[1])
    return false;
  else if (p1[2] < p2[2])
    return true;
  else if (p2[2] < p1[2])
    return false;
  else
    return false;
}
