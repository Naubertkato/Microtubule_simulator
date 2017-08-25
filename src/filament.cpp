#include "filament.h"

Filament::Filament() : Mesh() {
  SetParameters();
  InitElements();
} 

void Filament::SetParameters() {
  color_ = params_->filament.color;
  draw_ = draw_type::_from_string(params_->filament.draw_type.c_str());
  length_ = params_->filament.length;
  persistence_length_ = params_->filament.persistence_length;
  diameter_ = params_->filament.diameter;
  // TODO JMM: add subdivisions of bonds for interactions, 
  //           should depend on cell length
  max_length_ = params_->filament.max_length;
  min_length_ = params_->filament.min_length;
  max_bond_length_ = params_->filament.max_bond_length;
  dynamic_instability_flag_ = params_->filament.dynamic_instability_flag;
  spiral_flag_ = params_->filament.spiral_flag;
  force_induced_catastrophe_flag_ = params_->filament.force_induced_catastrophe_flag;
  p_g2s_ = params_->filament.f_grow_to_shrink*delta_;
  p_g2p_ = params_->filament.f_grow_to_pause*delta_;
  p_s2p_ = params_->filament.f_shrink_to_pause*delta_;
  p_s2g_ = params_->filament.f_shrink_to_grow*delta_;
  p_p2s_ = params_->filament.f_pause_to_shrink*delta_;
  p_p2g_ = params_->filament.f_pause_to_grow*delta_;
  v_depoly_ = params_->filament.v_depoly;
  v_poly_ = params_->filament.v_poly;
  driving_factor_ = params_->filament.driving_factor;
  friction_ratio_ = params_->filament.friction_ratio;
  metric_forces_ = params_->filament.metric_forces;
  stoch_flag_ = params_->stoch_flag; // determines whether we are using stochastic forces
  eq_steps_ = params_->n_steps_equil;
  eq_steps_count_ = 0;
}

void Filament::InitElements() {
  bond_length_ = 0;
  int n_sites,n_bonds;
  do {
    n_bonds = (int) ceil(length_/max_bond_length_);
    if (n_bonds < 2) 
      n_bonds++;
    n_sites = n_bonds+1;
    bond_length_ = length_/n_bonds;
    if (n_bonds == 2 && bond_length_ <= diameter_) {
      error_exit("bond_length <= diameter despite minimum number of bonds.\nTry reducing filament diameter or increasing filament length.");
    }
    if (bond_length_ <= diameter_) {
      max_bond_length_ += 0.1*max_bond_length_;
      warning("bond_length <= diameter, increasing max_bond_length to %2.2f",max_bond_length_);
    }
  } while (bond_length_ <= diameter_);
  if (length_/n_bonds < min_length_) {
    error_exit("min_length_ of flexible filament segments too large for filament length.");
  }
  // Initialize mesh
  Reserve(n_bonds);

  //Allocate control structures
  tensions_.resize(n_sites-1); //max_sites -1
  g_mat_lower_.resize(n_sites-2); //max_sites-2
  g_mat_upper_.resize(n_sites-2); //max_sites-2
  g_mat_diag_.resize(n_sites-1); //max_sites-1
  det_t_mat_.resize(n_sites+1); //max_sites+1
  det_b_mat_.resize(n_sites+1); //max_sites+1
  g_mat_inverse_.resize(n_sites-2); //max_sites-2
  k_eff_.resize(n_sites-2); //max_sites-2
  h_mat_diag_.resize(n_sites-1); //max_sites-1
  h_mat_upper_.resize(n_sites-2); //max_sites-2
  h_mat_lower_.resize(n_sites-2); //max_sites-2
  gamma_inverse_.resize(n_sites*n_dim_*n_dim_); //max_sites*ndim*ndim
  cos_thetas_.resize(n_sites-2); //max_sites-2
}
void Filament::InitRandom() {
  bool out_of_bounds = true;
  do {
    out_of_bounds = false;
    Clear();
    InitRandomSite(diameter_);
    Report();
    SubReport();
    AddRandomBondToTip(bond_length_);
    Report();
    SubReport();
    //if (out_of_bounds = CheckBounds(sites_[n_sites_-1].GetPosition())) continue;
    SetOrientation(bonds_[n_bonds_-1].GetOrientation());
    for (int i=0;i<n_bonds_max_-1;++i) {
      GenerateProbableOrientation();
      AddBondToTip(orientation_, bond_length_);
      //Report();
      //SubReport();
      // if (out_of_bounds = CheckBounds(sites_[n_sites_-1].GetPosition())) break;
    }
  } while (out_of_bounds);
  UpdateBondPositions();
  UpdateSiteOrientations();
}

void Filament::Init() {
  //if (spiral_flag_) {
    //InitSpiral2D();
    //return;
  //}
  //bool probable_conformation = false;
  if (params_->filament.insertion_type.compare("random")==0) {
    InitRandom();
    //probable_conformation = true;
  }
  else {
    error_exit("Not yet\n");
  }
  //else if(params_->filament.insertion_type.compare("random_oriented")==0) {
    //InsertRandom();
    //std::fill(orientation_,orientation_+3,0.0);
    //orientation_[n_dim_-1]=1.0;
    //probable_conformation = true;
  //}
  //else if(params_->filament.insertion_type.compare("centered_random")==0) {
    //InsertRandom();
    //std::fill(position_,position_+3,0.0);
    //for (int i=0;i<n_dim_; ++i) {
      //position_[i] = position_[i] - 0.5*length_*orientation_[i];
    //}
    //probable_conformation = true;
  //}
  //else if(params_->filament.insertion_type.compare("centered_oriented")==0) {
    //InsertRandom();
    //std::fill(position_,position_+3,0.0);
    //std::fill(orientation_,orientation_+3,0.0);
    //orientation_[n_dim_-1]=1.0;
    //for (int i=0;i<n_dim_; ++i) {
      //position_[i] = position_[i] - 0.5*length_*orientation_[i];
    //}
    //// We still want to sample probable conformations
    //probable_conformation = true; 
  //}
  //else if (params_->filament.insertion_type.compare("simple_crystal")==0) {
    //probable_conformation = false;
  //}
  //else {
    //error_exit("Insertion type not recognized for filaments. Exiting.");
  //}
  //generate_random_unit_vector(n_dim_, orientation_, rng_.r);
  //for (auto site=sites_.begin(); site!=sites_.end(); ++site) {
    //site->SetDiameter(diameter_);
    //site->SetLength(bond_length_);
    //site->SetPosition(position_);
    //site->SetOrientation(orientation_);
    //for (int i=0; i<n_dim_; ++i)
      //position_[i] = position_[i] + orientation_[i] * bond_length_;
    //if (probable_conformation)
      //GenerateProbableOrientation();
  //}
  UpdatePrevPositions();
  CalculateAngles();
  //UpdateBondPositions();
  SetDiffusion();
  //poly_ = poly_state::grow;
}

void Filament::InsertAt(double *pos, double *u) {
  //std::copy(pos, pos+3,position_);
  //std::copy(u,u+3,orientation_);
  //for (int i=0; i<n_dim_; ++i) {
    //position_[i] = position_[i] - 0.5*orientation_[i]*length_;
  //}
  //for (auto site=sites_.begin(); site!=sites_.end(); ++site) {
    //site->SetPosition(position_);
    //site->SetOrientation(orientation_);
    //for (int i=0; i<n_dim_; ++i)
      //position_[i] = position_[i] + orientation_[i] * bond_length_;
  //}
  //UpdatePrevPositions();
  //CalculateAngles();
  //UpdateBondPositions();
}

