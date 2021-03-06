#include "fields.h"

namespace elec {

ElectricFieldHolder::ElectricFieldHolder(size_t n) {
  maxnmon = n;
  v0_  = std::vector<double> (maxnmon);
  v1_  = std::vector<double> (maxnmon);
  v2_  = std::vector<double> (maxnmon);
  v3_  = std::vector<double> (maxnmon);
  v4_  = std::vector<double> (maxnmon);
  v5_  = std::vector<double> (maxnmon);
  v6_  = std::vector<double> (maxnmon);
  v7_  = std::vector<double> (maxnmon);
  v8_  = std::vector<double> (maxnmon);
  v9_  = std::vector<double> (maxnmon);
  v10_ = std::vector<double> (maxnmon);
}

////////////////////////////////////////////////////////////////////////////////

void ElectricFieldHolder::CalcPermanentElecFieldWithPolfacNonZero
  (double * xyz1, double * xyz2, 
   double * chg1, double * chg2,
   size_t mon1_index, 
   size_t mon2_index_start, size_t mon2_index_end,
   size_t nmon1, size_t nmon2,
   size_t site_i, size_t site_j,
   double Ai, double Asqsq,
   double aCC, double aCC1_4,
   double g34, 
   double *Efqx_mon1, double *Efqy_mon1, 
   double *Efqz_mon1, double *phi1,
   double * phi2,
   double * Efq2) {
             
  // Shifts that will be useful in the loops
  const size_t nmon12 = nmon1 * 2;
  const size_t nmon22 = nmon2 * 2;
  const size_t site_i3 = site_i * 3;
  const size_t site_j3 = site_j * 3;
  const size_t site_inmon1 = site_i * nmon1;
  const size_t site_jnmon2 = site_j * nmon2;
  const size_t site_inmon13 = nmon1 * site_i3;
  const size_t site_jnmon23 = nmon2 * site_j3;

  // Coordinates x, y and z of site i of monomer 1
  const double xyzmon1_x = xyz1[site_inmon13 + mon1_index];
  const double xyzmon1_y = xyz1[site_inmon13 + nmon1 + mon1_index];
  const double xyzmon1_z = xyz1[site_inmon13 + nmon12 + mon1_index];

  // Fill vectors with zeros in the desired range
  std::fill(v0_.begin() + mon2_index_start, v0_.begin() + mon2_index_end, 0.0);
  std::fill(v1_.begin() + mon2_index_start, v1_.begin() + mon2_index_end, 0.0);
  std::fill(v2_.begin() + mon2_index_start, v2_.begin() + mon2_index_end, 0.0);
  std::fill(v3_.begin() + mon2_index_start, v3_.begin() + mon2_index_end, 0.0);
  std::fill(v4_.begin() + mon2_index_start, v4_.begin() + mon2_index_end, 0.0);
  std::fill(v5_.begin() + mon2_index_start, v5_.begin() + mon2_index_end, 0.0);
  std::fill(v6_.begin() + mon2_index_start, v6_.begin() + mon2_index_end, 0.0);
  std::fill(v7_.begin() + mon2_index_start, v7_.begin() + mon2_index_end, 0.0);
  std::fill(v8_.begin() + mon2_index_start, v8_.begin() + mon2_index_end, 0.0);
  std::fill(v9_.begin() + mon2_index_start, v9_.begin() + mon2_index_end, 0.0);
  std::fill(v10_.begin() + mon2_index_start, 
            v10_.begin() + mon2_index_end, 0.0);

  // Store rijx, rijy and rijz in vectors
#ifdef _OPENMP
  # pragma omp simd
#endif
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    v0_[m] = xyzmon1_x - xyz2[site_jnmon23 + m];                     // rijx
    v1_[m] = xyzmon1_y - xyz2[site_jnmon23 + nmon2 + m];             // rijy
    v2_[m] = xyzmon1_z - xyz2[site_jnmon23 + nmon22 + m];            // rijz
  }

  // Store r^2 in vector
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    v3_[m] = v0_[m]*v0_[m] + v1_[m]*v1_[m] + v2_[m]*v2_[m];  // rsq
  }
  
  // Store 1/r, a*(r/A)^4 in vector
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    v4_[m] = 1.0 / std::sqrt(v3_[m]);                        // 1/r
    
    v5_[m] = aCC * v3_[m] * v3_[m] / Asqsq;                  // a*(r/A)^4
  }
  
  // Compute gammq and store result in vector. This loop is not vectorizable
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    v6_[m] = gammq(0.75, v5_[m]);                            // gammq
  }

  // Finalize computation of electric field
#ifdef _OPENMP
  # pragma omp simd
