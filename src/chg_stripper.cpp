#include "flame/constants.h"
#include "flame/moment.h"
#include "flame/moment_sup.h"
#include "flame/chg_stripper.h"

#define sqr(x)  ((x)*(x))
#define cube(x) ((x)*(x)*(x))

void GetCenofChg(const Config &conf, const MomentState &ST,
                 MomentState::vector_t &CenofChg, MomentState::vector_t &BeamRMS)
{
    size_t i, j;
    const size_t n = ST.real.size(); // # of states

    CenofChg = boost::numeric::ublas::zero_vector<double>(PS_Dim);
    BeamRMS.resize(PS_Dim);

    double Ntot = 0e0;
    for (i = 0; i < n; i++) {
        CenofChg += ST.moment0[i]*ST.real[i].IonQ;
        Ntot += ST.real[i].IonQ;
    }

    CenofChg /= Ntot;

    for (j = 0; j < MomentState::maxsize; j++) {
        double BeamVar = 0.0;
        for (i = 0; i < n; i++) {
            const double temp = ST.moment0[i][j]-CenofChg[j];
            BeamVar  += ST.real[i].IonQ*(ST.moment1[i](j, j)
                                         + sqr(temp));
        }
        BeamRMS[j] = sqrt(BeamVar/Ntot);
    }
}


static
double Gaussian(double in, const double Q_ave, const double d)
{
    // Gaussian distribution.
    return 1e0/sqrt(2e0*M_PI)/d*exp(-0.5e0*sqr(in-Q_ave)/sqr(d));
}


static
void StripperCharge(const double beta, double &Q_ave, double &d)
{
    // Baron's formula for carbon foil.
    double Q_ave1, Y;

    Q_ave1 = Stripper_IonProton*(1e0-exp(-83.275*(beta/pow(Stripper_IonProton, 0.447))));
    Q_ave  = Q_ave1*(1e0-exp(-12.905+0.2124*Stripper_IonProton-0.00122*sqr(Stripper_IonProton)));
    Y      = Q_ave1/Stripper_IonProton;
    d      = sqrt(Q_ave1*(0.07535+0.19*Y-0.2654*sqr(Y)));
}


static
void ChargeStripper(const double beta, const std::vector<double> ChgState, std::vector<double>& chargeAmount_Baron)
{
    unsigned    k;
    double Q_ave, d;

    StripperCharge(beta, Q_ave, d);
    for (k = 0; k < ChgState.size(); k++)
        chargeAmount_Baron.push_back(Gaussian(ChgState[k]*Stripper_IonMass, Q_ave, d));
}


static
void Stripper_Propagate_ref(const Config &conf, Particle &ref, const std::vector<double> ChgState)
{
    const int n = ChgState.size();

    // Change reference particle charge state.
    ref.IonZ = Stripper_IonZ;

//    ChargeStripper(ref.beta, ChgState, chargeAmount_Baron);

    // Evaluate change in reference particle energy due to stripper model energy straggling.
    ref.IonEk      = (ref.IonEk-Stripper_Para[2])*Stripper_E0Para[1] + Stripper_E0Para[0];

    ref.IonW       = ref.IonEk + ref.IonEs;
    ref.gamma      = ref.IonW/ref.IonEs;
    ref.beta       = sqrt(1e0-1e0/sqr(ref.gamma));
    ref.SampleIonK = 2e0*M_PI/(ref.beta*SampleLambda);
}