//void Filament::DiffusionValidationInit() {
  //for (int i=0; i<3; ++i) {
    //position_[i] = 0.0;
    //orientation_[i] = 0.0;
  //}
  //position_[n_dim_-1] = -0.5*length_;
  //orientation_[n_dim_-1] = 1.0;
  //for (auto site=sites_.begin(); site!=sites_.end(); ++site) {
    //site->SetDiameter(diameter_);
    //site->SetLength(bond_length_);
    //site->SetPosition(position_);
    //site->SetOrientation(orientation_);
    //for (int i=0; i<n_dim_; ++i)
      //position_[i] = position_[i] + orientation_[i] * bond_length_;
  //}
  //UpdatePrevPositions();
  //CalculateAngles();
  //UpdateBondPositions();
  //SetDiffusion();
  //poly_ = poly_state::grow;
//}

// Place a spool centered at the origin
//void Filament::InitSpiral2D() {
  //if (n_dim_ > 2)
    //error_exit("3D Spirals not coded yet.");
  //double prev_pos[3] = {0, 0, 0};
  //std::fill(position_, position_+3, 0);
  //sites_[n_sites_-1].SetPosition(prev_pos);
  //for (auto site=sites_.begin(); site!= sites_.end(); ++site) {
    //site->SetDiameter(diameter_);
    //site->SetLength(bond_length_);
  //}
  //double step = diameter_ / M_PI;
  //double theta = bond_length_ / step;
  //for (int i=2; i<n_sites_+1; ++i) {
    //double move = step*theta;
    //double angle = theta + 2*M_PI;
    //position_[0] = move * cos(angle);
    //position_[1] = move * sin(angle);
    //theta += bond_length_ / move;
    //// Set current site position and orientation
    //sites_[n_sites_-i].SetPosition(position_);
    //for (int j=0; j<3; ++j) {
      //orientation_[j] = (prev_pos[j] - position_[j])/bond_length_;
      //prev_pos[j] = position_[j];
    //}
    //sites_[n_sites_-i].SetOrientation(orientation_);
  //}
  //// Set last site orientation
  //sites_[n_sites_-1].SetOrientation(sites_[n_sites_-2].GetOrientation());
  //// Init usual suspects
  //UpdatePrevPositions();
  //CalculateAngles();
  //UpdateBondPositions();
  //SetDiffusion();
  //poly_ = poly_state::grow;
//}

void Filament::SetDiffusion() {
  double logLD = log(length_/diameter_);
  //double gamma_0 = 4.0/3.0*eps*((1+0.64*eps)/(1-1.15*eps) + 1.659 * SQR(eps));
  //friction_perp_ = bond_length_ * gamma_0;
  friction_perp_ = 4.0*length_/(3.0*n_sites_*logLD);
  friction_par_ = friction_perp_ / friction_ratio_;
  rand_sigma_perp_ = sqrt(24.0*friction_perp_/delta_);
  rand_sigma_par_ = sqrt(24.0*friction_par_/delta_);
}

void Filament::GenerateProbableOrientation() {
  /* This updates the current orientation with a generated probable orientation where
  we generate random theta pulled from probability distribution P(th) = exp(k cos(th))
  where k is the persistence_length of the filament
  If k is too large, there is enormous imprecision in this calculation since sinh(k)
  is very large so to fix this I introduce an approximate distribution that is valid 
  for large k */
  double theta;
  if (persistence_length_ == 0) {
    theta = gsl_rng_uniform_pos(rng_.r) * M_PI;
  }
  else if (persistence_length_ < 100) {
    theta = gsl_rng_uniform_pos(rng_.r) * M_PI;
    theta = acos( log( exp(-persistence_length_/bond_length_) + 
          2.0*gsl_rng_uniform_pos(rng_.r)*sinh(persistence_length_/bond_length_) ) 
        / (persistence_length_/bond_length_) );
  }
  else {
    theta = acos( (log( 2.0*gsl_rng_uniform_pos(rng_.r)) - 
          log(2.0) + persistence_length_/bond_length_)
        /(persistence_length_/bond_length_) );
  }
  double new_orientation[3] = {0, 0, 0};
  if (n_dim_==2) {
    theta = (gsl_rng_uniform_int(rng_.r,2)==0 ? -1 : 1) * theta;
    new_orientation[0] = cos(theta);
    new_orientation[1] = sin(theta);
  }
  else {
    double phi = gsl_rng_uniform_pos(rng_.r) * 2.0 * M_PI;
    new_orientation[0] = sin(theta)*cos(phi);
    new_orientation[1] = sin(theta)*sin(phi);
    new_orientation[2] = cos(theta);
  }
  rotate_orientation_vector(n_dim_, new_orientation, orientation_);
  std::copy(new_orientation,new_orientation+3,orientation_);
}

void Filament::UpdatePosition(bool midstep) {
  //ApplyForcesTorques();
  Integrate(midstep);
  UpdateAvgPosition();
  eq_steps_count_++;
}

/*******************************************************************************
  BD algorithm for inextensible wormlike chains with anisotropic friction
  Montesi, Morse, Pasquali. J Chem Phys 122, 084903 (2005).
********************************************************************************/
void Filament::Integrate(bool midstep) {
  CalculateAngles();
  CalculateTangents();
  if (midstep) {
    ConstructUnprojectedRandomForces();
    GeometricallyProjectRandomForces();
    UpdatePrevPositions();
  }
  AddRandomForces();
  CalculateBendingForces();
  CalculateTensions();
  UpdateSitePositions(midstep);
  UpdateBondPositions();
  UpdateSiteOrientations();
}

void Filament::UpdateSiteOrientations() {
  for (int i=0; i<n_sites_-1; ++i) {
    sites_[i].SetOrientation(bonds_[i].GetOrientation());
  }
  sites_[n_sites_-1].SetOrientation(bonds_[n_bonds_-1].GetOrientation());
}

void Filament::CalculateAngles() {
  double cos_angle, angle_sum = 0;
  for (int i_site=0; i_site<n_sites_-2; ++i_site) {
    double const * const u1 = sites_[i_site].GetOrientation();
    double const * const u2 = sites_[i_site+1].GetOrientation();
    cos_angle = dot_product(n_dim_, u1, u2);
    cos_thetas_[i_site] = cos_angle;
    if (spiral_flag_) {
      angle_sum += acos(cos_angle);
    }
  }
  if (spiral_flag_ && angle_sum < 3) { // check if sum of angles is much less than 2*pi
    std::cout << "  Spiral terminated\n";
    early_exit = true; // exit the simulation early
    spiral_flag_ = false; // to prevent this message more than once
  }
}

void Filament::CalculateTangents() {
  for (auto it = sites_.begin(); it!=sites_.end(); ++it) {
    it->CalcTangent();
  }
}