#endif
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    const double exp1 = std::exp(-v5_[m]);

    // Screening functions
    const double s1r = v4_[m] - exp1 * v4_[m];
    const double s0r = s1r + aCC1_4 * Ai * g34 * v6_[m];
    const double s1r3 = s1r * v4_[m] * v4_[m];

    // Compute contribution to the field phi
    // Storing the contrib to mon 1 in vector to make it vectorizable

    // Assuming phi will be at1mon1_index at1m2 at1m3 .. for same type of mons
    // phi[finsts1 + site_inmon1 + mon1_index] 
    //                           += s0r *chg[finsts2 + site_jnmon2 + m];
    v7_[m] = s0r *chg2[site_jnmon2 + m];
    phi2[site_jnmon2 + m] += s0r *chg1[site_inmon1 + mon1_index];

    // Field will be as xyz xxxxyyyyzzzzat1 xxxxxyyyyzzzz at2...
    const double s1r3ci = s1r3 * chg1[site_inmon1 + mon1_index];
    const double s1r3cj = s1r3 * chg2[site_jnmon2 + m];

    // Compute the three components of the permanent electric field
    // Storing contributions to mon1 in vectors to make the loop vectorizable

    //Efq[fincrd1 + site_inmon13 + mon1_index] += s1r3cj * v0_[m];
    v8_[m] = s1r3cj * v0_[m];
    Efq2[site_jnmon23 + m] -= s1r3ci * v0_[m];

    //Efq[fincrd1 + site_inmon13 + nmon1 + mon1_index] += s1r3cj * v1_[m];
    v9_[m] = s1r3cj * v1_[m];
    Efq2[site_jnmon23 + nmon2 + m] -= s1r3ci * v1_[m];

    //Efq[fincrd1 + site_inmon13 + nmon12 + mon1_index] += s1r3cj * v2_[m];
    v10_[m] = s1r3cj * v2_[m];
    Efq2[site_jnmon23 + nmon22 + m] -= s1r3ci * v2_[m];
  }

  // Add up the contributions to the mon1 site
  *phi1 = 0.0;
  *Efqx_mon1 = 0.0;
  *Efqy_mon1 = 0.0;
  *Efqz_mon1 = 0.0;
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    *phi1 += v7_[m];
    *Efqx_mon1 += v8_[m];
    *Efqy_mon1 += v9_[m];
    *Efqz_mon1 += v10_[m];
  }
}

////////////////////////////////////////////////////////////////////////////////

void ElectricFieldHolder::CalcPermanentElecFieldWithPolfacZero
  (double * xyz1, double * xyz2,
   double * chg1, double * chg2,
   size_t mon1_index,
   size_t mon2_index_start, size_t mon2_index_end,
   size_t nmon1, size_t nmon2,
   size_t site_i, size_t site_j,
   double *Efqx_mon1, double *Efqy_mon1,
   double *Efqz_mon1, double *phi1,
   double * phi2,
   double * Efq2) {

  // Shifts that will be useful in the loops
  const size_t nmon12 = nmon1 * 2;
  const size_t nmon22 = nmon2 * 2;
  const size_t site_i3 = site_i * 3;
  const size_t site_j3 = site_j * 3;
  const size_t site_inmon1 = site_i * nmon1;
  const size_t site_jnmon2 = site_j * nmon2;
  const size_t site_inmon13 = nmon1 * site_i3;
  const size_t site_jnmon23 = nmon2 * site_j3;

  // Coordinates x, y and z of site i of monomer 1
  const double xyzmon1_x = xyz1[site_inmon13 + mon1_index];
  const double xyzmon1_y = xyz1[site_inmon13 + nmon1 + mon1_index];
  const double xyzmon1_z = xyz1[site_inmon13 + nmon12 + mon1_index];

  // Fill vectors with zeros in the desired range
  std::fill(v0_.begin() + mon2_index_start, v0_.begin() + mon2_index_end, 0.0);
  std::fill(v1_.begin() + mon2_index_start, v1_.begin() + mon2_index_end, 0.0);
  std::fill(v2_.begin() + mon2_index_start, v2_.begin() + mon2_index_end, 0.0);
  std::fill(v3_.begin() + mon2_index_start, v3_.begin() + mon2_index_end, 0.0);

#ifdef _OPENMP
  # pragma omp simd
#endif
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    // Obtain distances in x, y and z between sites i and j from mon1 and mon2
    const double rijx  = xyzmon1_x - xyz2[site_jnmon23 + m];          // rijx
    const double rijy  = xyzmon1_y - xyz2[site_jnmon23 + nmon2 + m];  // rijy
    const double rijz  = xyzmon1_z - xyz2[site_jnmon23 + nmon22 + m]; // rijz

    const double risq = rijx * rijx + rijy * rijy + rijz * rijz;  // rsq
    const double ri = 1.0 / std::sqrt(risq);                      // 1/r

    // Screening functions
    const double s0r = ri;
    const double s1r3 = ri * risq;

    // Assuming phi will be at1mon1_index at1m2 at1m3 .. for same type of mons
    // Compute contribution to the field phi
    // Storing the contrib to mon 1 in vector to make it vectorizable
    //phi[finsts1 + site_inmon1 + mon1_index] 
    //                         += s0r *chg[finsts2 + site_jnmon2 + m];
    v0_[m] = s0r *chg2[site_jnmon2 + m];
    phi2[site_jnmon2 + m] += s0r *chg1[site_inmon1 + mon1_index];

    const double s1r3ci = s1r3 * chg1[site_inmon1 + mon1_index];
    const double s1r3cj = s1r3 * chg2[site_jnmon2 + m];

    // Compute the three components of the permanent electric field
    // Storing contributions to mon1 in vectors to make the loop vectorizable
    // Field will be as xyz xxxxyyyyzzzzat1 xxxxxyyyyzzzz at2...

    //Efq[fincrd1 + site_inmon13 + mon1_index] += s1r3cj * rijx;
    v1_[m] = s1r3cj * rijx;
    Efq2[site_jnmon23 + m] -= s1r3ci * rijx;
    //Efq[fincrd1 + site_inmon13 + nmon1 + mon1_index] += s1r3cj * rijy;
    v2_[m] = s1r3cj * rijy;
    Efq2[site_jnmon23 + nmon2 + m] -= s1r3ci * rijy;
    //Efq[fincrd1 + site_inmon13 + nmon12 + mon1_index] += s1r3cj * rijz;
    v3_[m] = s1r3cj * rijz;
    Efq2[site_jnmon23 + nmon22 + m] -= s1r3ci * rijz;
  }

  // Adding up the contributions to mon 1
  *phi1 = 0.0;
  *Efqx_mon1 = 0.0;
  *Efqy_mon1 = 0.0;
  *Efqz_mon1 = 0.0;
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    *phi1 += v0_[m];
    *Efqx_mon1 += v1_[m];
    *Efqy_mon1 += v2_[m];
    *Efqz_mon1 += v3_[m];
  }
}

