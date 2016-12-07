
#include "simulation.h"

#define REGISTER_SPECIES(n,m) species_factory_.register_class<n>(#m);

Simulation::Simulation() {}
Simulation::~Simulation() {}

void Simulation::Run(system_parameters params, std::string name) {
  params_ = params;
  run_name_ = name;
  rng_.init(params_.seed);
  InitSimulation();
  RunSimulation();
  ClearSimulation();
}

void Simulation::RunSimulation() {
  std::cout << "Running simulation: " << run_name_ << std::endl;
  std::cout << "   steps: " << params_.n_steps << std::endl;
  for (i_step_ = 0; i_step_<params_.n_steps; ++i_step_) {
    time_ = (i_step_+1) * params_.delta; 
    PrintComplete();
    ZeroForces();
    Interact();
    //KineticMonteCarlo();
    Integrate();
    GenerateStatistics(i_step_);
    if (debug_trace)
      DumpAll(i_step_);
    Draw();
    WriteOutputs();
  }
}

void Simulation::PrintComplete() {
  if ((100*i_step_) % (params_.n_steps) == 0) {
    printf("%d%% Complete\n", (int)(100 * (float)i_step_ / (float)params_.n_steps));
    fflush(stdout);
  }
  if (debug_trace)
    printf("********\nStep %d\n********\n", i_step_);
}

void Simulation::RunMovie(){
  std::cout << "Running movie: " << run_name_ << "\n";
  std::cout << "    steps: " << params_.n_steps << std::endl;
  for (i_step_=0; i_step_<params_.n_steps; ++i_step_) {
    time_ = (i_step_+1) * params_.delta; 
    PrintComplete();
    if (i_step_%params_.n_posit == 0){
      output_mgr_.ReadSpeciesPositions(); 
    }
    Draw();
  }
}

void Simulation::DumpAll(int i_step) {
  // Very yucky dump of all the particles and their positions and forces
  uengine_.DumpAll();
}

void Simulation::Integrate() {
  for (auto it=species_.begin(); it!=species_.end(); ++it)
    (*it)->UpdatePositions();
}

void Simulation::Interact() {
  std::vector<Simple*> objs;
  std::vector<interactionmindist> imds;
  for (auto spec_it : species_) {
    std::vector<Simple*> simples = spec_it->GetSimples();
    objs.insert(objs.end(),simples.begin(),simples.end());
  }
  int n_obj = (int) objs.size();
  for (int i=0; i<n_obj; ++i) {
    for (int j=i+1; j<n_obj; ++j) {
      if (objs[i]->GetRID() == objs[j]->GetRID())  continue;
      if (objs[i]->GetCID() == objs[j]->GetCID())  continue;
      if (objs[i]->GetOID() == objs[j]->GetOID()) error_exit("WHAT\n");
      interactionmindist imd;
      MinimumDistance(objs[i],objs[j],imd,params_.n_dim,params_.n_periodic,space_.GetStruct());
      if (imd.dr_mag2 > rcut_) {
      //if (sqrt(imd.dr_mag2)-imd.buffer_mag > rcut_) {
        //printf("drmag2, rcut2: %2.2f, %2.2f\n",imd.dr_mag2,rcut2_);
        continue;
      }
      //printf("INTERACTING\n");
      double force[3];
      double torque[3];
      WCA(sqrt(imd.dr_mag2), imd.dr, force);
      objs[i]->AddForce(force);
      cross_product(imd.contact1,force,torque,3);
      //printf("oid: %d, f: {%2.8f %2.8f}, t: {%2.8f}\n", objs[i]->GetOID(),force[0],force[1],torque[2]);
      for (int i=0;i<params_.n_dim; ++i)
        force[i] = -force[i];
      objs[j]->AddForce(force);
      cross_product(imd.contact2,force,torque,3);
      objs[j]->AddTorque(torque);
    }
  }
  //uengine_.Interact();
}

void Simulation::KineticMonteCarlo() {
  //uengine_.StepKMC();
}

void Simulation::ZeroForces() {
  for (auto it=species_.begin(); it != species_.end(); ++it) {
    (*it)->ZeroForces();
  }
}

void Simulation::GenerateStatistics(int istep) {
  uengine_.GenerateStatistics(istep);
}

void Simulation::InitSimulation() {
  space_.Init(&params_, gsl_rng_get(rng_.r));
  output_mgr_.Init(&params_, &graph_array, &i_step_, run_name_);
  InitSpecies();
  WCAInit();
  //uengine_.Init(&params_, space_.GetStruct(), &species_, &anchors_, gsl_rng_get(rng_.r));
  if (params_.graph_flag) {
    //When making a movie graphics are handled by output_mgr_
    if ( output_mgr_.IsMovie() ) output_mgr_.GetGraphicsStructure();
    else GetGraphicsStructure();
      
    double background_color = (params_.graph_background == 0 ? 0.1 : 1);
    #ifndef NOGRAPH
    graphics_.Init(&graph_array, space_.GetStruct(), background_color);
    graphics_.DrawLoop();
    #endif
  }
  InitOutputs();
}