void Filament::ConstructUnprojectedRandomForces() {
  // Create unprojected forces, see J. Chem. Phys. 122, 084903 (2005), eqn. 40.
  // xi is the random force vector with elements that are uncorrelated and randomly
  // distributed uniformly between -0.5 and 0.5, xi_term is the outer product of the
  // tangent vector u_tan_i u_tan_i acting on the vector xi
  if (!stoch_flag_) return;
  double xi[3], xi_term[3], f_rand[3];
  for (int i_site=0; i_site<n_sites_; ++i_site) {
    double const * const utan = sites_[i_site].GetTangent();
    for (int i=0; i<n_dim_; ++i) 
      xi[i] = gsl_rng_uniform_pos(rng_.r) - 0.5;
    if (n_dim_ == 2) {
      xi_term[0] = SQR(utan[0]) * xi[0] + utan[0] * utan[1] * xi[1];
      xi_term[1] = SQR(utan[1]) * xi[1] + utan[0] * utan[1] * xi[0];
    }
    else if (n_dim_ == 3) {
      xi_term[0] = SQR(utan[0]) * xi[0] + utan[0] * utan[1] * xi[1]
                  + utan[0] * utan[2] * xi[2];
      xi_term[1] = SQR(utan[1]) * xi[1] + utan[0] * utan[1] * xi[0]
                  + utan[1] * utan[2] * xi[2];
      xi_term[2] = SQR(utan[2]) * xi[2] + utan[0] * utan[2] * xi[0]
                  + utan[1] * utan[2] * xi[1];
    }
    for (int i=0; i<n_dim_; ++i) {
      f_rand[i] = rand_sigma_perp_ * xi[i] + 
          (rand_sigma_par_ - rand_sigma_perp_) * xi_term[i];
    }
    sites_[i_site].SetRandomForce(f_rand);
  }
}

void Filament::GeometricallyProjectRandomForces() {
  if (!stoch_flag_) return;
  double f_rand_temp[3];
  for (int i_site=0; i_site<n_sites_-1; ++i_site) {
    // Use the tensions vector to calculate the hard components of the random forces
    // These are not the same as the tensions, they will be calculated later
    double const * const f_rand1 = sites_[i_site].GetRandomForce();
    double const * const f_rand2 = sites_[i_site+1].GetRandomForce();
    double const * const u_site = sites_[i_site].GetOrientation();
    for (int i=0; i<n_dim_; ++i)
      f_rand_temp[i] = f_rand2[i] - f_rand1[i];
    tensions_[i_site] = dot_product(n_dim_, f_rand_temp, u_site);
    // Then get the G arrays (for inertialess case where m=1, see
    // ref. 15 of above paper)
    g_mat_diag_[i_site] = 2;
    if (i_site > 0) {
      g_mat_upper_[i_site-1] = - cos_thetas_[i_site-1];
      g_mat_lower_[i_site-1] = - cos_thetas_[i_site-1];
    }
  }
  // Now solve using tridiagonal solver
  tridiagonal_solver(&g_mat_lower_, &g_mat_diag_, &g_mat_upper_, &tensions_, n_sites_-1);
  // Update to the projected brownian forces
  // First the end sites:
  double f_proj[3];
  for (int i=0; i<n_dim_; ++i) {
    f_proj[i] = sites_[0].GetRandomForce()[i]
      + tensions_[0] * sites_[0].GetOrientation()[i];
  }
  sites_[0].SetRandomForce(f_proj);
  for (int i=0; i<n_dim_; ++i) {
    f_proj[i] = sites_[n_sites_-1].GetRandomForce()[i] 
    - tensions_[n_sites_-2] * sites_[n_sites_-2].GetOrientation()[i];
  }
  sites_[n_sites_-1].SetRandomForce(f_proj);
  // Then the rest
  for (int i_site=1; i_site<n_sites_-1; ++i_site) {
    double const * const u1 = sites_[i_site-1].GetOrientation();
    double const * const u2 = sites_[i_site].GetOrientation();
    for (int i=0; i<n_dim_; ++i) {
      f_proj[i] = sites_[i_site].GetRandomForce()[i] + tensions_[i_site]*u2[i] - tensions_[i_site-1] * u1[i];
    }
    sites_[i_site].SetRandomForce(f_proj);
  }
}

void Filament::AddRandomForces() {
  if (!stoch_flag_) return;
  for (auto site=sites_.begin(); site!=sites_.end(); ++site)
    site->AddRandomForce();
}