////////////////////////////////////////////////////////////////////////////////

void ElectricFieldHolder::CalcDipoleElecFieldWithPolfacNonZero
  (double * xyz1, double * xyz2,
   double * mu1, double * mu2,
   size_t mon1_index,
   size_t mon2_index_start, size_t mon2_index_end,
   size_t nmon1, size_t nmon2,
   size_t site_i, size_t site_j,
   double Asqsq,
   double aDD, double * Efd2,
   double *Efdx_mon1, double *Efdy_mon1, double *Efdz_mon1) {

  // Shifts that will be useful in the loops
  const size_t nmon12 = nmon1 * 2;
  const size_t nmon22 = nmon2 * 2;
  const size_t site_i3 = site_i * 3;
  const size_t site_j3 = site_j * 3;
  const size_t site_inmon13 = nmon1 * site_i3;
  const size_t site_jnmon23 = nmon2 * site_j3;

  // Coordinates x, y and z of site i of monomer 1
  const double xyzmon1_x = xyz1[site_inmon13 + mon1_index];
  const double xyzmon1_y = xyz1[site_inmon13 + nmon1 + mon1_index];
  const double xyzmon1_z = xyz1[site_inmon13 + nmon12 + mon1_index];

  // Fill vectors with zeros in the desired range
  std::fill(v0_.begin() + mon2_index_start, v0_.begin() + mon2_index_end, 0.0);
  std::fill(v1_.begin() + mon2_index_start, v1_.begin() + mon2_index_end, 0.0);
  std::fill(v2_.begin() + mon2_index_start, v2_.begin() + mon2_index_end, 0.0);

#ifdef _OPENMP
  # pragma omp simd
#endif
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    // Distances between sites i and j from mon1 and mon2
    const double rijx = xyzmon1_x - xyz2[site_jnmon23 + m];
    const double rijy = xyzmon1_y - xyz2[site_jnmon23 + nmon2 + m];
    const double rijz = xyzmon1_z - xyz2[site_jnmon23 + nmon22 + m];

    const double rsq = rijx*rijx + rijy*rijy + rijz*rijz;
    const double r = std::sqrt(rsq);
    const double ri = 1.0/r;
    const double risq = ri*ri;
    const double rsqsq = rsq * rsq;

    // Some values that will be used in the screening functions
    const double rA4 = rsqsq/Asqsq;
    const double arA4 = aDD*rA4;
    // TODO look at the exponential function intel vec
    const double exp1 = std::exp(-arA4);

    // Get screening functions
    const double s1r = ri - exp1*ri;
    const double s1r3 = s1r * risq;
    const double s2r5_3 = (3.0*s1r3
                          -4.0*aDD*rA4*exp1*risq*ri)*risq;
    const double ts2x = s2r5_3 * rijx;
    const double ts2y = s2r5_3 * rijy;
    const double ts2z = s2r5_3 * rijz;

    // Contributions to the dipole electric field to site i of mon1
    // Stored in vectors to make the loop vectorizable

    // Component x
    //Efd[fincrd1 + site_inmon13 + mon1_index] += 
    v0_[m] = 
              ((ts2x * rijx - s1r3) * mu2[site_jnmon23 + m] +
                ts2x * rijy * mu2[site_jnmon23 + nmon2 + m] +
                ts2x * rijz * mu2[site_jnmon23 + nmon22 + m]);

    // Component y
    //Efd[fincrd1 + site_inmon13 + nmon1 + mon1_index] += 
    v1_[m] = 
              ( ts2y * rijx * mu2[site_jnmon23 + m] +
               (ts2y * rijy - s1r3) * mu2[site_jnmon23 + nmon2 + m] +
                ts2y * rijz * mu2[site_jnmon23 + nmon22 + m]);

    // Component z
    //Efd[fincrd1 + site_inmon13 + nmon12 + mon1_index] +=
    v2_[m] =
              ( ts2z * rijx * mu2[site_jnmon23 + m] +
                ts2z * rijy * mu2[site_jnmon23 + nmon2 + m] +
               (ts2z * rijz - s1r3) * mu2[site_jnmon23 + nmon22 + m]);

    // Contributions to the dipole electric field to site j of mon2
    // Component x
    Efd2[site_jnmon23 + m] +=
              ((ts2x * rijx - s1r3) * mu1[site_inmon13 + mon1_index] +
               ts2x * rijy * mu1[site_inmon13 + nmon1 + mon1_index] +
               ts2x * rijz * mu1[site_inmon13 + nmon12 + mon1_index]);

    // Component y
    Efd2[site_jnmon23 + nmon2 + m] +=
              ((ts2y * rijx) * mu1[site_inmon13 + mon1_index] +
               (ts2y * rijy - s1r3) * mu1[site_inmon13 + nmon1 + mon1_index] +
               ts2y * rijz * mu1[site_inmon13 + nmon12 + mon1_index]);

    // Component z
    Efd2[site_jnmon23 + nmon22 + m] +=
              ((ts2z * rijx) * mu1[site_inmon13 + mon1_index] +
               ts2z * rijy * mu1[site_inmon13 + nmon1 + mon1_index] +
               (ts2z * rijz - s1r3) * mu1[site_inmon13 + nmon12 + mon1_index]);

  }

  // Setting the values to the output
  *Efdx_mon1 = 0.0;
  *Efdy_mon1 = 0.0;
  *Efdz_mon1 = 0.0;
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    *Efdx_mon1 += v0_[m];
    *Efdy_mon1 += v1_[m];
    *Efdz_mon1 += v2_[m];
  }
}

