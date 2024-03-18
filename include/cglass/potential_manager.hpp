#ifndef _CGLASS_POTENTIAL_MANAGER_H_
#define _CGLASS_POTENTIAL_MANAGER_H_

#include "max_force_potential.hpp"
#include "no_potential.hpp"
#include "lennard_jones_potential.hpp"
#include "r2_potential.hpp"
#include "soft_potential.hpp"
#include "soft_shoulder_potential.hpp"
#include "wca_potential.hpp"
#include "temperature_potential.hpp"

class PotentialManager {
 private:
  // Potentials
  NoPotential none_;
  WCAPotential wca_;
  LennardJonesPotential lj_;
  SoftPotential soft_;
  MaxForcePotential max_;
  TemeraturePotential temp_;
  // R2Potential r2pot_;
  // SoftShoulderPotential sspot_;
  PotentialBase *pot_;
  potential_type pot_type_;

 public:
  void InitPotentials(system_parameters *params) {
    /*
     * Check to see what kind of potential we are using,
     * and set that potential to be our default
     */
    pot_type_ = potential_type::_from_string(params->potential.c_str());
    switch (pot_type_) {
      case potential_type::none: {
        pot_ = &none_;
        break;
      }
      case potential_type::soft: {
        pot_ = &soft_;
        break;
      }
      case potential_type::lj: {
        pot_ = &lj_;
        break;
      }
      case potential_type::wca: {
        pot_ = &wca_;
        /*
         * Since WCA can result in infinite forces,
         * we initialize the max force potential in
         * case we have an overlap of objects
         */
        max_.Init(params);
        break;
      }
    case potential_type::temperature: {
      pot_ = &temp_;
      break;
    }
      default:
        Logger::Error("Potential type not set up in potential_manager.hpp");
    }
    pot_->Init(params);
  }
  void CalcPotential(Interaction &ix) {
    /*
     *  If dr_mag2 is very very small (or zero!) we
     *  are overlapping. This can be very dangerous
     *  for infinite potentials, so enforce a cutoff.
     */
    //if (ix.dr_mag2 < 1e-12 && pot_type_ == +potential_type::wca) {
      //max_.CalcPotential(ix);
      //return;
    //}
    pot_->CalcPotential(ix);
  }
  double GetRCut2() { return pot_->GetRCut2(); }
  bool CheckMaxForce() {
    if (pot_->CheckMaxForceViolation()) {
      pot_->ResetMaxForceViolation();
      return true;
    }
    return false;
  }
};

#endif
