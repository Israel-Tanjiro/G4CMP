//  G4CMPPhononKVgTable.cpp
//  Created by Daniel Palken in 2014 for G4CMP

#include "G4CMPPhononKVgTable.hh"
#include "G4CMPPhononKVgMap.hh"
#include "G4PhononPolarization.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "matrix.hh"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;
using G4CMP::matrix;

// ++++++++++++++++++++++ G4CMPPhononKVgTable METHODS +++++++++++++++++++++++++

// magic values for use with interpolation
const G4double G4CMPPhononKVgTable::OUT_OF_BOUNDS = 9.0e299;
const G4double G4CMPPhononKVgTable::ERRONEOUS_INPUT = 1.0e99;

G4CMPPhononKVgTable::G4CMPPhononKVgTable(G4CMPPhononKVgMap* map, G4double xmin,
					 G4double xmax, G4int nx,
					 G4double ymin, G4double ymax,
					 G4double ny)
  : nxMin(xmin), nxMax(xmax), nxStep((nx>0)?(xmax-xmin)/nx:1.), nxCount(nx),
    nyMin(ymin), nyMax(ymax), nyStep((ny>0)?(ymax-ymin)/ny:1.), nyCount(ny),
    mapper(map) {
  generateLookupTable();
  generateMultiEvenTable();
}

G4CMPPhononKVgTable::~G4CMPPhononKVgTable() { clearQuantityMap(); }

void G4CMPPhononKVgTable::clearQuantityMap() {
  // common technique to free up the memory of a vector:
  quantityMap.clear(); // this alone actually does not free up the memory
  vector<vector<G4CMPBiLinearInterp> > newVec;
  quantityMap.swap(newVec);
}

// returns any quantity desired from the interpolation table
double G4CMPPhononKVgTable::interpGeneral(int mode, const G4ThreeVector& k,
					  int typeDesired) {
  double nx = k.unit().x(), ny = k.unit().y();

  // note: this method at present does not require nz
  return interpolateEven(nx, ny, mode, typeDesired);
}

// returns the unit vector pointing in the direction of Vg
G4ThreeVector G4CMPPhononKVgTable::interpGroupVelocity_N(int mode, const G4ThreeVector& k) {
  G4ThreeVector Vg(interpGeneral(mode, k, V_GX),
		   interpGeneral(mode, k, V_GY),
		   interpGeneral(mode, k, V_GZ));
  return Vg;
}

// ****************************** BUILD METHODS ********************************
/* sets up the vector of vectors of vectors used to store the data
   from the lookup table */
void G4CMPPhononKVgTable::setUpDataVectors() {
  // Preload outer vectors with correct structure, to avoid push-backs
  lookupData.resize(G4PhononPolarization::NUM_MODES,
		    vector<vector<double> >(NUM_DATA_TYPES));
}

// makes the lookup table for whatever material is specified
void G4CMPPhononKVgTable::generateLookupTable() {
  setUpDataVectors();

  // Kinematic data buffers fetched from Mapper (avoids memory churn)
  G4double vphase;
  G4ThreeVector slowness;
  G4ThreeVector vgroup;
  G4ThreeVector polarization;

  // ensures even spacing for x and y on a unit circle in the xy-plane
  G4ThreeVector n_dir;
  for (double nx=nxMin; nx<=nxMax; nx+=nxStep) {
    for (double ny=nyMin; ny<=nyMax; ny+=nyStep) {
      double rho2 = nx*nx + ny*ny;
      if (!doubLessThanApprox(rho2, 1.0, 1.0e-4)) break;

      n_dir.set(nx, ny, sqrt(1.-rho2));		// Unit vector
      // NOTE: G4CMPPhononKVgTable code has opposite theta,phi convention from Geant4!
      double theta=n_dir.theta(), phi=n_dir.phi();

      for (int mode = 0; mode < G4PhononPolarization::NUM_MODES; mode++) {
	vphase = mapper->getPhaseSpeed(mode, n_dir) / (m/s);
	vgroup = mapper->getGroupVelocity(mode, n_dir) / (m/s);
	slowness = mapper->getSlowness(mode, n_dir) / (s/m);	// 1/vphase
	polarization = mapper->getPolarization(mode, n_dir);

        lookupData[mode][N_X].push_back(n_dir.x());	// Wavevector dir.
	lookupData[mode][N_Y].push_back(n_dir.y());
	lookupData[mode][N_Z].push_back(n_dir.z());
	lookupData[mode][THETA].push_back(theta);	// Wavevector angles
	lookupData[mode][PHI].push_back(phi);
	lookupData[mode][S_X].push_back(slowness.x());	// Slowness direction
	lookupData[mode][S_Y].push_back(slowness.y());
	lookupData[mode][S_Z].push_back(slowness.z());
	lookupData[mode][S_MAG].push_back(slowness.mag());
	lookupData[mode][S_PAR].push_back(slowness.perp());
	lookupData[mode][V_P].push_back(vphase);
	lookupData[mode][V_G].push_back(vgroup.mag());
	lookupData[mode][V_GX].push_back(vgroup.x());
	lookupData[mode][V_GY].push_back(vgroup.y());
	lookupData[mode][V_GZ].push_back(vgroup.z());
	lookupData[mode][E_X].push_back(polarization.x());
	lookupData[mode][E_Y].push_back(polarization.y());
	lookupData[mode][E_Z].push_back(polarization.z());
      }
    }
  }
}

