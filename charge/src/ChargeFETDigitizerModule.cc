#include "ChargeFETDigitizerModule.hh"
#include "ChargeFETDigitizerMessenger.hh"
#include "G4CMPElectrodeHit.hh"
#include "G4CMPMeshElectricField.hh"
#include "G4SystemOfUnits.hh"
#include "G4VDigitizerModule.hh"
#include "G4String.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include <sstream>

ChargeFETDigitizerModule::ChargeFETDigitizerModule(G4String modName) :
  G4VDigitizerModule(modName), outputFilename("FETOutput"),
  configFilename("config/G4CMP/FETSim/ConstantsFET"),
  templateFilename("config/G4CMP/FETSim/FETTemplates"),
  ramoFileDir("."), decayTime(40e-6*s), dt(800e-9*s), preTrig(4096e-7*s),
  templateEnergy(100e-3*eV), numChannels(4), timeBins(4096), runLive(false),
  messenger(new ChargeFETDigitizerMessenger(this))
{}

ChargeFETDigitizerModule::ChargeFETDigitizerModule() :
  G4VDigitizerModule("NoSim"), outputFilename("FETOutput"),
  configFilename("config/G4CMP/FETSim/ConstantsFET"),
  templateFilename("config/G4CMP/FETSim/FETTemplates"),
  ramoFileDir("."), decayTime(40e-6*s), dt(800e-9*s), preTrig(4096e-7*s),
  templateEnergy(100e-3*eV), numChannels(4), timeBins(4096), runLive(false),
  messenger(nullptr)
{}

ChargeFETDigitizerModule::~ChargeFETDigitizerModule()
{
  delete messenger;
  outputFile.close();
}

void ChargeFETDigitizerModule::Initialize()
{
  ReadFETConstantsFile();
  BuildFETTemplates();
  BuildRamoFields();
  if (outputFile.is_open())
    outputFile.close();
  outputFile.open(outputFilename, std::ios_base::app);
  outputFile << "Run ID,Event ID,Channel,Pulse (4096 bins)"
         << G4endl;
}

void ChargeFETDigitizerModule::Digitize()
{
  if (!runLive) return;
  G4HCofThisEvent* HCE = G4RunManager::GetRunManager()->GetCurrentEvent()->GetHCofThisEvent();
  G4SDManager* fSDM = G4SDManager::GetSDMpointer();
  G4int HCID = fSDM->GetCollectionID("G4CMPElectrodeHit");
  G4CMPElectrodeHitsCollection* hitCol =
        static_cast<G4CMPElectrodeHitsCollection*>(HCE->GetHC(HCID));
  vector<G4CMPElectrodeHit*>* hitVec = hitCol->GetVector();

  vector<G4double> scaleFactors(numChannels,0);
  G4double position[4] = {0.,0.,0.,0.};
  G4ThreeVector vecPosition;
  for(size_t chan = 0; chan < numChannels; ++chan) {
    for(size_t hitIdx=0; hitIdx < hitVec->size(); ++hitIdx) {
      if(hitVec->at(hitIdx)->GetParticleName()=="G4CMPDriftElectron" ||
         hitVec->at(hitIdx)->GetParticleName()=="G4CMPDriftHole") {
        vecPosition = hitVec->at(hitIdx)->GetFinalPosition();
        position[0] = vecPosition.getX();
        position[1] = vecPosition.getY();
        position[2] = vecPosition.getZ();
        scaleFactors[chan] -= hitVec->at(hitIdx)->GetCharge()
                                    * RamoFields[chan].GetPotential(position);
      }
    }
  }

  vector<vector<G4double> > FETTraces(CalculateTraces(scaleFactors));
  G4int runID = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
  WriteFETTraces(FETTraces, runID, eventID);
}

void ChargeFETDigitizerModule::PostProcess(const G4String& fileName)
{
  std::ifstream input(fileName);
  if (!input.good()) {
    G4ExceptionDescription msg;
    msg << "Error reading data input file from " << fileName;
    G4Exception("ChargeFETDigitizerModule::PostProcess", "Charge002",
    FatalException, msg);
  }
  G4double throw_away;
  G4double position[4] = {0.,0.,0.,0.};
  G4double charge;
  G4int RunID, EventID;
  vector<G4double> scaleFactors(numChannels,0);

  G4String line;
  G4String entry;
  std::getline(input, line); //Grab column headers first
  while (!std::getline(input, line).eof()) {
    std::istringstream ssLine(line);

    std::getline(ssLine,entry,',');
    std::istringstream(entry) >> RunID;

    std::getline(ssLine,entry,',');
    std::istringstream(entry) >> EventID;

    std::getline(ssLine,entry,',');
    std::istringstream(entry) >> throw_away;

    std::getline(ssLine,entry,',');
    std::istringstream(entry) >> charge;

    for (G4int i=0; i<6; ++i) {
      std::getline(ssLine,entry,',');
      std::istringstream(entry) >> throw_away;
    }

    for (G4int i=0; i<3; ++i) {
      std::getline(ssLine,entry,',');
      std::istringstream(entry) >> position[i];
      position[i] *= m;
    }

    for(G4int chan = 0; chan < numChannels; ++chan)
      scaleFactors[chan] -= charge*RamoFields[chan].GetPotential(position);
  }

  vector<vector<G4double> > FETTraces(CalculateTraces(scaleFactors));
  WriteFETTraces(FETTraces, RunID, EventID);
}