//XXX
void Simulation::WCA(double dr_mag,double *dr,double *f) {
  std::fill(f, f + n_dim_ + 1, 0.0);
  double rmag = dr_mag;
  double ffac, r6, rinv, rinv2;
  rinv = 1.0/(rmag);
  rinv2 = rinv*rinv;
  r6 = rinv2*rinv2*rinv2;
  ffac = -(12.0*c12_*r6 - 6.0*c6_)*r6*rinv;
  // Cut off the force at fcut
  if (ABS(ffac) > fcut_) {
    ffac = SIGNOF(ffac) * fcut_;
  }
  for (int i = 0; i < n_dim_; ++i) 
    f[i] = ffac*dr[i]/rmag;
  //fpote[n_dim_] = r6*(c12_*r6 - c6_) + eps_;
}

//XXX
void Simulation::WCAInit() {
  eps_    = params_.wca_eps;
  sigma_  = params_.wca_sig;
  fcut_ = params_.f_cutoff;
  n_dim_ = params_.n_dim;
  // For WCA potentials, the rcutoff is actually important, as it must be
  // restricted to be at 2^(1/6)sigma
  rcut_ = pow(2.0, 1.0/6.0)*sigma_;
  rcut2_ = rcut_*rcut_;
  c12_ = 4.0 * eps_ * pow(sigma_, 12.0);
  c6_  = 4.0 * eps_ * pow(sigma_,  6.0);
}

void Simulation::InitSpecies() {
//#include "init_species.h"
  // Check out the configuration file
  std::cout << "********\n";
  std::cout << "Species Load ->\n";
  std::cout << "  file: " << params_.config_file << std::endl;

  YAML::Node node = YAML::LoadFile(params_.config_file);

  // We have to search for the various types of species that we have
  // Maybe hijack init_species.h for this
  REGISTER_SPECIES(MDBeadSpecies,md_bead);
  REGISTER_SPECIES(BrRodSpecies,br_rod);
  REGISTER_SPECIES(DyRodSpecies,dy_rod);
  //REGISTER_SPECIES(XlinkSpecies,xlink);
  REGISTER_SPECIES(FilamentSpecies,filament);
  //REGISTER_SPECIES(MDBeadOptSpecies,md_bead_opt);
  REGISTER_SPECIES(BrBeadSpecies,br_bead);
  //REGISTER_SPECIES(SpindlePoleBodySpecies,spb);

  std::string config_type = "separate";
  if (node["configuration_type"]) {
    config_type = node["configuration_type"].as<std::string>();
  }
  anchors_.clear();
  // Search the species_factory_ for any registered species, and find them in the
  // yaml file
  if (config_type.compare("separate") == 0) {
    for (auto possibles = species_factory_.m_classes.begin(); 
        possibles != species_factory_.m_classes.end(); ++possibles) {
      if (node[possibles->first]) {
        SpeciesBase *spec = (SpeciesBase*)species_factory_.construct(possibles->first);
        spec->InitConfig(&params_, space_.GetStruct(), &anchors_, gsl_rng_get(rng_.r));
        spec->Configurator();
        species_.push_back(spec);
      }
    }
    output_mgr_.AddSpecies(&species_);
  //} else if (config_type.compare("spindle") == 0) {
    //ConfigureSpindle();
  } else {
    std::cout << "Unknown configuration type " << config_type << std::endl;
    exit(1);
  }
}

//void Simulation::ConfigureSpindle() {
  //YAML::Node node = YAML::LoadFile(params_.config_file);
  //std::cout << "Configurator (Spindle) started using file " << params_.config_file << std::endl;
  //int nspbs = (int)node["spb"].size();
  //int nkcs = (int)node["kc"].size();
  //// Create the species that we know about for sure
  //SpindlePoleBodySpecies *pspbspec = (SpindlePoleBodySpecies*)species_factory_.construct("spb");
  //SpeciesBase *pspbspec_base = (SpeciesBase*)pspbspec;
  //// FIXME XXX change to microtubule species
  //BrRodSpecies *prspec = (BrRodSpecies*)species_factory_.construct("br_rod");
  //SpeciesBase *prspec_base = (SpeciesBase*)prspec;
  //assert(pspbspec != nullptr);
  //assert(prspec != nullptr);
  //pspbspec_base->InitConfig(&params_, space_.GetStruct(), &anchors_, gsl_rng_get(rng_.r));
  //prspec_base->InitConfig(&params_, space_.GetStruct(), &anchors_, gsl_rng_get(rng_.r));
  //int nmts = 0;
  //for (int i = 0; i < nspbs; ++i) {
    //nmts += (int)node["spb"][i]["mt"].size();
  //}
  //nmts += (int)node["mt"].size();
  //std::cout << "\nBasic Parameters:\n";
  //std::cout << "   n spbs: " << nspbs << std::endl;
  //std::cout << "   n mts : " << nmts << std::endl;
  //params_.n_rod = nmts;
  //// initialize the spbs
  //anchors_.clear();
  //for (int ispb = 0; ispb < nspbs; ++ispb) {
    //pspbspec->ConfiguratorSpindle(ispb, &anchors_);
    //std::vector<SpindlePoleBody*>* spindle_pole_bodies = pspbspec->GetMembers();
    //prspec->ConfiguratorSpindle(ispb, (*spindle_pole_bodies)[ispb]->GetOID(),
        //(*spindle_pole_bodies)[ispb]->GetPosition(),
        //(*spindle_pole_bodies)[ispb]->GetUAnchor(),
        //(*spindle_pole_bodies)[ispb]->GetVAnchor(),
        //(*spindle_pole_bodies)[ispb]->GetWAnchor(),
        //&anchors_);
  //}

  //// Now the crosslinks
  //// XXX FIXME hardcoded for now
  //XlinkSpecies *pxspec = (XlinkSpecies*)species_factory_.construct("xlink");
  //SpeciesBase *pxspec_base = (SpeciesBase*)pxspec;
  //pxspec_base->InitConfig(&params_, space_.GetStruct(), &anchors_, gsl_rng_get(rng_.r));
  //pxspec_base->Configurator();
  //species_.push_back(pspbspec_base);
  //species_.push_back(prspec_base);
  //species_.push_back(pxspec_base);
