#include "cglass/crosslink_manager.hpp"

void CrosslinkManager::Init(system_parameters *params, SpaceBase *space, 
                            Tracker *tracker, std::vector<Object *> *objs, unsigned long seed) {
  objs_ = objs;
  update_ = false;
  obj_size_ = 0.0;
  params_ = params;
  space_ = space;
  tracker_ = tracker;
  rng_ = new RNG(seed);
}

void CrosslinkManager::InitSpecies(sid_label &slab, ParamsParser &parser,
                                   unsigned long seed) {
  if (xlink_species_.size() == 0) {
    xlink_species_.reserve(parser.GetNCrosslinkSpecies());
  }
  xlink_species_.push_back(new CrosslinkSpecies(seed));
  xlink_species_.back()->Init(slab.second, parser);
  Logger::Warning("Crosslinker species %s was added", (xlink_species_.back()->GetSpeciesName()).c_str());
  species_map_[(xlink_species_.back()->GetSpeciesName()).c_str()] = xlink_species_.back();
  if (xlink_species_.back()->GetNInsert() <= 0) {
    delete xlink_species_.back();
    xlink_species_.pop_back();
  } else {
    xlink_species_.back()->InitInteractionEnvironment(objs_, &obj_size_, tracker_,
                                                      &update_, &bound_curr_);
    rcutoff_ = xlink_species_.back()->GetRCutoff();
  }
}

/* Keep track of volume of objects in the system. Affects the
 * probability of a free crosslink binding to an object. */
void CrosslinkManager::UpdateObjsSize() {
  obj_size_ = 0;
  for (auto it = objs_->begin(); it != objs_->end(); ++it) {
    switch ((*it)->GetShape()) {
      case shape::rod:
        obj_size_ += (*it)->GetLength();
        break;
      case shape::sphere:
        if ((*it)->GetNAnchored() == 0) obj_size_ += (*it)->GetArea();
        break;
      default:
        break;
    }
  }
}

/* Whether to reinsert anchors into the interactors list */
bool CrosslinkManager::CheckUpdate() {
  if (update_) {
    update_ = false;
    return true;
  }
  return false;
}

/* Return singly-bound anchors, for finding neighbors to bind to */
void CrosslinkManager::GetInteractors(std::vector<Object *> &ixors) {
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->GetInteractors(ixors);
  }
}

/* Returns all anchors, not just singly-bound anchors. Used for reassigning
   bound anchors to bonds upon a checkpoint reload */
void CrosslinkManager::GetAnchorInteractors(std::vector<Object *> &ixors) {
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->GetAnchorInteractors(ixors);
  }
}

void CrosslinkManager::UpdateCrosslinks() {
  update_ = false;
  UpdateObjsSize();
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->UpdatePositions();
    (*it)->UpdateBindRate();
  }
  if (!bound_curr_.empty()){
    Knockout();
  }
  //Use global check for cross so we don't need to check individual crosslinkers every time step
  if (global_check_for_cross == true) {
    CheckForCross();
    global_check_for_cross = false;
  }
}

//Check if any crosslinkers are crossing
void CrosslinkManager::CheckForCross() {
  Logger::Trace("Checking if crosslinkers are crossing");
  //Sorry this is a mess to look at, this is going over over crosslink in every species 
  //and comparing it to every other crosslink in every species to see if the crosslinks
  //are crossing
  for (auto spec_one = xlink_species_.begin(); spec_one != xlink_species_.end(); ++spec_one) {
  int member_num_ = (*spec_one) -> GetNMembers(); 
    for (int i = 0; i <= (member_num_-1); i++) {
      Crosslink* link_one = (*spec_one) -> GetCrosslink(i);
      // Check is we need to check for crossing for this crosslinker 
      if (link_one -> IsDoubly() && link_one -> ReturnCheckForCross() == true) {
        for (auto spec_two = xlink_species_.begin(); spec_two != xlink_species_.end(); ++spec_two) {
          int member_num_two_ = (*spec_two) -> GetNMembers();  
          for (int j = 0; j <= (member_num_two_-1); j++) {
            Crosslink* link_two = (*spec_two) -> GetCrosslink(j);
            //If link two is double bound and the crosslinkers aren't the same crosslinker
            if(link_two -> IsDoubly() && link_one -> GetOID() != link_two -> GetOID()) {
              //Get how far the crosslinker anchors are along the filament
              std::vector<double> link_one_s = link_one -> GetAnchorS();
              std::vector<double> link_two_s = link_two -> GetAnchorS();
              //Get the IDs of the PCs crosslink one is attatched to
              std::vector<int> rec_ids_one = link_one -> GetReceptorPCIDs();
              //Get the IDs of the PCs crosslink two is attatched to
              std::vector<int> rec_ids_two = link_two -> GetReceptorPCIDs();
              //If same index doesn't refer to sites on the same filament switch indexs so 
              //so the same index refers to anchors on the same microtubule
              if (rec_ids_one[0] != rec_ids_two[0]) {
                std::swap(link_two_s[0], link_two_s[1]);
                std::swap(rec_ids_two[0], rec_ids_two[1]);
              } 
              //If the sites are still on different filaments then crosslinkers aren't binding the same microtubules
              if (rec_ids_one[0] == rec_ids_two[0]) {
                same_microtubules = true;
              }
              else {
                same_microtubules = false;
              }
              //If crosslinkers are crossing and connected same microtubules, then unbind most rect crosslinker
              if (same_microtubules && ((link_one_s[0] > link_two_s[0] && link_one_s[1] < link_two_s[1]) 
                     || (link_one_s[0] < link_two_s[0] && link_one_s[1] > link_two_s[1]))) { 
                link_one -> UnbindCrossing();
                break;
              } else {
                link_one -> SetCheckForCross();
              } 
            }
          }
        }    
      }
    }
  }
}

