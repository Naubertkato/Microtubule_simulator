
#include "simulation.h"

#define REGISTER_SPECIES(n,m) species_factory_.register_class<n>(#m);

void Simulation::Run(system_parameters params) {
  params_ = params;
  run_name_ = params.run_name;
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
    Integrate();
    Statistics();
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
      output_mgr_.ReadPosits(); 
    }
    Draw();
  }
}

void Simulation::Integrate() {
  for (auto it=species_.begin(); it!=species_.end(); ++it)
    (*it)->UpdatePositions();
}

void Simulation::Interact() {
  iengine_.Interact();
}

void Simulation::ZeroForces() {
  for (auto it=species_.begin(); it != species_.end(); ++it) {
    (*it)->ZeroForces();
  }
}

void Simulation::Statistics() {
  if (params_.constant_pressure && i_step_ > 0) {
    if (i_step_ % params_.virial_time_avg == 0) {
      iengine_.CalculatePressure();
      space_.ConstantPressure();
    }
  }
  else if (params_.constant_volume)
    space_.ConstantVolume();
  if (space_.GetUpdate()) {
    space_.UpdateSpace();
    ScaleSpeciesPositions();
  }
}

void Simulation::ScaleSpeciesPositions() {
  for (auto spec : species_)
    spec->ScalePositions();
}

void Simulation::InitSimulation() {
  space_.Init(&params_);
  InitSpecies();
  output_mgr_.Init(&params_, &species_, &i_step_, run_name_);
  iengine_.Init(&params_, &species_, space_.GetStruct());
  if (params_.graph_flag) {
    GetGraphicsStructure();
    double background_color = (params_.graph_background == 0 ? 0.1 : 1);
    #ifndef NOGRAPH
    graphics_.Init(&graph_array, space_.GetStruct(), background_color);
    graphics_.DrawLoop();
    #endif
    params_.movie_directory.append("/");
    params_.movie_directory.append(params_.run_name);
  }
  InitOutputs();
}

void Simulation::InitSpecies() {

  // We have to search for the various types of species that we have
  REGISTER_SPECIES(MDBeadSpecies,md_bead);
  REGISTER_SPECIES(HardRodSpecies,hard_rod);
  REGISTER_SPECIES(FilamentSpecies,filament);
  REGISTER_SPECIES(BrBeadSpecies,br_bead);

  /* Search the species_factory_ for any registered species,
   and find them in the yaml file */
  for (auto registered = species_factory_.m_classes.begin(); 
      registered != species_factory_.m_classes.end(); ++registered) {
    SpeciesBase *spec = (SpeciesBase*)species_factory_.construct(registered->first);
    spec->InitConfig(&params_, space_.GetStruct(), gsl_rng_get(rng_.r));
    spec->Configurator();
    species_.push_back(spec);
  }
}

void Simulation::ClearSpecies() {
  for (auto it=species_.begin(); it!=species_.end(); ++it)
    delete (*it);
}

void Simulation::ClearSimulation() {
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
    GetGraphicsStructure();
    graphics_.Draw();
    if (params_.movie_flag) {
      // Record bmp image of frame 
      grabber(graphics_.windx_, graphics_.windy_,
              params_.movie_directory, (int) i_step_/params_.n_graph);
    }
  }
  #endif
}

void Simulation::GetGraphicsStructure() {
  graph_array.clear();
  for (auto it=species_.begin(); it!=species_.end(); ++it)
    (*it)->Draw(&graph_array);
}

void Simulation::InitOutputs() {
  if (params_.time_flag) {
    cpu_init_time_ = cpu();
  }
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
  }
}

//TODO Make sure only species that are put through with m posit are initialized
void Simulation::CreateMovie(system_parameters params, std::string name, std::vector<std::string> posit_files){
  params_ = params;
  run_name_ = name;
  //Graph and don't make new posit files
  params_.graph_flag = true;
  params_.posit_flag = false;
  output_mgr_.SetPosits(posit_files);
  rng_.init(params_.seed);
  InitSimulation();
  RunMovie();
  ClearSimulation();
}

//void Simulation::Analysis(system_parameters params, std::string name, std::vector<std::string> posit_files) {
  //params_ = params;
  //run_name_ = name;
  //params_.graph_flag = 0;
  //params_