void Filament::CalculateBendingForces() {
  if (metric_forces_) {
    det_t_mat_[0] = 1;
    det_t_mat_[1] = 2;
    det_b_mat_[n_sites_] = 1;
    det_b_mat_[n_sites_-1] = 2;
    for (int i=2; i<n_sites_; ++i) {
      det_t_mat_[i] = 2 * det_t_mat_[i-1] - SQR(- cos_thetas_[i-2]) * det_t_mat_[i-2];
      det_b_mat_[n_sites_-i] = 2 * det_b_mat_[n_sites_-i+1] - SQR(- cos_thetas_[n_sites_-i-1]) * det_b_mat_[n_sites_-i+2];
    }
    double det_g = det_t_mat_[n_sites_-1];
    for(int i=0; i<n_sites_-2; ++i) {
      g_mat_inverse_[i] = cos_thetas_[i] * det_t_mat_[i] * det_b_mat_[i+3] / det_g;
    }
  }
  else {
    for(int i=0; i<n_sites_-2; ++i) {
      g_mat_inverse_[i] = 0;
    }
  }                                        
  // Now calculate the effective rigidities 
  for (int i=0; i<n_sites_-2; ++i) {
    k_eff_[i] = (persistence_length_ + bond_length_ * g_mat_inverse_[i])/SQR(bond_length_);
  }
  // Using these, we can calculate the forces on each of the sites
  // These calculations were done by hand and are not particularly readable,
  // but are more efficient than doing it explicitly in the code for readability
  // If this ever needs fixed, you need to either check the indices very carefully
  // or redo the calculation by hand! 
  // See Pasquali and Morse, J. Chem. Phys. Vol 116, No 5 (2002)
  double f_site[3] = {0, 0, 0};
  if (n_dim_ == 2) {
    for (int k_site=0; k_site<n_sites_; ++k_site) {
      std::fill(f_site,f_site+3,0.0);
      if (k_site>1) {
        double const * const u1 = sites_[k_site-2].GetOrientation();
        double const * const u2 = sites_[k_site-1].GetOrientation();
        f_site[0] += k_eff_[k_site-2] * ( (1-SQR(u2[0]))*u1[0] - u2[0]*u2[1]*u1[1] );
        f_site[1] += k_eff_[k_site-2] * ( (1-SQR(u2[1]))*u1[1] - u2[0]*u2[1]*u1[0] );
      }
      if (k_site>0 && k_site<n_sites_-1) {
        double const * const u1 = sites_[k_site-1].GetOrientation();
        double const * const u2 = sites_[k_site].GetOrientation();
        f_site[0] += k_eff_[k_site-1] * ( (1-SQR(u1[0]))*u2[0] - u1[0]*u1[1]*u2[1]
                        -((1-SQR(u2[0]))*u1[0] - u2[0]*u2[1]*u1[1]) );
        f_site[1] += k_eff_[k_site-1] * ( (1-SQR(u1[1]))*u2[1] - u1[0]*u1[1]*u2[0]
                        -((1-SQR(u2[1]))*u1[1] - u2[0]*u2[1]*u1[0]) );
      }
      if (k_site<n_sites_-2) {
        double const * const u1 = sites_[k_site].GetOrientation();
        double const * const u2 = sites_[k_site+1].GetOrientation();
        f_site[0] -= k_eff_[k_site] * ( (1-SQR(u1[0]))*u2[0] - u1[0]*u1[1]*u2[1] );
        f_site[1] -= k_eff_[k_site] * ( (1-SQR(u1[1]))*u2[1] - u1[0]*u1[1]*u2[0] );
      }
      sites_[k_site].AddForce(f_site);
    }
  }
  else if (n_dim_ == 3) {
    for (int k_site=0; k_site<n_sites_; ++k_site) {
      std::fill(f_site,f_site+3,0.0);
      if (k_site>1) {
        double const * const u1 = sites_[k_site-2].GetOrientation();
        double const * const u2 = sites_[k_site-1].GetOrientation();
        f_site[0] += k_eff_[k_site-2] * ( (1-SQR(u2[0]))*u1[0] - u2[0]*u2[1]*u1[1] - u2[0]*u2[2]*u1[2] );
        f_site[1] += k_eff_[k_site-2] * ( (1-SQR(u2[1]))*u1[1] - u2[1]*u2[0]*u1[0] - u2[1]*u2[2]*u1[2] );
        f_site[2] += k_eff_[k_site-2] * ( (1-SQR(u2[2]))*u1[2] - u2[2]*u2[0]*u1[0] - u2[2]*u2[1]*u1[1] );
      }
      if (k_site>0 && k_site<n_sites_-1) {
        double const * const u1 = sites_[k_site-1].GetOrientation();
        double const * const u2 = sites_[k_site].GetOrientation();
        f_site[0] += k_eff_[k_site-1] * ( (1-SQR(u1[0]))*u2[0] - u1[0]*u1[1]*u2[1] - u1[0]*u1[2]*u2[2] 
                        - ( (1-SQR(u2[0]))*u1[0] - u2[0]*u2[1]*u1[1] - u2[0]*u2[2]*u1[2] ) );
        f_site[1] += k_eff_[k_site-1] * ( (1-SQR(u1[1]))*u2[1] - u1[1]*u1[0]*u2[0] - u1[1]*u1[2]*u2[2]
                        - ( (1-SQR(u2[1]))*u1[1] - u2[1]*u2[0]*u1[0] - u2[1]*u2[2]*u1[2] ) );
        f_site[2] += k_eff_[k_site-1] * ( (1-SQR(u1[2]))*u2[2] - u1[2]*u1[0]*u2[0] - u1[2]*u1[1]*u2[1]
                        - ( (1-SQR(u2[2]))*u1[2] - u2[2]*u2[0]*u1[0] - u2[1]*u2[2]*u1[1] ) );
      }
      if(k_site<n_sites_-2) {
        double const * const u1 = sites_[k_site].GetOrientation();
        double const * const u2 = sites_[k_site+1].GetOrientation();
        f_site[0] -= k_eff_[k_site] * ( (1-SQR(u1[0]))*u2[0] - u1[0]*u1[1]*u2[1] - u1[0]*u1[2]*u2[2] );
        f_site[1] -= k_eff_[k_site] * ( (1-SQR(u1[1]))*u2[1] - u1[1]*u1[0]*u2[0] - u1[1]*u1[2]*u2[2] );
        f_site[2] -= k_eff_[k_site] * ( (1-SQR(u1[2]))*u2[2] - u1[2]*u1[0]*u2[0] - u1[2]*u1[1]*u2[1] );
      }
      sites_[k_site].AddForce(f_site);
    }
  }
}

void Filament::CalculateTensions() {
  // Calculate friction_inverse matrix
  int site_index = 0;
  int next_site = n_dim_*n_dim_;
  for (int i_site=0; i_site<n_sites_; ++i_site) {
    int gamma_index = 0;
    double const * const utan = sites_[i_site].GetTangent();
    for (int i=0; i<n_dim_; ++i) {
      for (int j=0; j<n_dim_; ++j) {
        gamma_inverse_[site_index+gamma_index] = 1.0/friction_par_ * (utan[i]*utan[j])
                                     + 1.0/friction_perp_ * ((i==j ? 1 : 0) - utan[i]*utan[j]);
        gamma_index++;
      }
    }
    site_index += next_site;
  }
  // Populate the H matrix and Q vector using tensions (p_vec) array
  double temp_a, temp_b;
  double f_diff[3];
  double utan1_dot_u2, utan2_dot_u2;
  site_index = 0;
  for (int i_site=0; i_site<n_sites_-1; ++i_site) {
    // f_diff is the term in par_entheses in equation 29 of J. Chem. Phys. 122, 084903 (2005)
    double const * const f1 = sites_[i_site].GetForce();
    double const * const f2 = sites_[i_site+1].GetForce();
    double const * const u2 = sites_[i_site].GetOrientation();
    double const * const utan1 = sites_[i_site].GetTangent();
    double const * const utan2 = sites_[i_site+1].GetTangent();
    for (int i=0; i<n_dim_; ++i) {
      temp_a = gamma_inverse_[site_index+n_dim_*i] * f1[0] + gamma_inverse_[site_index + n_dim_*i+1] * f1[1];
      if (n_dim_ == 3) temp_a += gamma_inverse_[site_index+n_dim_*i+2] * f1[2];
      temp_b = gamma_inverse_[site_index+next_site+n_dim_*i] * f2[0] + gamma_inverse_[site_index+next_site+n_dim_*i+1] * f2[1];
      if (n_dim_ == 3) temp_b += gamma_inverse_[site_index+next_site+n_dim_*i+2] * f2[2];
      f_diff[i] = temp_b - temp_a;
    }
    tensions_[i_site] = dot_product(n_dim_, u2, f_diff);
    utan1_dot_u2 = dot_product(n_dim_, utan1, u2);
    utan2_dot_u2 = dot_product(n_dim_, utan2, u2);
    h_mat_diag_[i_site] = 2.0/friction_perp_ + (1.0/friction_par_ - 1.0/friction_perp_) *
      (SQR(utan1_dot_u2) + SQR(utan2_dot_u2));
    if (i_site>0) {
      double const * const u1 = sites_[i_site-1].GetOrientation();
      h_mat_upper_[i_site-1] = -1.0/friction_perp_ * dot_product(n_dim_, u2, u1)
        - (1.0/friction_par_ - 1.0/friction_perp_) * (dot_product(n_dim_, utan1, u1) *
            dot_product(n_dim_, utan1, u2));
      h_mat_lower_[i_site-1] = h_mat_upper_[i_site-1];
    }
    site_index += next_site;
  }
  tridiagonal_solver(&h_mat_lower_, &h_mat_diag_, &h_mat_upper_, &tensions_, n_sites_-1);
}