// Loop over all spheres bound in the last dt. If multiple xlinks want to bind
// to a site, roll based on their relative probabilities, and unbind all of the
// "losers".
void CrosslinkManager::Knockout() {
  Logger::Info("Printing bound_curr, size is %i", bound_curr_.size());
  for (auto& kv : bound_curr_) {
    Sphere* receptor = kv.first;
    std::pair<std::vector<double>, std::vector<std::pair<Anchor*, std::string> > >&  receptor_info = kv.second;
    Logger::Info("Receptor id is, %d", receptor->GetOID());
    int events = receptor_info.first.size();
    while (events>0) { 
      events-=1; 
      Logger::Warning("Events is %i, lengths are, %i, %i, %i", events, receptor_info.first.size(), receptor_info.second.size(), receptor_info.second.size());
      //Logger::Warning("Prpp is %i", )receptor_info.first[events]
      Logger::Warning("bind type is %s", receptor_info.second[events].second.c_str());
      if (receptor_info.second[events].second == "forward step" || receptor_info.second[events].second == "back step" || receptor_info.second[events].second == "single to double") {
        Logger::Info("Inside if");
        printf("Address of x is %p\n", (void *)receptor_info.second[events].first);
        Logger::Info("Prop is %f, Anchor is %i, Bind type is %s", receptor_info.first[events], receptor_info.second[events].first->GetOID(), receptor_info.second[events].second.c_str());
      }
    
    }
    Logger::Info("After if"); 
  } 
  //Logger::Warning("knocjout seed in knockout is %f", rng_->RandomUniform());
  //Logger::Warning("Begining knockout bind, %lu", bound_curr_.size());
  for (auto& kv : bound_curr_) {
    Sphere* receptor = kv.first;
    std::pair<std::vector<double>, std::vector<std::pair<Anchor*, std::string> > >&  receptor_info = kv.second;
    // If one anchor wants to bind to a receptor bind it
    
    if (receptor_info.first.size() == 1) {
      KnockoutBind(receptor, 0);
      }
    
    //If two or three anchors want to bind to a receptor choose which one binds 
    else if (receptor_info.first.size() == 2 || receptor_info.first.size() == 3) {
      Logger::Warning("%i anchors wanted to bind to receptor, if this is common reduce time step", (receptor_info.first.size()));
      double sum_of_probs = std::accumulate(receptor_info.first.begin(), receptor_info.first.end(), 0.0);
      Logger::Warning("probability of anchor binding to site is %f. If this warning is common, or if probability is much greater than 1 reduce time step.", sum_of_probs);
      double roll = sum_of_probs*rng_->RandomUniform();
      int count = 0;
      std::vector<double> prob_list = receptor_info.first;
      
      for (auto prob = prob_list.begin(); prob != prob_list.end(); ++prob) {
        if (*prob>roll) {
          KnockoutBind(receptor, count);
          Logger::Warning("chose %s, from roll, %f", receptor_info.second[count].second.c_str(), roll); 
        }
        else {
          roll -= *prob;
          count+= 1;
        }
      } 
       
          
    }
    // If more than 3 anchors want to bind to a receptor through error 
    else if (receptor_info.first.size()>3) {
      Logger::Error("More than 3 anchors trying to bind to receptor, time step too large");
    }
    for (auto it = receptor_info.second.begin(); it != receptor_info.second.end(); ++it){
      if (it->second == "forward step" || it->second == "back step") {
        it->first->ResetChangedThisStep();
      }
    }
  }  
  AddKnockoutCrosslinks();
  bound_curr_.clear();
  //Logger::Warning("");
}

