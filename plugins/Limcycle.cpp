#include "SC_PlugIn.h"
#include "shared.hpp"
#include "nonlinear.hpp"

static InterfaceTable* ft;

struct Limcycle : public Unit {
    double m_x;
    double m_v;
    double m_forcePhase;
    double m_sr;
};

enum { Freq, Mu, Shape, Well, Force, ForceFreq, Noise, Tail, Oversample };

void Limcycle_next(Limcycle* unit, int inNumSamples) {
    float* o0 = OUT(0);
    float* o1 = OUT(1);

    const double freq = ZIN0(Freq);
    const double mu = t_clamp(ZIN0(Mu), 0.0, 50.0);
    const double shape = t_clamp(ZIN0(Shape), 0.0, 1.0);
    const double well = t_clamp(ZIN0(Well), 0.0, 1.0);
    const double force = ZIN0(Force);
    const double forceFreq = ZIN0(ForceFreq);
    const double noise = ZIN0(Noise);
    const double tail = ZIN0(Tail);
    int ovs = (int)ZIN0(Oversample);
    if (ovs < 1) ovs = 1;
    if (ovs > 32) ovs = 32;

    RGen& rgen = *unit->mParent->mRGen;
    const double sr = unit->m_sr;
    const double w = 2.0 * T_PI * freq;
    const double dt = 1.0 / (sr * (double)ovs);
    const double forceInc = forceFreq / (sr * (double)ovs);

    double x = unit->m_x;
    double v = unit->m_v;
    double fp = unit->m_forcePhase;

    for (int s = 0; s < inNumSamples; ++s) {
        // Heavy-tailed (Lévy) stochastic kick, sampled-and-held over the sub-steps.
        const double kick = (noise != 0.0)
            ? noise * t_stable(tail, rgen.frand(), rgen.frand())
            : 0.0;

        for (int k = 0; k < ovs; ++k) {
            const double f = force * std::sin(2.0 * T_PI * fp) + kick;
            t_relax_step(x, v, w, mu, shape, well, f, dt);
            x = t_blowup_guard(x, 1e4);
            v = t_blowup_guard(v, 1e4);
            fp += forceInc;
            fp -= std::floor(fp);
        }
        o0[s] = (float)t_tanh(0.5 * x);
        o1[s] = (float)t_tanh(0.05 * v);
    }

    unit->m_x = x;
    unit->m_v = v;
    unit->m_forcePhase = fp;
}

void Limcycle_Ctor(Limcycle* unit) {
    unit->m_sr = SAMPLERATE;
    unit->m_x = 0.1; // nonzero seed so the limit cycle builds up
    unit->m_v = 0.0;
    unit->m_forcePhase = 0.0;

    SETCALC(Limcycle_next);
    Limcycle_next(unit, 1);
}

PluginLoad(Limcycle) {
    ft = inTable;
    DefineSimpleUnit(Limcycle);
}