// *****************************************************************************

// $$$$$$$$$$$$$$$$$$$$$$$$$$$ EVEN INTERPOLATION HEADERS $$$$$$$$$$$$$$$$$$$$$$

/* given the (pointer to the) evenly spaced interpolation grid
   generated previously, this method returns an interpolated value for
   the data type already built into the G4CMPBiLinearInterp structure */
double G4CMPPhononKVgTable::interpolateEven(G4CMPBiLinearInterp& grid,
					    double nx, double ny) {
    // check that the n values we're interpolating at are possible:
    if (doubGreaterThanApprox((nx*nx + ny*ny), 1.0)) {
        cout << "ERROR: Cannot interpolate at (" << nx << ", " << ny << "): "
	     << "Pythagorean sum exceeds 1.0\n";
        return ERRONEOUS_INPUT; // exit method
    }
    // perform interpolation and return result:
    return grid.interp(nx, ny);
}

/* overloaded method interpolates on the vector of vectors of
   interpolation grid structures.  Much the same as its simpler version,
   but this one requires specification of the mode and data type
   desired */
double G4CMPPhononKVgTable::interpolateEven(double nx, double ny, int MODE,
					    int TYPE_OUT, bool SILENT) {
  // check that the n values we're interpolating at are possible:
  if (doubGreaterThanApprox((nx*nx + ny*ny), 1.0)) {
    cout << "ERROR: Cannot interpolate at (" << nx << ", " << ny << "): "
	 << "Pythagorean sum exceeds 1.0\n";
    return ERRONEOUS_INPUT; // exit method
  }
  
  // check that the data type we're asking for is not nx or ny
  if (TYPE_OUT == N_X || TYPE_OUT == N_Y) {
    if (!SILENT) cout << "Warning: Data type desired provided as input\n";
    // no sense in actually interpolating here, just return appropriate input
    return (TYPE_OUT==N_X ? nx : ny);
  }
    
  // perform interpolation and return result:
  return interpolateEven(quantityMap[MODE][TYPE_OUT], nx, ny);
}

/* sets up JUST ONE interpolation table for N_X and N_Y, which are evenly spaced.
   Any one kind of data (TYPE_OUT) can be read off of this table */