void Filament::UpdateSitePositions(bool midstep) {
  double delta = (midstep ? 0.5*delta_ : delta_);
  double f_site[3];
  // First get total forces
  // Handle end sites first
  for (int i=0; i<n_dim_; ++i)
    f_site[i] = tensions_[0] * sites_[0].GetOrientation()[i];
  sites_[0].AddForce(f_site);
  for (int i=0; i<n_dim_; ++i)
    f_site[i] = -tensions_[n_sites_-2] * sites_[n_sites_-2].GetOrientation()[i];
  sites_[n_sites_-1].AddForce(f_site);
  // and then the rest
  for (int i_site=1; i_site<n_sites_-1; ++i_site) {
    double const * const u_site1 = sites_[i_site-1].GetOrientation();
    double const * const u_site2 = sites_[i_site].GetOrientation();
    for (int i=0; i<n_dim_; ++i) {
      f_site[i] = tensions_[i_site] * u_site2[i] - tensions_[i_site-1] * u_site1[i];
    }
    sites_[i_site].AddForce(f_site);
  }
  // Now update positions
  double f_term[3], r_new[3];
  int site_index = 0;
  int next_site = n_dim_*n_dim_;
  for (int i_site=0; i_site<n_sites_; ++i_site) {
    double const * const f_site1 = sites_[i_site].GetForce();
    double const * const r_site1 = sites_[i_site].GetPosition();
    double const * const r_prev = sites_[i_site].GetPrevPosition();
    for (int i=0; i<n_dim_; ++i) {
      f_term[i] = gamma_inverse_[site_index+n_dim_*i] * f_site1[0] + gamma_inverse_[site_index+n_dim_*i+1] * f_site1[1];
      if (n_dim_ == 3)
        f_term[i] += gamma_inverse_[site_index+n_dim_*i+2] * f_site1[2];
      r_new[i] = r_prev[i] + delta * f_term[i];
    }
    sites_[i_site].SetPosition(r_new);
    site_index += next_site;
  }
  // Next, update orientation vectors
  double u_mag, r_diff[3];
  for (int i_site=0; i_site<n_sites_-1; ++i_site) {
    double const * const r_site1 = sites_[i_site].GetPosition();
    double const * const r_site2 = sites_[i_site+1].GetPosition();
    u_mag = 0.0;
    for (int i=0; i<n_dim_; ++i) {
      r_diff[i] = r_site2[i] - r_site1[i];
      u_mag += SQR(r_diff[i]);
    }
    u_mag = sqrt(u_mag);
    for (int i=0; i<n_dim_; ++i)
      r_diff[i]/=u_mag;
    sites_[i_site].SetOrientation(r_diff);
  }
  sites_[n_sites_-1].SetOrientation(sites_[n_sites_-2].GetOrientation());
  // Finally, normalize site positions, making sure the sites are still rod-length apart
  for (int i_site=1; i_site<n_sites_; ++i_site) {
    double const * const r_site1 = sites_[i_site-1].GetPosition();
    double const * const u_site1 = sites_[i_site-1].GetOrientation();
    for (int i=0; i<n_dim_; ++i)
      r_diff[i] = r_site1[i] + bond_length_ * u_site1[i];
    sites_[i_site].SetPosition(r_diff);
  }
}

void Filament::UpdateAvgPosition() {
  std::fill(position_, position_+3, 0.0);
  std::fill(orientation_, orientation_+3, 0.0);
  for (auto site_it=sites_.begin(); site_it!=sites_.end(); ++site_it) {
    double const * const site_pos = site_it->GetPosition();
    double const * const site_u = site_it->GetOrientation();
    for (int i=0; i<n_dim_; ++i) {
      position_[i] += site_pos[i];
      orientation_[i] += site_u[i];
    }
  }
  normalize_vector(orientation_, n_dim_);
  for (int i=0; i<n_dim_; ++i) {
    position_[i] /= n_sites_;
  }
}

void Filament::UpdatePrevPositions() {
  for (auto site=sites_.begin(); site!=sites_.end(); ++site)
    site->SetPrevPosition(site->GetPosition());
}

double const * const Filament::GetDrTot() {return nullptr;}

void Filament::ApplyForcesTorques() {
  double pure_torque[3] = {0,0,0};
  double site_force[3] = {0,0,0};
  double linv=1.0/bond_length_;
  for (int i=0; i<n_bonds_; ++i) {
    double const * const f = bonds_[i].GetForce();
    double const * const t = bonds_[i].GetTorque();
    double const * const u = sites_[i].GetOrientation();
    AddPotential(bonds_[i].GetPotentialEnergy());
    // Convert torques into forces at bond ends
    // u x t / bond_length = pure torque force at tail of bond
    cross_product(u, t, pure_torque, 3);
    for (int i=0; i<n_dim_; ++i) {
      pure_torque[i]*=linv;
      site_force[i] = 0.5*f[i];
    }
    // Add translational forces and pure torque forces at bond ends
    sites_[i].AddForce(site_force);
    sites_[i].AddForce(pure_torque);
    for (int j=0; j<n_dim_; ++j)
      pure_torque[j] *= -1;
    sites_[i+1].AddForce(site_force);
    sites_[i+1].AddForce(pure_torque);
    // Add driving (originating from the com of the bond)
    // The driving factor is a force per unit length,
    // so need to multiply by bond length to get f_dr on bond
    if (eq_steps_count_ > eq_steps_) {
      double f_dr[3];
      for (int j=0; j<n_dim_; ++j)
        f_dr[j] = 0.5*u[j]*driving_factor_ * bond_length_;
      sites_[i].AddForce(f_dr);
      sites_[i+1].AddForce(f_dr);
    }
  }
}

void Filament::Draw(std::vector<graph_struct*> * graph_array) {
  for (auto bond=bonds_.begin(); bond!= bonds_.end(); ++bond) {
    bond->Draw(graph_array);
  }
}

// Scale bond and site positions from new unit cell
void Filament::ScalePosition() {
  // scale first bond position using new unit cell
  bonds_[0].ScalePosition();
  // then reposition sites based on first bond position
  // handle first site
  double r[3];
  double const * const bond_r = bonds_[0].GetPosition();
  double const * const bond_u = bonds_[0].GetOrientation();
  for (int i=0; i<n_dim_; ++i)
    r[i] = bond_r[i] - 0.5*bond_length_*bond_u[i];
  sites_[0].SetPosition(r);
  // then handle remaining sites
  for (int i_site=1; i_site<n_sites_; ++i_site) {
    double const * const prev_r = sites_[i_site-1].GetPosition();
    double const * const prev_u = sites_[i_site-1].GetOrientation();
    for (int i=0; i<n_dim_; ++i)
      r[i] = prev_r[i] + bond_length_*prev_u[i];
    sites_[i_site].SetPosition(r);
  }
  // update remaining bond positions
  UpdateBondPositions();
}


void Filament::GetAvgOrientation(double * au) {
  double avg_u[3] = {0.0, 0.0, 0.0};
  int size=0;
  for (auto it=sites_.begin(); it!=sites_.end(); ++it) {
    double const * const u = it->GetOrientation();
    for (int i=0; i<n_dim_; ++i)
      avg_u[i] += u[i];
    size++;
  }
  if (size == 0)
    error_exit("Something went wrong in GetAvgOrientation!");
  for (int i=0; i<n_dim_; ++i)
    avg_u[i]/=size;
  std::copy(avg_u, avg_u+3, au);
}