//}

void Simulation::ClearSpecies() {
  for (auto it=species_.begin(); it!=species_.end(); ++it)
    delete (*it);
}

void Simulation::ClearSimulation() {
  space_.Clear();
  output_mgr_.Close();
  ClearSpecies();
  #ifndef NOGRAPH
  if (params_.graph_flag)
    graphics_.Clear();
  #endif
}

void Simulation::Draw() {
  #ifndef NOGRAPH
  if (params_.graph_flag && i_step_%params_.n_graph==0) {
    if ( !output_mgr_.IsMovie() )
      GetGraphicsStructure();
    graphics_.Draw();
    if (params_.grab_flag) {
      // Record bmp image of frame 
      grabber(graphics_.windx_, graphics_.windy_,
              params_.grab_file, (int) i_step_/params_.n_graph);
    }
  }
  #endif
}

void Simulation::GetGraphicsStructure() {

  graph_array.clear();
  for (auto it=species_.begin(); it!=species_.end(); ++it)
    (*it)->Draw(&graph_array);
  //if (params_.draw_interactions)
    //uengine_.Draw(&graph_array);
}

void Simulation::InitOutputs() {
  if (params_.time_flag) {
    cpu_init_time_ = cpu();
  }
  if (params_.energy_analysis_flag) {
    std::ostringstream file_name;
    file_name << run_name_ << ".energy";
    std::ofstream en_file(file_name.str().c_str(), std::ios_base::out);
    en_file << "#kinetic  #potential  #total\n";
    en_file.close();
  }
  {
    uengine_.PrepOutputs();
  }
  output_mgr_.MakeHeaders();
}

void Simulation::WriteOutputs() {
  output_mgr_.WriteOutputs();
  if (i_step_ == 0) {
    return; // skip first step
  }
  if (params_.time_flag && i_step_ == params_.n_steps-1) {
    double cpu_time = cpu() - cpu_init_time_;
    std::cout << "CPU Time for Initialization: " <<  cpu_init_time_ << "\n";
    std::cout << "CPU Time: " << cpu_time << "\n";
    std::cout << "Sim Time: " << time_ << "\n";
    std::cout << "CPU Time/Sim Time: " << "\n" << cpu_time/time_ << std::endl;
    //double tot_en = 0;
    //for (auto it=species_.begin(); it!=species_.end(); ++it)
      //tot_en += (*it)->GetTotalEnergy();
    //std::cout << "Final system energy: " << tot_en << std::endl;
  }
  if (i_step_%1000==0 && params_.energy_analysis_flag) {
    std::ostringstream file_name;
    file_name << run_name_ << "-energy.log";
    std::ofstream en_file(file_name.str().c_str(), std::ios_base::out | std::ios_base::app);
    en_file.precision(16);
    en_file.setf(std::ios::fixed, std::ios::floatfield);
    double tot_en=0;
    double k_en=0;
    double p_en=0;
    for (auto it=species_.begin(); it!=species_.end(); ++it) {
      k_en += (*it)->GetKineticEnergy();
      p_en += (*it)->GetPotentialEnergy();
      tot_en += (*it)->GetTotalEnergy();
    }
    en_file << k_en << " " << p_en << " " << tot_en << "\n";
    en_file.close();
  }
  if (i_step_ == params_.n_steps-1) {
    for (auto it=species_.begin(); it!=species_.end(); ++it)
      (*it)->WriteOutputs(run_name_);
  }
  // XXX CJE FIXME write outputs more clearly
  if (i_step_%1000==0) {
    uengine_.WriteOutputs(i_step_);
  }
}

//TODO Make sure only species that are put through with m posit are initialized
void Simulation::CreateMovie(system_parameters params, std::string name, std::vector<std::string> posit_files){
  params_ = params;
  run_name_ = name;
  //Graph and don't make new posit files
  params_.graph_flag = 1;
  params_.posit_flag = 0;
  output_mgr_.SetMovie(posit_files);
  rng_.init(params_.seed);
  InitSimulation();
  RunMovie();
  ClearSimulation();
}

