#include "SC_PlugIn.h"
#include "shared.hpp"
#include "nonlinear.hpp"

static InterfaceTable* ft;

struct Limcycle : public Unit {
    double m_x;
    double m_v;
    double m_z;
    double m_forcePhase;
    double m_sr;
};

enum { Freq, Mu, Shape, Well, Force, ForceFreq, Feedback, FbRate, Noise, Tail, Oversample };

void Limcycle_next(Limcycle* unit, int inNumSamples) {
    float* o0 = OUT(0);
    float* o1 = OUT(1);

    const double freq = ZIN0(Freq);
    const double mu = t_clamp(ZIN0(Mu), 0.0, 50.0);
    const double shape = t_clamp(ZIN0(Shape), 0.0, 1.0);
    const double well = t_clamp(ZIN0(Well), 0.0, 1.0);
    const double force = ZIN0(Force);
    const double forceFreq = ZIN0(ForceFreq);
    const double fb = t_clamp(ZIN0(Feedback), 0.0, 20.0);
    const double fbRate = t_clamp(ZIN0(FbRate), 0.001, 4.0);
    const double noise = ZIN0(Noise);
    const double tail = ZIN0(Tail);
    int ovs = (int)ZIN0(Oversample);
    if (ovs < 1) ovs = 1;
    if (ovs > 32) ovs = 32;

    RGen& rgen = *unit->mParent->mRGen;
    const double sr = unit->m_sr;

    // Nondimensional time: tau = 2*pi*freq*t, so one natural period is 2*pi of tau.
    // RK4 needs dtau small against both the natural period and the mu-driven damping,
    // so add sub-steps beyond oversample rather than let high freq/mu diverge.
    int sub = ovs;
    const double dtauMax = 0.05 / (1.0 + 0.25 * mu);
    const double need = 2.0 * T_PI * freq / (sr * dtauMax);
    if (need > (double)sub) sub = (int)std::ceil(need);
    if (sub > 64) sub = 64;

    const double dtau = 2.0 * T_PI * freq / (sr * (double)sub);
    const double forceInc = forceFreq / (sr * (double)sub);
    // Velocity peaks scale with mu; normalize so out[1] stays in range across the sweep.
    const double vScale = 1.0 / (2.0 + mu);

    double x = unit->m_x;
    double v = unit->m_v;
    double z = unit->m_z;
    double fp = unit->m_forcePhase;

    for (int s = 0; s < inNumSamples; ++s) {
        // Heavy-tailed (Levy) stochastic kick, sampled-and-held over the sub-steps.
        const double kick = (noise != 0.0)
            ? noise * t_stable(tail, rgen.frand(), rgen.frand())
            : 0.0;

        for (int k = 0; k < sub; ++k) {
            const double f = force * std::sin(2.0 * T_PI * fp) + kick;
            t_relax_step(x, v, z, mu, shape, well, fb, fbRate, f, dtau);
            x = t_blowup_guard(x, 1e3);
            v = t_blowup_guard(v, 1e3);
            z = t_blowup_guard(z, 1e3);
            fp += forceInc;
            fp -= std::floor(fp);
        }
        o0[s] = (float)t_tanh(0.5 * x);
        o1[s] = (float)t_tanh(v * vScale);
    }

    unit->m_x = x;
    unit->m_v = v;
    unit->m_z = z;
    unit->m_forcePhase = fp;
}

void Limcycle_Ctor(Limcycle* unit) {
    unit->m_sr = SAMPLERATE;
    unit->m_x = 0.1; // nonzero seed so the limit cycle builds up
    unit->m_v = 0.0;
    unit->m_z = 0.0;
    unit->m_forcePhase = 0.0;

    SETCALC(Limcycle_next);
    Limcycle_next(unit, 1);
}

PluginLoad(Limcycle) {
    ft = inTable;
    DefineSimpleUnit(Limcycle);
}