void Stripper_GetMat(const Config &conf,
                     MomentState &ST)
{
    unsigned               k, n;
    double                 tmptotCharge, Fy_abs_recomb, Ek_recomb, stdEkFoilVariation, ZpAfStr, growthRate;
    double                 stdXYp, XpAfStr, growthRateXp, YpAfStr, growthRateYp, s;
    Particle               ref;
    state_t                *StatePtr = &ST;
    MomentState::matrix_t  tmpmat;
    std::vector<double>    chargeAmount_Baron;


    // Evaluate beam parameter recombination.

    //std::cout<<"In "<<__FUNCTION__<<"\n";

    tmptotCharge  = 0e0;
    Fy_abs_recomb = 0e0;
    Ek_recomb     = 0e0;
    tmpmat        = boost::numeric::ublas::zero_matrix<double>(PS_Dim);
    for (k = 0; k < ST.size(); k++) {
        const double Q = ST.real[k].IonQ;
        tmptotCharge  += Q;
        Fy_abs_recomb += Q*StatePtr->real[k].phis;
        Ek_recomb     += Q*StatePtr->real[k].IonEk;
        tmpmat        +=
                Q*(StatePtr->moment1[k]+outer_prod(StatePtr->moment0[k]-ST.moment0_env, StatePtr->moment0[k]-ST.moment0_env));
    }

    Fy_abs_recomb /= tmptotCharge;
    Ek_recomb     /= tmptotCharge;

    // Stripper model energy straggling.
    Ek_recomb      = (Ek_recomb-Stripper_Para[2])*Stripper_E0Para[1] + Stripper_E0Para[0];
    tmpmat        /= tmptotCharge;

    // Estimate Zp stripper caused envelope increase.
    stdEkFoilVariation = sqrt(1e0/3e0)*Stripper_Para[1]/100e0*Stripper_Para[0]*Stripper_E0Para[2];
    ZpAfStr            = tmpmat(5, 5) + sqr(Stripper_E1Para) + sqr(stdEkFoilVariation);
    growthRate         = sqrt(ZpAfStr/tmpmat(5, 5));

    for (k = 0; k < 6; k++) {
        tmpmat(k, 5) = tmpmat(k, 5)*growthRate;
        // This trick allows two times growthRate for <Zp, Zp>.
        tmpmat(5, k) = tmpmat(5, k)*growthRate;
    }

    // Estimate Xp, Yp stripper caused envelope increase.
    stdXYp       = sqrt(Stripper_upara/sqr(Stripper_lambda))*1e-3;// mrad to rad
    XpAfStr      = tmpmat(1, 1) + sqr(stdXYp);
    growthRateXp = sqrt(XpAfStr/tmpmat(1, 1));
    YpAfStr      = tmpmat(3, 3) + sqr(stdXYp);
    growthRateYp = sqrt(YpAfStr/tmpmat(3, 3));

    for (k = 0; k < 6; k++) {
        tmpmat(k, 1) = tmpmat(k, 1)*growthRateXp;
        // This trick allows two times growthRate for <Zp, Zp>.
        tmpmat(1, k) = tmpmat(1, k)*growthRateXp;
        tmpmat(k, 3) = tmpmat(k, 3)*growthRateYp;
        tmpmat(3, k) = tmpmat(3, k)*growthRateYp;
    }

    // Get new charge states.
    const std::vector<double>& ChgState = conf.get<std::vector<double> >("IonChargeStates");
    const std::vector<double>& NChg = conf.get<std::vector<double> >("NCharge");
    n = ChgState.size();
    assert(NChg.size()==n);

    // Propagate reference particle.
    ref = ST.ref;
    Stripper_Propagate_ref(conf, ref, ChgState);

    s = StatePtr->pos;

    ST.real.resize(n);
    ST.moment0.resize(n);
    ST.moment1.resize(n);

    // Length is zero.
    StatePtr->pos = s;

    StatePtr->ref = ref;

    ChargeStripper(StatePtr->real[0].beta, ChgState, chargeAmount_Baron);

    for (k = 0; k < n; k++) {
        StatePtr->real[k].IonZ  = ChgState[k];
        StatePtr->real[k].IonQ  = chargeAmount_Baron[k];
        StatePtr->real[k].IonEs = ref.IonEs;
        StatePtr->real[k].IonEk = Ek_recomb;
        StatePtr->real[k].recalc();
        StatePtr->real[k].phis  = Fy_abs_recomb;
        StatePtr->moment0[k]    = ST.moment0_env;
        StatePtr->moment1[k]    = tmpmat;
    }

    ST.recalc(); // necessary?

    ST.calc_rms();
}

void ElementStripper::advance(StateBase &s)
{
    state_t& ST = static_cast<state_t&>(s);

    Stripper_GetMat(conf(), ST);
}