////////////////////////////////////////////////////////////////////////////////

void ElectricFieldHolder::CalcDipoleElecFieldWithPolfacZero
  (double * xyz1, double * xyz2,
   double * mu1, double * mu2,
   size_t mon1_index,
   size_t mon2_index_start, size_t mon2_index_end,
   size_t nmon1, size_t nmon2,
   size_t site_i, size_t site_j, double * Efd2,
   double *Efdx_mon1, double *Efdy_mon1, double *Efdz_mon1) {

  // Shifts that will be useful in the loops
  const size_t nmon12 = nmon1 * 2;
  const size_t nmon22 = nmon2 * 2;
  const size_t site_i3 = site_i * 3;
  const size_t site_j3 = site_j * 3;
  const size_t site_inmon13 = nmon1 * site_i3;
  const size_t site_jnmon23 = nmon2 * site_j3;

  // Coordinates x, y and z of site i of monomer 1
  const double xyzmon1_x = xyz1[site_inmon13 + mon1_index];
  const double xyzmon1_y = xyz1[site_inmon13 + nmon1 + mon1_index];
  const double xyzmon1_z = xyz1[site_inmon13 + nmon12 + mon1_index];

  // Fill vectors with zeros in the desired range
  std::fill(v0_.begin() + mon2_index_start, v0_.begin() + mon2_index_end, 0.0);
  std::fill(v1_.begin() + mon2_index_start, v1_.begin() + mon2_index_end, 0.0);
  std::fill(v2_.begin() + mon2_index_start, v2_.begin() + mon2_index_end, 0.0);

#ifdef _OPENMP
  # pragma omp simd
#endif
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    // Distances between sites i and j from mon1 and mon2
    const double rijx = xyzmon1_x - xyz2[site_jnmon23 + m];
    const double rijy = xyzmon1_y - xyz2[site_jnmon23 + nmon2 + m];
    const double rijz = xyzmon1_z - xyz2[site_jnmon23 + nmon22 + m];

    const double rsq = rijx*rijx + rijy*rijy + rijz*rijz;
    const double r = std::sqrt(rsq);
    const double ri = 1.0/r;
    const double risq = ri*ri;

    // Some values that will be used in the screening functions

    // Get screening functions
    const double s1r = ri;
    const double s1r3 = s1r * risq;
    const double s2r5_3 = 3.0*s1r3*risq;
                          
    // Get tensors
    const double ts2x = s2r5_3 * rijx;
    const double ts2y = s2r5_3 * rijy;
    const double ts2z = s2r5_3 * rijz;

    // Contributions to the dipole electric field to site i of mon1
    // Stored in vectors to make the loop vectorizable

    // Component x
    //Efd[fincrd1 + site_inmon13 + mon1_index] += 
    v0_[m] =
              ((ts2x * rijx - s1r3) * mu2[site_jnmon23 + m] +
                ts2x * rijy * mu2[site_jnmon23 + nmon2 + m] +
                ts2x * rijz * mu2[site_jnmon23 + nmon22 + m]);

    // Component y
    //Efd[fincrd1 + site_inmon13 + nmon1 + mon1_index] += 
    v1_[m] =
              ( ts2y * rijx * mu2[site_jnmon23 + m] +
               (ts2y * rijy - s1r3) * mu2[site_jnmon23 + nmon2 + m] +
                ts2y * rijz * mu2[site_jnmon23 + nmon22 + m]);

    // Component z
    //Efd[fincrd1 + site_inmon13 + nmon12 + mon1_index] +=
    v2_[m] =
              ( ts2z * rijx * mu2[site_jnmon23 + m] +
                ts2z * rijy * mu2[site_jnmon23 + nmon2 + m] +
               (ts2z * rijz - s1r3) * mu2[site_jnmon23 + nmon22 + m]);
    
    // Contributions to the dipole electric field to site j of mon2
    // Component x
    Efd2[site_jnmon23 + m] +=
              ((ts2x * rijx - s1r3) * mu1[site_inmon13 + mon1_index] +
               ts2x * rijy * mu1[site_inmon13 + nmon1 + mon1_index] +
               ts2x * rijz * mu1[site_inmon13 + nmon12 + mon1_index]);

    // Component y
    Efd2[site_jnmon23 + nmon2 + m] +=
              ((ts2y * rijx) * mu1[site_inmon13 + mon1_index] +
               (ts2y * rijy - s1r3) * mu1[site_inmon13 + nmon1 + mon1_index] +
               ts2y * rijz * mu1[site_inmon13 + nmon12 + mon1_index]);

    // Component z
    Efd2[site_jnmon23 + nmon22 + m] +=
              ((ts2z * rijx) * mu1[site_inmon13 + mon1_index] +
               ts2z * rijy * mu1[site_inmon13 + nmon1 + mon1_index] +
               (ts2z * rijz - s1r3) * mu1[site_inmon13 + nmon12 + mon1_index]);
  }

  // Setting the values to the output
  *Efdx_mon1 = 0.0;
  *Efdy_mon1 = 0.0;
  *Efdz_mon1 = 0.0;
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    *Efdx_mon1 += v0_[m];
    *Efdy_mon1 += v1_[m];
    *Efdz_mon1 += v2_[m];
  }
  
}

////////////////////////////////////////////////////////////////////////////////

