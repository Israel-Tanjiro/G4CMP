//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//

#ifndef XWrapperContinuousDiscreteProcess_h
#define XWrapperContinuousDiscreteProcess_h 1

#include "G4ios.hh"
#include "globals.hh"
#include "G4VContinuousDiscreteProcess.hh"
#include "ChannelingParticleUserInfo.hh"

class G4Material;

class XWrapperContinuousDiscreteProcess : public G4VContinuousDiscreteProcess
{
public:
    
    XWrapperContinuousDiscreteProcess(const G4String& processName ="XWrapperContinuousDiscreteProcess" );
    XWrapperContinuousDiscreteProcess(const G4String& processName, G4VContinuousDiscreteProcess*);
    
    virtual ~XWrapperContinuousDiscreteProcess();
    
public:
    void RegisterProcess(G4VContinuousDiscreteProcess*);
    void RegisterProcess(G4VContinuousDiscreteProcess*,G4int);
    
    G4double GetDensity(const G4Track&);
    G4double GetDensityPreviousStep(const G4Track&);
    
    void SetNucleiOrElectronFlag(G4int);
    G4int GetNucleiOrElectronFlag();
    
private:
    // hide assignment operator as private
    XWrapperContinuousDiscreteProcess(XWrapperContinuousDiscreteProcess&);
    XWrapperContinuousDiscreteProcess& operator=(const XWrapperContinuousDiscreteProcess& right);
    
    //private data members
    G4int bNucleiOrElectronFlag; //Decide whether to use nuclei (+1) or electron (-1) or both (0) density to change parameters
    G4VContinuousDiscreteProcess* fRegisteredProcess;
    
    const G4Step theStepCopy;
    
    /////////////////////////////////////////////////////////
    /////////////////// GEANT4 PROCESS METHODS //////////////
    /////////////////////////////////////////////////////////
public:
    // DO IT
    virtual G4VParticleChange* PostStepDoIt(const G4Track&, const G4Step& );
    virtual G4VParticleChange* AlongStepDoIt(const G4Track&, const G4Step& );

    // GPIL
    virtual G4double PostStepGetPhysicalInteractionLength (const G4Track&, G4double, G4ForceCondition*);
    virtual G4double AlongStepGetPhysicalInteractionLength (const G4Track&,G4double, G4double, G4double&, G4GPILSelection*);

    // GENERAL
    void StartTracking(G4Track* aTrack);
    virtual G4bool IsApplicable(const G4ParticleDefinition&);
    
    // PHYSICS TABLE
    virtual void BuildPhysicsTable(const G4ParticleDefinition&);
    virtual void PreparePhysicsTable(const G4ParticleDefinition&);
    virtual G4bool StorePhysicsTable(const G4ParticleDefinition* ,const G4String&, G4bool);
    virtual G4bool RetrievePhysicsTable( const G4ParticleDefinition* ,const G4String&, G4bool);
    
protected:
    // MFP
    virtual G4double GetMeanFreePath(const G4Track&, G4double, G4ForceCondition* );

protected:
    virtual G4double GetContinuousStepLimit(const G4Track& ,G4double  ,G4double ,G4double& );
    

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    
};

#endif