vector<vector<G4double> > ChargeFETDigitizerModule::CalculateTraces(
                                    const vector<G4double>& scaleFactors)
{
  vector<vector<G4double> > FETTraces(numChannels,vector<G4double>(timeBins,0.));
  for(size_t chan=0; chan < numChannels; ++chan)
    for(size_t cross=0; cross < numChannels; ++cross)
      for(size_t bin=0; bin < timeBins; ++bin)
        FETTraces[chan][bin] += scaleFactors[cross]*FETTemplates[chan][cross][bin];
  return FETTraces;
}

void ChargeFETDigitizerModule::ReadFETConstantsFile()
{
  constantsFile.open(configFilename);
  if (!constantsFile.good()) {
    G4ExceptionDescription msg;
    msg << "Error reading FET constants file from " << configFilename;
    G4Exception("ChargeFETDigitizerModule::ReadFETConstantsFile", "Charge001",
    FatalException, msg);
  }
  G4String buffer;
  G4String varName;
  G4String varVal;
  G4bool comment = false;
  size_t pos;

  while(!constantsFile.eof()) {
    getline(constantsFile, buffer);
    if((!comment) && (buffer.find("//") != std::string::npos)) {
      if(buffer.find("//") == 0)
        continue;
      else
        buffer = buffer.substr(0, buffer.find("//"));
    } else if((!comment) && (buffer.find("/*") != std::string::npos)) {
      buffer.erase(buffer.find("/*"), buffer.find("*/") + 2);
      if(buffer.find("*/") == std::string::npos)
        comment = true;
    } else if(buffer.find("*/") != std::string::npos && comment) {
      comment = false;
      buffer.erase(buffer.find("*/"), 2);
    }

    if(!comment && !buffer.empty()) {
      while(buffer.find(' ') != std::string::npos) //Erase all spaces
        buffer.erase(buffer.find(' '), 1);

      pos = buffer.find('='); //To split buffer into lhs and rhs
      if(pos == std::string::npos)
        continue;
      else {
        varName = buffer.substr(0, pos++);
        varVal = buffer.substr(pos);

        if(*(--varVal.end()) == ';') //Strip ; from end
          varVal.erase(--varVal.end());

        if(varName == "numChannels")
          numChannels = atoi(varVal);
        else if(varName == "timeBins")
          timeBins = atoi(varVal);
        else if(varName == "decayTime")
          decayTime = atof(varVal)*s;
        else if(varName == "dt")
          dt = atof(varVal)*s;
        else if(varName == "preTrig")
          preTrig = atof(varVal)*s;
        else if(varName == "templateEnergy")
          templateEnergy = atof(varVal)*eV;
        else if(varName == "templateFilename")
          templateFilename = varVal.substr(1,varVal.length()-2); //strip quotes
        else if(varName == "ramoFileDir") {
          varVal = varVal.substr(1,varVal.length()-2); //strip quotes
          if (*(--varVal.end()) == '/') varVal.erase(--varVal.end()); //strip trailing slash
          ramoFileDir = varVal;
        }
        else
          G4cout << "FETSim Warning: Variable " << varName
                 << " is unused." << G4endl;
      }
    }
  }
  constantsFile.close();
}

void ChargeFETDigitizerModule::BuildFETTemplates()
{
  FETTemplates = vector<vector<vector<G4double> > >
    (numChannels,vector<vector<G4double> >
    (numChannels,vector<G4double>(timeBins,0) ) );
  std::fstream templateFile(templateFilename.c_str());
  if(!templateFile.fail()) {
    for(G4int i=0; i<numChannels; ++i)
      for(G4int j=0; j<numChannels; ++j)
        for(G4int k=0; k<timeBins; ++k)
          templateFile >> FETTemplates[i][j][k];
  } else {
    G4cout << "ChargeFETDigitizerModule::BuildFETTemplate(): WARNING: Reading "
           << "from template file failed. Using default pulse templates."
           << G4endl;
    for(G4int i=0; i<numChannels; ++i) {
      G4int ndt = (G4int)(preTrig/dt);
      for(G4int j=0; j<ndt; ++j)
        FETTemplates[i][i][j] = 0;

      for(G4int k=1; k<timeBins-ndt+1; ++k)
        FETTemplates[i][i][k+ndt-1] = exp(-k*dt/decayTime);
    }
  }
  templateFile.close();
}

void ChargeFETDigitizerModule::BuildRamoFields()
{
  for(G4int i=0; i < numChannels; ++i) {
    std::stringstream name;
    name << ramoFileDir << "/EpotRamoChan" << i+1;
    std::ifstream ramoFile(name.str().c_str());
    if(ramoFile.good()) {
      ramoFile.close();
      RamoFields.emplace_back(G4CMPMeshElectricField(name.str()));
    } else {
      ramoFile.close();
      G4cerr << "ChargeFETDigitizerModule::BuildRamoFields(): ERROR: Could"
        << " not open Ramo files for each FET channel." << G4endl;
    }
  }
}

void ChargeFETDigitizerModule::WriteFETTraces(
  const vector<vector<G4double> >& traces, G4int RunID, G4int EventID)
{
  for(G4int chan = 0; chan < numChannels; ++chan) {
    outputFile << RunID << "," << EventID << "," << chan+1 << ",";
    for(G4int bin = 0; bin < timeBins-1; ++bin) {
      outputFile << traces[chan][bin] << ",";
    }
    outputFile << traces[chan][timeBins-1] << "\n";
  }
}
