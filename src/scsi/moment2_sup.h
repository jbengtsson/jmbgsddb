#ifndef MOMENT2_SUP_H
#define MOMENT2_SUP_H

#include <vector>
#include "base.h"
#include "moment2.h"

void GetCenofChg(const Config &conf, const Moment2State &ST,
                 Moment2State::vector_t &CenofChg, Moment2State::vector_t &BeamRMS);

void GetEdgeMatrix(const double rho, const double phi, typename Moment2ElementBase::value_t &M);

void GetQuadMatrix(const double L, const double K, const unsigned ind, typename Moment2ElementBase::value_t &M);

void GetSolMatrix(const double L, const double K, typename Moment2ElementBase::value_t &M);

#endif // MOMENT2_SUP_H