G4CMPBiLinearInterp 
G4CMPPhononKVgTable::generateEvenTable(int MODE,
				     G4CMPPhononKVgTable::DataTypes TYPE_OUT) {
  /* set up the two vectors of input components (N_X and N_Y) which
     will define the grid that will be interpolated on */
  size_t SIZE = lookupData[MODE][N_X].size();
  /* these grids are deliberately oversized, as our grid is evenly
     spaced but circular, not square. Make the matrix<double>s and vector<double>s
     pointers so they are not be destroyed when they go out of scope
     at the end of this method */
  vector<double> x1(nxCount+1, OUT_OF_BOUNDS);
  vector<double> x2(nyCount+1, OUT_OF_BOUNDS);
  matrix<double> dataVals(x1.size(), x2.size(), OUT_OF_BOUNDS);
  
  double lastLargestNx = nxMin, lastLargestNy = nyMin;
  x1[0] = lastLargestNx, x2[0] = lastLargestNy;
  int currentXindex = 1, currentYindex = 1;
  for (size_t i = 0; i<SIZE; i++) {
    double nxVal = lookupData[MODE][N_X][i];
    double nyVal = lookupData[MODE][N_Y][i];
    
    // if new largest x-value found...
    // following 2 if-statements will assemble vectors of x and y in ascending order:
    if (doubGreaterThanApprox(nxVal, lastLargestNx)) {
      x1[currentXindex] = lastLargestNx = nxVal;
      currentXindex++;
    } if (doubGreaterThanApprox(nyVal, lastLargestNy)) {
      x2[currentYindex] = lastLargestNy = nyVal;
      currentYindex++;
    }
    
    // figure out n_x-index of current data point:
    int x1vecIndex = (int)(nxVal / nxStep);
    if (doubApproxEquals(x1[x1vecIndex+1], nxVal) && x1vecIndex+1 < x1.size())
      x1vecIndex++;                           // resolve possible rounding issue
    else if (!doubApproxEquals(x1[x1vecIndex], nxVal))  // check
      cerr << "ERROR: Unanticipated n_x-index rounding behavior, nxVal= "
	   << nxVal << endl;

    // figure out n_y-index of current data point:
    int x2vecIndex = (int)(nyVal / nyStep);    // may round down to nearest integer index
    if (doubApproxEquals(x2[x2vecIndex+1], nyVal) && x2vecIndex+1 < x2.size())
      x2vecIndex++;                           // resolve possible rounding issue
    else if (!doubApproxEquals(x2[x2vecIndex], nyVal))  // check
      cerr << "ERROR: Unanticipated n_y-index rounding behavior, nyVal= "
	   << nyVal << endl;
    
    // fill in data point in matrix:
    // first check to make sure that point not already entered:
    if (!doubApproxEquals(dataVals[x1vecIndex][x2vecIndex], OUT_OF_BOUNDS,
			  1.0e-10, false))
      cerr << "ERROR: Attempting to enter a data value more than once @ "
	   << x1vecIndex << " " << x2vecIndex << endl;
    else // at long last, put data point in matrix
      dataVals[x1vecIndex][x2vecIndex] = lookupData[MODE][TYPE_OUT][i];
  }

  // create and return interpolation data structure, the output of this method
  return G4CMPBiLinearInterp(x1, x2, dataVals);
}

/* a method for setting up a vector of interpolation grids - it does
   so by calling the single grid constructor many times. While this is
   certainly not the absolute most efficient method concievable, the
   gains in efficiency that might be made are not terribly
   significant. Output is a vector of vectors of G4CMPBiLinearInterps, with the
   first index specifying mode, and the 2nd data type, as ususal */
void G4CMPPhononKVgTable::generateMultiEvenTable() {
  clearQuantityMap();

  for (int mode = 0; mode < G4PhononPolarization::NUM_MODES; mode++) {
    // create 2nd level vectors:
    vector<G4CMPBiLinearInterp> subTable;
    for (int dType = 0; dType < NUM_DATA_TYPES; dType++)
      // make individual tables:
      subTable.push_back(generateEvenTable(mode, (DataTypes)dType));
    quantityMap.push_back(subTable);
  }
}

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// +++++++++++++++++++++++++++++ COMPLETE LOOKUP TABLE +++++++++++++++++++++++++
void G4CMPPhononKVgTable::write() {
  // <^><^><^><^><^><^><^><^><^> INITIAL SETUP <^><^><^><^><^><^><^><^><
  // set up the lookup table as a data file:
  string fName = mapper->getLatticeName()+"LookupTable.txt";
  ofstream lookupTable(fName.c_str());

  lookupTable.setf(ios::left);

  // file header:
  /* the next few steps ensure the same number of header lines looked for when reading
     from the lookup table */
  vector<string> headerLines;
  headerLines.push_back("Columns:");
  headerLines.push_back("mode");
  headerLines.push_back("----");
  for (int i = 0; i < NUM_DATA_TYPES; i++) {
    headerLines[1] += " | " + getDataTypeName(i);
    headerLines[2] += "------";
  }

  for (int i=0; i < headerLines.size(); i++)
    lookupTable << "# " << headerLines[i] << endl;
  // <^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><^><

  // Reproduce (x,y) loops used to fill lookup table, to get indexing

  int entry=0;
  for (int ix=0; ix<=nxCount; ix++) {
    double nx = nxMin + nxStep*ix;

    for (int iy=0; iy<=nyCount; iy++) {
      double ny = nyMin + nyStep*iy;

      double rho2 = nx*nx + ny*ny;
      if (!doubLessThanApprox(rho2, 1.0, 1.0e-4)) break;

      for (int mode = 0; mode < G4PhononPolarization::NUM_MODES; mode++) {
	lookupTable << setw(18) << G4PhononPolarization::Label(mode);
	for (size_t cols=0; cols<NUM_DATA_TYPES; cols++) {
	  lookupTable << setw(18) << lookupData[mode][cols][entry];
	}
	lookupTable << endl;
      }

      entry++;		// Increment counter, corresponding to push_back()
    }
  }
}