void Filament::GetAvgPosition(double * ap) {
  double avg_p[3] = {0.0, 0.0, 0.0};
  int size=0;
  for (auto it=sites_.begin(); it!=sites_.end(); ++it) {
    double const * const p = it->GetPosition();
    for (int i=0; i<n_dim_; ++i)
      avg_p[i] += p[i];
    size++;
  }
  if (size == 0)
    error_exit("Something went wrong in GetAvgPosition!");
  for (int i=0; i<n_dim_; ++i)
    avg_p[i]/=size;
  std::copy(avg_p, avg_p+3, ap);
}

void Filament::ReportAll() {
  printf("tensions:\n  {");
  for (int i=0; i<n_sites_-1; ++i)
    printf(" %5.5f ",tensions_[i]);
  printf("}\n");
  printf("cos_thetas:\n  {");
  for (int i=0; i<n_sites_-2; ++i)
    printf(" %5.5f ",cos_thetas_[i]);
  printf("}\n");
  printf("g_mat_lower:\n  {");
  for (int i=0; i<n_sites_-2; ++i)
    printf(" %5.5f ",g_mat_lower_[i]);
  printf("}\n");
  printf("g_mat_upper:\n  {");
  for (int i=0; i<n_sites_-2; ++i)
    printf(" %5.5f ",g_mat_upper_[i]);
  printf("}\n");
  printf("g_mat_diag:\n  {");
  for (int i=0; i<n_sites_-1; ++i)
    printf(" %5.5f ",g_mat_diag_[i]);
  printf("}\n");
  printf("det_t_mat:\n  {");
  for (int i=0; i<n_sites_+1; ++i)
    printf(" %5.5f ",det_t_mat_[i]);
  printf("}\n");
  printf("det_b_mat:\n  {");
  for (int i=0; i<n_sites_+1; ++i)
    printf(" %5.5f ",det_b_mat_[i]);
  printf("}\n");
  printf("h_mat_diag:\n  {");
  for (int i=0; i<n_sites_-1; ++i)
    printf(" %5.5f ",h_mat_diag_[i]);
  printf("}\n");
  printf("h_mat_upper:\n  {");
  for (int i=0; i<n_sites_-2; ++i)
    printf(" %5.5f ",h_mat_upper_[i]);
  printf("}\n");
  printf("h_mat_lower:\n  {");
  for (int i=0; i<n_sites_-2; ++i)
    printf(" %5.5f ",h_mat_lower_[i]);
  printf("}\n");
  printf("k_eff:\n  {");
  for (int i=0; i<n_sites_-2; ++i)
    printf(" %5.5f ",k_eff_[i]);
  printf("}\n\n\n");
}

/* The spec output for one filament is:
    diameter
    length
    persistence_length (added 1/17/2017)
    friction_par (added 1/17/2017)
    friction_perp (added 1/17/2017)
    bond_length
    n_bonds,
    position of first site
    position of last site
    all bond orientations
    */

void Filament::WriteSpec(std::fstream &ospec){
  ospec.write(reinterpret_cast<char*>(&diameter_), sizeof(diameter_));
  ospec.write(reinterpret_cast<char*>(&length_), sizeof(length_));
  ospec.write(reinterpret_cast<char*>(&persistence_length_), sizeof(persistence_length_));
  ospec.write(reinterpret_cast<char*>(&friction_par_), sizeof(friction_par_));
  ospec.write(reinterpret_cast<char*>(&friction_perp_), sizeof(friction_perp_));
  ospec.write(reinterpret_cast<char*>(&bond_length_), sizeof(bond_length_));
  ospec.write(reinterpret_cast<char*>(&n_bonds_), sizeof(n_bonds_));
  double temp[3];
  double const * const r0 = sites_[0].GetPosition();
  std::copy(r0, r0+3, temp);
  for (auto& pos : temp)
    ospec.write(reinterpret_cast<char*>(&pos), sizeof(pos));
  double const * const rf = sites_[n_bonds_].GetPosition();
  std::copy(rf, rf+3, temp);
  for (auto& pos : temp)
    ospec.write(reinterpret_cast<char*>(&pos), sizeof(pos));
  for (int i=0; i<n_bonds_; ++i) {
    double const * const orientation = bonds_[i].GetOrientation();
    std::copy(orientation, orientation+3, temp);
    for (auto& u : temp) 
      ospec.write(reinterpret_cast<char*>(&u), sizeof(u));
  }
  return;
}

void Filament::ReadSpec(std::fstream &ispec) {
  if (ispec.eof()) return;
  double r0[3], rf[3], u_bond[3];
  ispec.read(reinterpret_cast<char*>(&diameter_), sizeof(diameter_));
  ispec.read(reinterpret_cast<char*>(&length_), sizeof(length_));
  ispec.read(reinterpret_cast<char*>(&persistence_length_), sizeof(persistence_length_));
  ispec.read(reinterpret_cast<char*>(&friction_par_), sizeof(friction_par_));
  ispec.read(reinterpret_cast<char*>(&friction_perp_), sizeof(friction_perp_));
  ispec.read(reinterpret_cast<char*>(&bond_length_), sizeof(bond_length_));
  ispec.read(reinterpret_cast<char*>(&n_bonds_), sizeof(n_bonds_));
  bonds_.resize(n_bonds_, bonds_[0]);
  // Get initial site position
  for (int i=0; i<3; ++i)
    ispec.read(reinterpret_cast<char*>(&r0[i]), sizeof(double));
  for (int i=0; i<3; ++i)
    ispec.read(reinterpret_cast<char*>(&rf[i]), sizeof(double));
  // Initialize bonds from relative orientations
  for (int i_bond=0; i_bond<n_bonds_; ++i_bond) {
    for (int i=0; i<3; ++i)
      ispec.read(reinterpret_cast<char*>(&u_bond[i]), sizeof(double));
    for (int i=0; i<n_dim_; ++i) {
      // Set bond position
      rf[i] = r0[i] + 0.5 * bond_length_ * u_bond[i];
      // Set next site position
      r0[i] += bond_length_ * u_bond[i];
    }
    sites_[i_bond].SetOrientation(u_bond);
    bonds_[i_bond].SetPosition(rf);
    bonds_[i_bond].SetOrientation(u_bond);
    bonds_[i_bond].SetDiameter(diameter_);
    bonds_[i_bond].SetLength(bond_length_);
    bonds_[i_bond].UpdatePeriodic();
  }
  sites_[n_bonds_].SetOrientation(sites_[n_bonds_-1].GetOrientation());
  double pos[3];
  for (int i=0; i < params_->n_dim; ++i) {
    pos[i] = bonds_[0].GetPosition()[i] - 0.5*bonds_[0].GetOrientation()[i] * bond_length_;
  }
  sites_[0].SetPosition(pos);
  for (int i=0; i < params_->n_dim; ++i) {
    pos[i] = bonds_[n_bonds_-1].GetPosition()[i] + 0.5*bonds_[n_bonds_-1].GetOrientation()[i] * bond_length_;
  }
  sites_[n_bonds_].SetPosition(pos);
  CalculateAngles();
}

void Filament::WritePosit(std::fstream &oposit) {
  double avg_pos[3], avg_u[3];
  GetAvgPosition(avg_pos);
  GetAvgOrientation(avg_u);
  std::copy(avg_pos,avg_pos+3,position_);
  UpdatePeriodic();
  for (auto& pos : position_)
    oposit.write(reinterpret_cast<char*>(&pos), sizeof(pos));
  for (auto& spos : scaled_position_)
    oposit.write(reinterpret_cast<char*>(&spos), sizeof(spos));
  for (auto& u : avg_u) 
    oposit.write(reinterpret_cast<char*>(&u), sizeof(u));
  oposit.write(reinterpret_cast<char*>(&diameter_), sizeof(diameter_));
  oposit.write(reinterpret_cast<char*>(&length_), sizeof(length_));
}