void ElectricFieldHolder::CalcElecFieldGradsWithPolfacNonZero
  (double * xyz1, double * xyz2,
   double * chg1, double * chg2,
   double * mu1, double * mu2,
   size_t mon1_index,
   size_t mon2_index_start, size_t mon2_index_end,
   size_t nmon1, size_t nmon2,
   size_t site_i, size_t site_j, 
   double aDD, double aCD, double Asqsq,
   double *grdx, double *grdy, double *grdz,
   double *phi1, double * phi2,
   double * grd2) {

  // Shifts that will be useful in the loops
  const size_t nmon12 = nmon1 * 2;
  const size_t nmon22 = nmon2 * 2;
  const size_t site_i3 = site_i * 3;
  const size_t site_j3 = site_j * 3;
  const size_t site_inmon1 = site_i * nmon1;
  const size_t site_jnmon2 = site_j * nmon2;
  const size_t site_inmon13 = nmon1 * site_i3;
  const size_t site_jnmon23 = nmon2 * site_j3;

  // Coordinates x, y and z of site i of monomer 1
  const double xyzmon1_x = xyz1[site_inmon13 + mon1_index];
  const double xyzmon1_y = xyz1[site_inmon13 + nmon1 + mon1_index];
  const double xyzmon1_z = xyz1[site_inmon13 + nmon12 + mon1_index];

  // Fill vectors with zeros in the desired range
  std::fill(v0_.begin() + mon2_index_start, v0_.begin() + mon2_index_end, 0.0);
  std::fill(v1_.begin() + mon2_index_start, v1_.begin() + mon2_index_end, 0.0);
  std::fill(v2_.begin() + mon2_index_start, v2_.begin() + mon2_index_end, 0.0);
  std::fill(v3_.begin() + mon2_index_start, v3_.begin() + mon2_index_end, 0.0);

#ifdef _OPENMP
  # pragma omp simd
#endif
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    // Distances between sites i and j from mon1 and mon2
    const double rijx = xyzmon1_x - xyz2[site_jnmon23 + m];
    const double rijy = xyzmon1_y - xyz2[site_jnmon23 + nmon2 + m];
    const double rijz = xyzmon1_z - xyz2[site_jnmon23 + nmon22 + m];

    const double rijx2 = rijx*rijx;
    const double rijy2 = rijy*rijy;
    const double rijz2 = rijz*rijz;
    const double rsq = rijx2 + rijy2 + rijz2;
    const double r = std::sqrt(rsq);
    const double ri = 1.0/r;
    const double risq = ri*ri;
    const double rsqsq = rsq * rsq;
    // Some values that will be used in the screening functions
    const double rA4 = rsqsq/Asqsq;
    // TODO look at the exponential function intel vec

    const double adrA4 = aDD*4.0*rA4;
    const double acrA4 = aCD*4.0*rA4;
    const double exp1d = std::exp(-aDD*rA4);
    const double exp1c = std::exp(-aCD*rA4);

    // Get screening functions
    const double s1rd = ri - exp1d*ri;
    const double s1r3d = s1rd * risq;
    const double s2r5_3d = (3.0*s1r3d
                           -adrA4*exp1d*risq*ri)*risq;

    const double s3r7_15d = (s2r5_3d * 5.0 - adrA4 * exp1d *
            (adrA4 - 1.0) * risq * risq * ri) * risq;
    const double s3r7_15x2 = s3r7_15d * rijx2;
    const double s3r7_15y2 = s3r7_15d * rijy2;
    const double s3r7_15z2 = s3r7_15d * rijz2;

    const double s1rc = ri - exp1c*ri;
    const double s1r3c = s1rc * risq;
    const double rxs1r3c = rijx * s1r3c;
    const double rys1r3c = rijy * s1r3c;
    const double rzs1r3c = rijz * s1r3c;
    const double s2r5_3c = (3.0*s1r3c
                           -acrA4*exp1c*risq*ri)*risq;

    // Tensors
    const double ts2x = s2r5_3c * rijx;
    const double ts2y = s2r5_3c * rijy;
    const double ts2z = s2r5_3c * rijz;

    // T_alpha_beta_gamma tensor
    const double t3_0 = s3r7_15x2* rijx
                      - s2r5_3d * 3.0 * rijx;          // x x x
    const double t3_1 = s3r7_15x2 * rijy
                      - s2r5_3d * rijy;                // x x y
    const double t3_2 = s3r7_15x2 * rijz
                      - s2r5_3d * rijz;                // x x z
    const double t3_3 = s3r7_15y2 * rijx
                      - s2r5_3d * rijx;                // x y y
    const double t3_4 = s3r7_15d * rijx * rijy * rijz; // x y z
    const double t3_5 = s3r7_15z2 * rijx
                      - s2r5_3d * rijx;                // x z z
    const double t3_6 = s3r7_15d * rijy2 * rijy
                      - s2r5_3d * 3.0 * rijy;          // y y y
    const double t3_7 = s3r7_15y2 * rijz
                      - s2r5_3d * rijz;                // y y z
    const double t3_8 = s3r7_15z2 * rijy
                      - s2r5_3d * rijy;                // y z z
    const double t3_9 = s3r7_15z2 * rijz
                      - s2r5_3d * 3.0 * rijz;          // z z z

    // T_alpha_beta tensor
    const double t2_0 = ts2x * rijx - s1r3c; // alpha = x, beta = x
    const double t2_1 = ts2x * rijy;
    const double t2_2 = ts2x * rijz;
    const double t2_3 = ts2y * rijy - s1r3c;
    const double t2_4 = ts2y * rijz;
    const double t2_5 = ts2z * rijz - s1r3c;

    // Charge times the dipole component
    const double ci_mjx = chg1[site_inmon1 + mon1_index] 
                        * mu2[site_jnmon23 + m];
    const double cj_mix = chg2[site_jnmon2 + m] 
                        * mu1[site_inmon13 + mon1_index];
    const double ci_mjy = chg1[site_inmon1 + mon1_index] 
                        * mu2[site_jnmon23 + nmon2 + m];
    const double cj_miy = chg2[site_jnmon2 + m] 
                        * mu1[site_inmon13 + nmon1 + mon1_index];
    const double ci_mjz = chg1[site_inmon1 + mon1_index] 
                        * mu2[site_jnmon23 + nmon22 + m];
    const double cj_miz = chg2[site_jnmon2 + m] 
                        * mu1[site_inmon13 + nmon12 + mon1_index];

    // Dipole times dipole
    const double mu1xmu2x = mu1[site_inmon13 + mon1_index] 
                          * mu2[site_jnmon23 + m];
    const double mu1xmu2y = mu1[site_inmon13 + mon1_index] 
                          * mu2[site_jnmon23 + nmon2 + m];
    const double mu1xmu2z = mu1[site_inmon13 + mon1_index] 
                          * mu2[site_jnmon23 + nmon22 + m];
    const double mu1ymu2x = mu1[site_inmon13 + nmon1 + mon1_index] 
                          * mu2[site_jnmon23 + m];
    const double mu1ymu2y = mu1[site_inmon13 + nmon1 + mon1_index] 
                          * mu2[site_jnmon23 + nmon2 + m];
    const double mu1ymu2z = mu1[site_inmon13 + nmon1 + mon1_index] 
                          * mu2[site_jnmon23 + nmon22 + m];
    const double mu1zmu2x = mu1[site_inmon13 + nmon12 + mon1_index] 
                          * mu2[site_jnmon23 + m];
    const double mu1zmu2y = mu1[site_inmon13 + nmon12 + mon1_index] 
                          * mu2[site_jnmon23 + nmon2 + m];
    const double mu1zmu2z = mu1[site_inmon13 + nmon12 + mon1_index] 
                          * mu2[site_jnmon23 + nmon22 + m];

    if (phi2 != 0) {
    // Gradients Charge-Dipole
    // Site site_i
    // Note. Sign changed
    v0_[m] = (cj_mix - ci_mjx ) * t2_0            // x x 
           + (cj_miy - ci_mjy ) * t2_1            // x y 
           + (cj_miz - ci_mjz ) * t2_2;           // x z 
    v1_[m] = (cj_mix - ci_mjx ) * t2_1            // y x 
           + (cj_miy - ci_mjy ) * t2_3            // y y 
           + (cj_miz - ci_mjz ) * t2_4;           // y z 
    v2_[m] = (cj_mix - ci_mjx ) * t2_2            // z x 
           + (cj_miy - ci_mjy ) * t2_4            // z y 
           + (cj_miz - ci_mjz ) * t2_5;           // z z 

    // Site site_j
    grd2[site_jnmon23 + m] -= v0_[m];
    grd2[site_jnmon23 + nmon2 + m] -= v1_[m];
    grd2[site_jnmon23 + nmon22 + m] -= v2_[m];
    
    // Update field
    // Site site_i
    v3_[m] = rxs1r3c * mu2[site_jnmon23 + m] 
           + rys1r3c * mu2[site_jnmon23 + nmon2 + m]
           + rzs1r3c * mu2[site_jnmon23 + nmon22 + m];

    // Site site_j
    phi2[site_jnmon2 + m] -= (rxs1r3c * mu1[site_inmon13 + mon1_index] 
                      + rys1r3c * mu1[site_inmon13 + nmon1 + mon1_index]
                      + rzs1r3c * mu1[site_inmon13 + nmon12 + mon1_index]);
    }
    // Gradients Dipole-Dipole
    const double gx = mu1xmu2x * t3_0            // x x x 
                    + mu1xmu2y * t3_1            // x x y 
                    + mu1xmu2z * t3_2            // x x z 
                    + mu1ymu2x * t3_1            // x y x 
                    + mu1ymu2y * t3_3            // x y y 
                    + mu1ymu2z * t3_4            // x y z 
                    + mu1zmu2x * t3_2            // x z x 
                    + mu1zmu2y * t3_4            // x z y 
                    + mu1zmu2z * t3_5;           // x z z 

    const double gy = mu1xmu2x * t3_1            // y x x 
                    + mu1xmu2y * t3_3            // y x y 
                    + mu1xmu2z * t3_4            // y x z 
                    + mu1ymu2x * t3_3            // y y x 
                    + mu1ymu2y * t3_6            // y y y 
                    + mu1ymu2z * t3_7            // y y z 
                    + mu1zmu2x * t3_4            // y z x 
                    + mu1zmu2y * t3_7            // y z y 
                    + mu1zmu2z * t3_8;           // y z z

    const double gz = mu1xmu2x * t3_2            // z x x 
                    + mu1xmu2y * t3_4            // z x y 
                    + mu1xmu2z * t3_5            // z x z 
                    + mu1ymu2x * t3_4            // z y x 
                    + mu1ymu2y * t3_7            // z y y 
                    + mu1ymu2z * t3_8            // z y z 
                    + mu1zmu2x * t3_5            // z z x 
                    + mu1zmu2y * t3_8            // z z y 
                    + mu1zmu2z * t3_9;           // z z z
    // Site site_i
    v0_[m] += gx;
    v1_[m] += gy;
    v2_[m] += gz;

    // Site site_j
    grd2[site_jnmon23 + m] -= gx;
    grd2[site_jnmon23 + nmon2 + m] -= gy;
    grd2[site_jnmon23 + nmon22 + m] -= gz;


  }

  // Compress vectors to double
  *grdx = 0.0;
  *grdy = 0.0;
  *grdz = 0.0;
  *phi1 = 0.0;
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    *grdx += v0_[m];
    *grdy += v1_[m];
    *grdz += v2_[m];
    *phi1 += v3_[m];
  }
}