// given the data type index, returns the abbreviation (s_x, etc...)
// make sure to update this method if the data types are altered
string G4CMPPhononKVgTable::getDataTypeName(int TYPE) {
  switch (TYPE) {
  case N_X:   return "n_x";
  case N_Y:   return "n_y";
  case N_Z:   return "n_z";
  case THETA: return "theta";
  case PHI:   return "phi";
  case S_X:   return "s_x";
  case S_Y:   return "s_y";
  case S_Z:   return "s_z";
  case S_MAG: return "s";
  case S_PAR: return "s_par";
  case V_P:   return "v_p";
  case V_G:   return "V_g";
  case V_GX:  return "V_gx";
  case V_GY:  return "V_gy";
  case V_GZ:  return "V_gz";
  case E_X:   return "e_x";
  case E_Y:   return "e_y";
  case E_Z:   return "e_z";
  default:  throw("ERROR: not al valid data type");
  } throw("ERROR: not al valid data type");
}
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ############################# EXTRANEOUS HEADERS ############################

/* takes two Doubs and returns 'true' if they are sufficiently close
   to one another.  This is to get around the often annoying issue of
   doubles not quite being equal, even though they should be, in an
   exact sense. Set TOL to 0 to do a normal '==' check */
bool doubApproxEquals(double D1, double D2, double TOL, bool USE_ABS_TOL)
{
    // determine value of tolerance given type of tolerance to use
    double absTol = TOL;                                  // use absolute tolerance
    if (!USE_ABS_TOL) absTol = ((D1 + D2)/2.0) * TOL;   // use relative tolerance
    
    // check for (near) equality:
    return (D1-D2 >= -absTol && D1-D2 <= absTol);
}

/* takes two Doubs and returns true if the first is significantly
   greater than the second, with "significantly" here meaning by an
   amount more than the tolerance. Set TOL to 0 to do a normal '>'
   check */
bool doubGreaterThanApprox(double D1, double D2, double TOL, bool USE_ABS_TOL)
{
    // determine value of tolerance given type of tolerance to use
    double absTol = TOL;                                  // use absolute tolerance
    if (!USE_ABS_TOL) absTol = ((D1 + D2)/2.0) * TOL;   // use relative tolerance
    
    // check for (near) equality:
    return (D1 > D2 + absTol);	                  // DEFINITELY greater than
}

/* takes two Doubs and returns true if the first is significantly less
   than the second, with "significantly" here meaning by an amount more
   than the tolerance Set TOL to 0 to do a normal '<' check*/
bool doubLessThanApprox(double D1, double D2, double TOL, bool USE_ABS_TOL)
{
    // determine value of tolerance given type of tolerance to use
    double absTol = TOL;                                  // use absolute tolerance
    if (!USE_ABS_TOL) absTol = ((D1 + D2)/2.0) * TOL;   // use relative tolerance
    
    // check for (near) equality:
    return (D1 < D2 - absTol);                  // DEFINITELY less than
}
// #############################################################################