void Filament::ReadPosit(std::fstream &iposit) {
  if (iposit.eof()) return;
  double avg_pos[3], avg_u[3], s_pos[3];
  for (int i=0; i<3; ++i)
    iposit.read(reinterpret_cast<char*>(&avg_pos[i]), sizeof(double));
  for (int i=0; i<3; ++i)
    iposit.read(reinterpret_cast<char*>(&s_pos[i]), sizeof(double));
  for (int i=0; i<3; ++i)
    iposit.read(reinterpret_cast<char*>(&avg_u[i]), sizeof(double));
  iposit.read(reinterpret_cast<char*>(&diameter_), sizeof(diameter_));
  iposit.read(reinterpret_cast<char*>(&length_), sizeof(length_));
  // Initialize first bond position
  for (int i=0; i<n_dim_; ++i) 
    avg_pos[i] = avg_pos[i] - 0.5*(length_ - bond_length_)*avg_u[i];
  for (int i_bond=0; i_bond<n_bonds_; ++i_bond) {
    bonds_[i_bond].SetPosition(avg_pos);
    bonds_[i_bond].SetOrientation(avg_u);
    bonds_[i_bond].SetDiameter(diameter_);
    bonds_[i_bond].UpdatePeriodic();
    // Set next bond position
    for (int i=0; i<n_dim_; ++i) 
      avg_pos[i] += bond_length_*avg_u[i];
  }
}

void Filament::WriteCheckpoint(std::fstream &ocheck) {
  void * rng_state = gsl_rng_state(rng_.r);
  size_t rng_size = gsl_rng_size(rng_.r);
  ocheck.write(reinterpret_cast<char*>(&rng_size), sizeof(size_t));
  ocheck.write(reinterpret_cast<char*>(rng_state), rng_size);
  WriteSpec(ocheck);
}

void Filament::ReadCheckpoint(std::fstream &icheck) {
  if (icheck.eof()) return;
  void * rng_state = gsl_rng_state(rng_.r);
  size_t rng_size;
  icheck.read(reinterpret_cast<char*>(&rng_size), sizeof(size_t));
  icheck.read(reinterpret_cast<char*>(rng_state), rng_size);
  ReadSpec(icheck);
  n_sites_ = n_bonds_+1;
  double r[3];
  for (int i=0; i<3; ++i)
    r[i] = bonds_[0].GetPosition()[i] - 0.5*bond_length_*bonds_[0].GetOrientation()[i];
  sites_[0].SetPosition(r);
  sites_[0].SetOrientation(bonds_[0].GetOrientation());
  for (int i_bond=1; i_bond<n_bonds_; ++i_bond) {
    for (int i=0; i<3; ++i)
      r[i] += bond_length_*sites_[i_bond-1].GetOrientation()[i];
    sites_[i_bond].SetPosition(r);
    sites_[i_bond].SetOrientation(bonds_[i_bond].GetOrientation());
  }
  for (int i=0; i<3; ++i)
    r[i] += bond_length_*sites_[n_sites_-2].GetOrientation()[i];
  sites_[n_sites_-1].SetPosition(r); 
  sites_[n_sites_-1].SetOrientation(bonds_[n_bonds_-1].GetOrientation());
  UpdateBondPositions();
  // Hack CIDs and RIDs to Set up interaction zones
  //for (int i=0; i<n_bonds_; ++i) {
    //bonds_[i].InitRID();
    //bonds_[i].SetCID(bonds_[i].GetRID());
    //bonds_[i].UpdatePeriodic();
  //}
  //for (int i=1; i<n_bonds_-1; ++i) {
    //if (i%2==0) 
      //bonds_[i].SetRID(bonds_[i-1].GetRID());
    //else
      //bonds_[i].SetCID(bonds_[i-1].GetCID());
  //}
  //bonds_[n_bonds_-1].SetCID(bonds_[n_bonds_-2].GetCID());
  //bonds_[n_bonds_-1].SetRID(bonds_[n_bonds_-2].GetRID());

  //Reallocate control structures
  tensions_.resize(n_sites_-1); //max_sites -1
  g_mat_lower_.resize(n_sites_-2); //max_sites-2
  g_mat_upper_.resize(n_sites_-2); //max_sites-2
  g_mat_diag_.resize(n_sites_-1); //max_sites-1
  det_t_mat_.resize(n_sites_+1); //max_sites+1
  det_b_mat_.resize(n_sites_+1); //max_sites+1
  g_mat_inverse_.resize(n_sites_-2); //max_sites-2
  k_eff_.resize(n_sites_-2); //max_sites-2
  h_mat_diag_.resize(n_sites_-1); //max_sites-1
  h_mat_upper_.resize(n_sites_-2); //max_sites-2
  h_mat_lower_.resize(n_sites_-2); //max_sites-2
  gamma_inverse_.resize(n_sites_*n_dim_*n_dim_); //max_sites*ndim*ndim
  cos_thetas_.resize(n_sites_-2); //max_sites-2
}

void FilamentSpecies::InitAnalysis() {
  time_ = 0;
  if (params_->filament.spiral_flag) {
    InitSpiralAnalysis();
  }
  if (params_->filament.theta_analysis) {
    InitThetaAnalysis();
  }
  if (params_->filament.lp_analysis) {
    InitMse2eAnalysis();
  }
  RunAnalysis();
}

void FilamentSpecies::InitMse2eAnalysis() {
  std::string fname = params_->run_name;
  fname.append("_filament.mse2e");
  mse2e_file_.open(fname, std::ios::out);
  mse2e_file_ << "mse2e_analysis_file\n";
  mse2e_file_ << "length diameter bond_length persistence_length driving ndim nsteps nspec delta theory\n";
  auto it=members_.begin();
  double l = it->GetLength();
  double d = it->GetDiameter();
  double cl = it->GetBondLength();
  double pl = it->GetPersistenceLength();
  double dr = it->GetDriving();
  double nspec = GetNSpec();
  double theory;
  if (params_->n_dim == 2) {
    theory = l * pl * 4.0 - 8.0 * pl * pl * (1-exp(-0.5*l/pl));
  }
  else {
    theory = l * pl * 2.0 - 2.0 * pl * pl * (1-exp(-l/pl));
  }
  mse2e_file_ << l << " " << d << " " << cl << " " << pl << " " << dr << " " << params_->n_dim << " " << params_->n_steps << " " << nspec << " " << params_->delta << " " << theory << "\n";
  mse2e_file_ << "num_filaments_averaged mse2e_mean mse2e_std_err\n";
  mse2e_ = 0.0;
  mse2e2_ = 0.0;
  n_samples_ = 0;
}

