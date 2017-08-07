/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPVProcess.cch
/// \brief Implementation of the G4CMPVProcess base class
//
// $Id$
//
// 20170601  New abstract base class for all G4CMP processes
// 20170802  Add registration of external scattering rate (MFP) model

#include "G4CMPVProcess.hh"
#include "G4CMPConfigManager.hh"
#include "G4CMPVScatteringRate.hh"
#include "G4ForceCondition.hh"
#include "G4SystemOfUnits.hh"


// Constructor and destructor

G4CMPVProcess::G4CMPVProcess(const G4String& processName,
			     G4CMPProcessSubType stype)
  : G4VDiscreteProcess(processName, fPhonon), G4CMPProcessUtils(),
    rateModel(0) {
  verboseLevel = G4CMPConfigManager::GetVerboseLevel();
  SetProcessSubType(stype);
}

G4CMPVProcess::~G4CMPVProcess() {
  delete rateModel; rateModel=0;
}


// Register utility class for computing scattering rate for MFP
// NOTE:  Takes ownership of model for deletion; deletes any previous version

void G4CMPVProcess::UseRateModel(G4CMPVScatteringRate* model) {
  if (!model) model->SetVerboseLevel(verboseLevel);

  if (rateModel) delete rateModel;		// Avoid memory leaks!
  rateModel = model;
}


// Configuration

void G4CMPVProcess::StartTracking(G4Track* track) {
  G4VProcess::StartTracking(track);	// Apply base class actions
  LoadDataForTrack(track);
}

void G4CMPVProcess::EndTracking() {
  G4VProcess::EndTracking();		// Apply base class actions
  ReleaseTrack();
}


// Compute MFP using track velocity and scattering rate

G4double G4CMPVProcess::GetMeanFreePath(const G4Track& aTrack, G4double,
					G4ForceCondition* condition) {
  *condition = (rateModel && rateModel->IsForced()) ? Forced : NotForced;

  G4double rate = rateModel ? rateModel->Rate(aTrack) : 0.;
  G4double mfp  = rate>0. ? GetVelocity(aTrack)/rate : DBL_MAX;

  if (verboseLevel>1) {
    G4cout << GetProcessName() << " rate = " << rate/hertz << " Hz" << G4endl;
    if (rate>0.)
      G4cout << GetProcessName() << "  MFP = " << mfp/m << " m" << G4endl;
  }

  return mfp;
}