void CrosslinkManager::KnockoutBind(Sphere* receptor, int winner) {
  Logger::Warning("Bind type is %s", bound_curr_[receptor].second[winner].second.c_str());
  if (bound_curr_[receptor].second[winner].second == "forward step") {
    Logger::Warning("steped forward");
    Logger::Info("forward step %i", bound_curr_[receptor].second[winner].first->GetOID());
    bound_curr_[receptor].second[winner].first->StepForward();
    //Logger::Warning("anchor %i wanted to bind to receptor %i, if this is common reduce time step", bound_curr_[receptor].second[0].first->GetOID(), receptor->GetOID()); 
 
  }

  else if (bound_curr_[receptor].second[winner].second == "back step") {
    Logger::Info("back step %i", bound_curr_[receptor].second[winner].first->GetOID());
    bound_curr_[receptor].second[winner].first->StepBack();
    Logger::Warning("stepped back");
    Logger::Warning("anchor %i wanted to backstep to receptor %i, if this is common reduce time step", bound_curr_[receptor].second[0].first->GetOID(), receptor->GetOID()); 
 
  }

  else if (bound_curr_[receptor].second[winner].second == "single to double") {
    Logger::Info("single to double %i", bound_curr_[receptor].second[winner].first->GetOID());
    bound_curr_[receptor].second[winner].first->AttachObjCenter(receptor);
    Object* ob_pointer = bound_curr_[receptor].second[winner].first->GetCrosslinkPointer();
    //Logger::Warning("Crosslink species is %d", ob_pointer->GetSID()); 
    //if (ob_pointer == nullptr) {
    //  Logger::Warning("Object pointer also null");
    //} 
    Crosslink* cl_pointer = dynamic_cast<Crosslink*>(ob_pointer);
    if (cl_pointer == nullptr) {
    //Logger::Warning("pointer is null");
    }
    cl_pointer->SetDoubly();
    //Logger::Warning("anchor %i, from crosslink %d wanted to bind to receptor, if this is common reduce time step", bound_curr_[receptor].second[0].first->GetOID(), cl_pointer->GetOID()); 
 
  }
  //bind from soultion 
  else {
  //for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    CrosslinkSpecies* cl_species_ = species_map_[bound_curr_[receptor].second[winner].second.c_str()];
    //std::cout << "Bindingcrosslink from species :\n " << cl_species_->GetSpeciesName();
    //Logger::Warning("Binding from solution, %i, %s", xlink_species_.size(), (cl_species_->GetSpeciesName()).c_str());
    //Logger::Warning("anchor %i wanted to bind to receptor, if this is common reduce time step", bound_curr_[receptor].second[0].first->GetOID()); 
 
   std::pair<CrosslinkSpecies*, Sphere*> cl_rec_pair_;
   cl_rec_pair_.first = cl_species_;
   cl_rec_pair_.second = receptor;     
   //}
    //cl_species_->KnockoutBind(receptor);
    cl_to_add_this_step_.push_back(cl_rec_pair_);
    Logger::Info("Added Crosslink to knockout");
  }

}

void CrosslinkManager::AddKnockoutCrosslinks() {
  
  for (auto it = cl_to_add_this_step_.begin(); it != cl_to_add_this_step_.end(); ++it) {
     Logger::Info("Added Crosslink during knockout");
    (it->first)->KnockoutBind(it->second);
  }
  cl_to_add_this_step_.clear();
}

void CrosslinkManager::InsertCrosslinks() {
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->InsertCrosslinks();
    (*it)->SetGlobalCheckForCrossPointer(&global_check_for_cross);
  } 
}

void CrosslinkManager::InsertAttachedCrosslinks(std::vector<std::vector<Object *>> receptor_list) {
  // Need to do this for GetRandomObject to work with spheres
  UpdateObjsSize();
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->UpdateBindRate();
    (*it)->InsertAttachedCrosslinksSpecies(receptor_list);
  }
}

void CrosslinkManager::Clear() {
  output_mgr_.Close();
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->CleanUp();
    delete (*it);
  }
  xlink_species_.clear();
}

void CrosslinkManager::Draw(std::vector<graph_struct *> &graph_array) {
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->Draw(graph_array);
  }
}

void CrosslinkManager::AddNeighborToAnchor(Object *anchor, Object *neighbor) {
  if (anchor->GetSID() != +species_id::crosslink) {
    Logger::Error(
        "AddNeighborToAnchor expected crosslink object, got generic object.");
  }
  Anchor *a = dynamic_cast<Anchor *>(anchor);
  a->AddNeighbor(neighbor);
}

void CrosslinkManager::InitOutputs(bool reading_inputs, run_options *run_opts) {
  output_mgr_.Init(params_, &xlink_species_, space_, reading_inputs, run_opts);
}

void CrosslinkManager::WriteOutputs() { output_mgr_.WriteOutputs(); }

void CrosslinkManager::ZeroDrTot() {
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->ZeroDrTot();
  }
}

const int CrosslinkManager::GetDoublyBoundCrosslinkNumber() const {
  int num = 0;
  for (const auto &xl_spec : xlink_species_) {
    num += xl_spec->GetDoublyBoundCrosslinkNumber();
  }
  return num;
}

const double CrosslinkManager::GetDrMax() {
  double dr_max = 0;
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    double dr = (*it)->GetDrMax();
    if (dr > dr_max) {
      dr_max = dr;
    }
  }
  return dr_max;
}

void CrosslinkManager::LoadCrosslinksFromCheckpoints(
    std::string run_name, std::string checkpoint_run_name) {
  for (auto it = xlink_species_.begin(); it != xlink_species_.end(); ++it) {
    (*it)->LoadFromCheckpoints(run_name, checkpoint_run_name);
  }
}

void CrosslinkManager::ReadInputs() { output_mgr_.ReadInputs(); }
void CrosslinkManager::Convert() { output_mgr_.Convert(); }