void FilamentSpecies::InitSpiralAnalysis() {
  std::string fname = params_->run_name;
  fname.append("_filament.spiral");
  spiral_file_.open(fname, std::ios::out);
  spiral_file_ << "spiral_analysis_file\n";
  spiral_file_ << "length diameter bond_length persistence_length driving nsteps nspec delta\n";
  for (auto it=members_.begin(); it!=members_.end(); ++it) {
    double l = it->GetLength();
    double d = it->GetDiameter();
    double cl = it->GetBondLength();
    double pl = it->GetPersistenceLength();
    double dr = it->GetDriving();
    double nspec = GetNSpec();
    spiral_file_ << l << " " << d << " " << cl << " " << pl << " " << dr << " " << params_->n_steps << " " << nspec << " " << params_->delta << "\n";
  }
  spiral_file_ << "time angle_sum E_bend tip_z_proj head_pos_x head_pos_y tail_pos_x tail_pos_y\n";
}

void FilamentSpecies::InitThetaAnalysis() {
  // TODO Should check to make sure the same lengths, child lengths, persistence lengths, etc are used for each filament in system.
  std::string fname = params_->run_name;
  fname.append("_filament.theta");
  theta_file_.open(fname, std::ios::out);
  theta_file_ << "theta_analysis_file\n";
  theta_file_ << "length diameter bond_length persistence_length n_filaments n_bonds n_steps n_spec delta n_dim metric_forces\n";
  double l, cl, pl, dr, d;
  int nbonds;
  int nmembers = members_.size();
  for (auto it=members_.begin(); it!=members_.end(); ++it) {
    l = it->GetLength();
    d = it->GetDiameter();
    cl = it->GetBondLength();
    pl = it->GetPersistenceLength();
    dr = it->GetDriving();
    nbonds = it->GetNBonds();
  }
  int nspec = GetNSpec();
  theta_file_ << l << " " << d << " " << cl << " " << pl << " " << nmembers << " " << nbonds << " " << params_->n_steps << " " << nspec << " " << params_->delta << " " << params_->n_dim << " " << params_->filament.metric_forces << "\n";
  theta_file_ << "cos_theta";
  for (int i=0; i<nbonds-1; ++i) {
    theta_file_ << " theta_" << i+1 << i+2;
  }
  theta_file_ << "\n";
  n_bins_ = 10000;
  int nfil = members_.size();
  theta_histogram_ = new int *[nbonds-1];
  for (int ibond=0; ibond<nbonds-1; ++ibond) {
    theta_histogram_[ibond] = new int[n_bins_];
    for (int ibin=0;ibin<n_bins_;++ibin) {
      theta_histogram_[ibond][ibin] = 0;
    }
  }
}

void FilamentSpecies::RunAnalysis() {
  if (params_->filament.spiral_flag) {
    RunSpiralAnalysis();
  }
  // TODO Analyze conformation and ms end-to-end
  if (params_->filament.theta_analysis) {
    if (params_->interaction_flag) {
      std::cout << "WARNING! Theta analysis running on interacting filaments!\n";
    }
    RunThetaAnalysis();
  }
  if (params_->filament.lp_analysis) {
    RunMse2eAnalysis();
  }
  time_++;
}

void FilamentSpecies::RunSpiralAnalysis() {
  // Treat as though we have many spirals for now
  double tip_z;
  auto it=members_.begin();
  e_bend_ = tot_angle_ = 0;
  double length = it->GetLength();
  double plength = it->GetPersistenceLength();
  double clength = it->GetBondLength();
  double e_zero = length * plength / (clength * clength);
  std::vector<double> const * const thetas = it->GetThetas();
  for (int i=0; i<thetas->size(); ++i) {
    tot_angle_ += acos((*thetas)[i]);
    e_bend_ += (*thetas)[i];
  }
  // record energy relative to the bending "zero energy" (straight rod)
  e_bend_ = e_zero - e_bend_ * plength / clength;
  tip_z = it->GetTipZ();
  double const * const head_pos = it->GetHeadPos();
  double const * const tail_pos = it->GetTailPos();
  if (spiral_file_.is_open()) {
    spiral_file_ << time_ << " " << tot_angle_ << " " << e_bend_ << " " << tip_z << " " << head_pos[0] << " " << head_pos[1] << " " << tail_pos[0] << " " << tail_pos[1] << "\n";
  }
  else {
    early_exit = true;
    std::cout << "ERROR: Problem opening file in RunSpiralAnalysis! Exiting.\n";
  }
}

void FilamentSpecies::RunMse2eAnalysis() {
  // Treat as though we have many spirals for now
  //if ( ! mse2e_file_.is_open()) {
    //early_exit = true;
    //std::cout << " Error! Problem opening file in RunMse2eAnalysis! Exiting.\n";
  //}
  //mse2e_file_ << time_;
  for (auto it=members_.begin(); it!= members_.end(); ++it) {
    double const * const head_pos = it->GetHeadPos();
    double const * const tail_pos = it->GetTailPos();
    double mse2e_temp = 0.0;
    for (int i=0; i<params_->n_dim; ++i) {
      double temp = (head_pos[i] - tail_pos[i]);
      mse2e_temp += temp*temp;
    }
    mse2e_ += mse2e_temp;
    mse2e2_ += mse2e_temp*mse2e_temp;
    //mse2e_file_ << " " << mse2e ;
  }
  //mse2e_ /= members_.size();
  //mse2e2_ /= members_.size();
  //mse2e_file_ << "\n";
  n_samples_++;
}


void FilamentSpecies::RunThetaAnalysis() {
  for (auto it=members_.begin(); it!=members_.end(); ++it) {
    std::vector<double> const * const thetas = it->GetThetas();
    for (int i=0; i<(it->GetNBonds()-1); ++i) {
      int bin_number = (int) floor( (1 + (*thetas)[i]) * (n_bins_/2) );
      if (bin_number == n_bins_) {
        bin_number = n_bins_-1;
      }
      else if (bin_number == -1) {
        bin_number = 0;
      }
      else if (bin_number > n_bins_ && bin_number < 0) {
        error_exit("Something went wrong in RunThetaAnalysis!");
      }
      theta_histogram_[i][bin_number]++;
    }
  }
}

void FilamentSpecies::FinalizeAnalysis() {
  if (spiral_file_.is_open()) {
    spiral_file_.close();
  }
  if (theta_file_.is_open()) {
    FinalizeThetaAnalysis();
    theta_file_.close();
  }
  if (mse2e_file_.is_open()) {
    FinalizeMse2eAnalysis();
    mse2e_file_.close();
  }
}

void FilamentSpecies::FinalizeMse2eAnalysis() {
  int num = members_.size();
  mse2e_file_ << num << " ";
  mse2e_ /= n_samples_*num;
  mse2e2_ /= n_samples_*num;
  mse2e_file_ << mse2e_ << " ";
  mse2e_file_ << sqrt((mse2e2_ - mse2e_*mse2e_)/num) << "\n";
}

void FilamentSpecies::FinalizeThetaAnalysis() {
  int nbonds = members_[members_.size()-1].GetNBonds();
  for (int i=0; i<n_bins_; ++i) {
    double axis = (2.0/n_bins_)*i - 1;
    theta_file_ << " " << axis;
    for (int ibond=0; ibond<nbonds-1; ++ibond) {
      theta_file_ << " " << theta_histogram_[ibond][i];
    }
    theta_file_ << "\n";
  }

  for (int ibond=0; ibond<nbonds-1; ++ibond) {
    delete[] theta_histogram_[ibond];
  }
  delete[] theta_histogram_;
}

