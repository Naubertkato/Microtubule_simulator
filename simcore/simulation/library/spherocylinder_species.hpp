#ifndef _SIMCORE_SPHEROCYLINDER_SPECIES_H_
#define _SIMCORE_SPHEROCYLINDER_SPECIES_H_

#include "species.hpp"
#ifdef ENABLE_OPENMP
#include "omp.h"
#endif
#include "spherocylinder.hpp"

class SpherocylinderSpecies : public Species<Spherocylinder> {
 protected:
  bool midstep_;
  double **pos0_, **u0_, *msd_, *msd_err_, *vcf_, *vcf_err_;
  int time_, time_avg_interval_, n_samples_;
  std::fstream diff_file_;
  void InitDiffusionAnalysis();
  void DiffusionAnalysis();
  void CalculateMSD();
  void CalculateVCF();
  void UpdateInitPositions();
  void FinalizeDiffusionAnalysis();

 public:
  SpherocylinderSpecies() : Species() { SetSID(species_id::spherocylinder); }
  void Init(system_parameters *params, species_parameters *sparams, space_struct *space) {
    Species::Init(params, sparams, space);
    sparams_ = &(params_->spherocylinder);
    midstep_ = params_->spherocylinder.midstep;
  }
  void UpdatePositions() {
    for (auto it = members_.begin(); it != members_.end(); ++it) {
      it->UpdatePosition();
    }
  }
  virtual void InitAnalysis();
  virtual void RunAnalysis();
  virtual void FinalizeAnalysis();
};

#endif