////////////////////////////////////////////////////////////////////////////////

void ElectricFieldHolder::CalcElecFieldGradsWithPolfacZero
  (double * xyz1, double * xyz2,
   double * chg1, double * chg2,
   double * mu1, double * mu2,
   size_t mon1_index,
   size_t mon2_index_start, size_t mon2_index_end,
   size_t nmon1, size_t nmon2,
   size_t site_i, size_t site_j,
   double *grdx, double *grdy, double *grdz,
   double *phi1, double * phi2,
   double * grd2) {

  // Shifts that will be useful in the loops
  const size_t nmon12 = nmon1 * 2;
  const size_t nmon22 = nmon2 * 2;
  const size_t site_i3 = site_i * 3;
  const size_t site_j3 = site_j * 3;
  const size_t site_inmon1 = site_i * nmon1;
  const size_t site_jnmon2 = site_j * nmon2;
  const size_t site_inmon13 = nmon1 * site_i3;
  const size_t site_jnmon23 = nmon2 * site_j3;

  // Coordinates x, y and z of site i of monomer 1
  const double xyzmon1_x = xyz1[site_inmon13 + mon1_index];
  const double xyzmon1_y = xyz1[site_inmon13 + nmon1 + mon1_index];
  const double xyzmon1_z = xyz1[site_inmon13 + nmon12 + mon1_index];

  // Fill vectors with zeros in the desired range
  std::fill(v0_.begin() + mon2_index_start, v0_.begin() + mon2_index_end, 0.0);
  std::fill(v1_.begin() + mon2_index_start, v1_.begin() + mon2_index_end, 0.0);
  std::fill(v2_.begin() + mon2_index_start, v2_.begin() + mon2_index_end, 0.0);
  std::fill(v3_.begin() + mon2_index_start, v3_.begin() + mon2_index_end, 0.0);

#ifdef _OPENMP
  # pragma omp simd
#endif
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    // Distances between sites i and j from mon1 and mon2
    const double rijx = xyzmon1_x - xyz2[site_jnmon23 + m];
    const double rijy = xyzmon1_y - xyz2[site_jnmon23 + nmon2 + m];
    const double rijz = xyzmon1_z - xyz2[site_jnmon23 + nmon22 + m];

    const double rijx2 = rijx*rijx;
    const double rijy2 = rijy*rijy;
    const double rijz2 = rijz*rijz;
    const double rsq = rijx2 + rijy2 + rijz2;
    const double r = std::sqrt(rsq);
    const double ri = 1.0/r;
    const double risq = ri*ri;
    // Some values that will be used in the screening functions

    // Get screening functions
    const double s1r = ri;
    const double s1r3 = s1r * risq;
    const double s2r5_3 = 3.0 * s1r3 * risq;

    const double s3r7_15 = s2r5_3 * 5.0 * risq;
    const double s3r7_15x2 = s3r7_15 * rijx2;
    const double s3r7_15y2 = s3r7_15 * rijy2;
    const double s3r7_15z2 = s3r7_15 * rijz2;

    const double rxs1r3c = rijx * s1r3;
    const double rys1r3c = rijy * s1r3;
    const double rzs1r3c = rijz * s1r3;
    const double ts2x = s2r5_3 * rijx;
    const double ts2y = s2r5_3 * rijy;
    const double ts2z = s2r5_3 * rijz;

    // T_alpha_beta_gamma tensor
    const double t3_0 = s3r7_15x2* rijx
                      - s2r5_3 * 3.0 * rijx;          // x x x
    const double t3_1 = s3r7_15x2 * rijy
                      - s2r5_3 * rijy;                // x x y
    const double t3_2 = s3r7_15x2 * rijz
                      - s2r5_3 * rijz;                // x x z
    const double t3_3 = s3r7_15y2 * rijx
                      - s2r5_3 * rijx;                // x y y
    const double t3_4 = s3r7_15 * rijx * rijy * rijz; // x y z
    const double t3_5 = s3r7_15z2 * rijx
                      - s2r5_3 * rijx;                // x z z
    const double t3_6 = s3r7_15 * rijy2 * rijy
                      - s2r5_3 * 3.0 * rijy;          // y y y
    const double t3_7 = s3r7_15y2 * rijz
                      - s2r5_3 * rijz;                // y y z
    const double t3_8 = s3r7_15z2 * rijy
                      - s2r5_3 * rijy;                // y z z
    const double t3_9 = s3r7_15z2 * rijz
                      - s2r5_3 * 3.0 * rijz;          // z z z

    // T_alpha_beta tensor
    const double t2_0 = ts2x * rijx - s1r3; // alpha = x, beta = x
    const double t2_1 = ts2x * rijy;
    const double t2_2 = ts2x * rijz;
    const double t2_3 = ts2y * rijy - s1r3;
    const double t2_4 = ts2y * rijz;
    const double t2_5 = ts2z * rijz - s1r3;

    // Charge times the dipole component
    const double ci_mjx = chg1[site_inmon1 + mon1_index] 
                        * mu2[site_jnmon23 + m];
    const double cj_mix = chg2[site_jnmon2 + m] 
                        * mu1[site_inmon13 + mon1_index];
    const double ci_mjy = chg1[site_inmon1 + mon1_index] 
                        * mu2[site_jnmon23 + nmon2 + m];
    const double cj_miy = chg2[site_jnmon2 + m] 
                        * mu1[site_inmon13 + nmon1 + mon1_index];
    const double ci_mjz = chg1[site_inmon1 + mon1_index] 
                        * mu2[site_jnmon23 + nmon22 + m];
    const double cj_miz = chg2[site_jnmon2 + m] 
                        * mu1[site_inmon13 + nmon12 + mon1_index];

    // Dipole times dipole
    const double mu1xmu2x = mu1[site_inmon13 + mon1_index] 
                          * mu2[site_jnmon23 + m];
    const double mu1xmu2y = mu1[site_inmon13 + mon1_index] 
                          * mu2[site_jnmon23 + nmon2 + m];
    const double mu1xmu2z = mu1[site_inmon13 + mon1_index] 
                          * mu2[site_jnmon23 + nmon22 + m];
    const double mu1ymu2x = mu1[site_inmon13 + nmon1 + mon1_index] 
                          * mu2[site_jnmon23 + m];
    const double mu1ymu2y = mu1[site_inmon13 + nmon1 + mon1_index] 
                          * mu2[site_jnmon23 + nmon2 + m];
    const double mu1ymu2z = mu1[site_inmon13 + nmon1 + mon1_index] 
                          * mu2[site_jnmon23 + nmon22 + m];
    const double mu1zmu2x = mu1[site_inmon13 + nmon12 + mon1_index] 
                          * mu2[site_jnmon23 + m];
    const double mu1zmu2y = mu1[site_inmon13 + nmon12 + mon1_index] 
                          * mu2[site_jnmon23 + nmon2 + m];
    const double mu1zmu2z = mu1[site_inmon13 + nmon12 + mon1_index] 
                          * mu2[site_jnmon23 + nmon22 + m];
    
    if (phi2 != 0) {
    // Gradients Charge-Dipole
    // Site site_i
    // Note. Sign changed
    v0_[m] = (cj_mix - ci_mjx ) * t2_0            // x x 
           + (cj_miy - ci_mjy ) * t2_1            // x y 
           + (cj_miz - ci_mjz ) * t2_2;           // x z 
    v1_[m] = (cj_mix - ci_mjx ) * t2_1            // y x 
           + (cj_miy - ci_mjy ) * t2_3            // y y 
           + (cj_miz - ci_mjz ) * t2_4;           // y z 
    v2_[m] = (cj_mix - ci_mjx ) * t2_2            // z x 
           + (cj_miy - ci_mjy ) * t2_4            // z y 
           + (cj_miz - ci_mjz ) * t2_5;           // z z 

    // Site site_j
    grd2[site_jnmon23 + m] -= v0_[m];
    grd2[site_jnmon23 + nmon2 + m] -= v1_[m];
    grd2[site_jnmon23 + nmon22 + m] -= v2_[m];

    // Update field
    // Site site_i
    v3_[m] = rxs1r3c * mu2[site_jnmon23 + m]
           + rys1r3c * mu2[site_jnmon23 + nmon2 + m]
           + rzs1r3c * mu2[site_jnmon23 + nmon22 + m];

    // Site site_j
    phi2[site_jnmon2 + m] -= (rxs1r3c * mu1[site_inmon13 + mon1_index]
                      + rys1r3c * mu1[site_inmon13 + nmon1 + mon1_index]
                      + rzs1r3c * mu1[site_inmon13 + nmon12 + mon1_index]);
    }
    // Gradients Dipole-Dipole
    const double gx = mu1xmu2x * t3_0            // x x x 
                    + mu1xmu2y * t3_1            // x x y 
                    + mu1xmu2z * t3_2            // x x z 
                    + mu1ymu2x * t3_1            // x y x 
                    + mu1ymu2y * t3_3            // x y y 
                    + mu1ymu2z * t3_4            // x y z 
                    + mu1zmu2x * t3_2            // x z x 
                    + mu1zmu2y * t3_4            // x z y 
                    + mu1zmu2z * t3_5;           // x z z 

    const double gy = mu1xmu2x * t3_1            // y x x 
                    + mu1xmu2y * t3_3            // y x y 
                    + mu1xmu2z * t3_4            // y x z 
                    + mu1ymu2x * t3_3            // y y x 
                    + mu1ymu2y * t3_6            // y y y 
                    + mu1ymu2z * t3_7            // y y z 
                    + mu1zmu2x * t3_4            // y z x 
                    + mu1zmu2y * t3_7            // y z y 
                    + mu1zmu2z * t3_8;           // y z z

    const double gz = mu1xmu2x * t3_2            // z x x 
                    + mu1xmu2y * t3_4            // z x y 
                    + mu1xmu2z * t3_5            // z x z 
                    + mu1ymu2x * t3_4            // z y x 
                    + mu1ymu2y * t3_7            // z y y 
                    + mu1ymu2z * t3_8            // z y z 
                    + mu1zmu2x * t3_5            // z z x 
                    + mu1zmu2y * t3_8            // z z y 
                    + mu1zmu2z * t3_9;           // z z z
    // Site site_i
    v0_[m] += gx;
    v1_[m] += gy;
    v2_[m] += gz;

    // Site site_j
    grd2[site_jnmon23 + m] -= gx;
    grd2[site_jnmon23 + nmon2 + m] -= gy;
    grd2[site_jnmon23 + nmon22 + m] -= gz;


  }

  // Compress vectors to double
  *grdx = 0.0;
  *grdy = 0.0;
  *grdz = 0.0;
  *phi1 = 0.0;
  for (size_t m = mon2_index_start; m < mon2_index_end; m++) {
    *grdx += v0_[m];
    *grdy += v1_[m];
    *grdz += v2_[m];
    *phi1 += v3_[m];
  }

}

////////////////////////////////////////////////////////////////////////////////

} // namespace elec

////////////////////////////////////////////////////////////////////////////////
