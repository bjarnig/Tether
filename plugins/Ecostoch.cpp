#include "SC_PlugIn.h"
#include "shared.hpp"
#include "nonlinear.hpp"

static InterfaceTable* ft;

struct Ecostoch : public Unit {
    double m_prey;
    double m_pred;
    double m_cphase;
    double m_sr;
};

enum { CycleRate, Growth, Decay, Interaction, CarrierFreq, FmIndex, Noise, Oversample };

void Ecostoch_next(Ecostoch* unit, int inNumSamples) {
    float* o0 = OUT(0); // FM/AM signal
    float* o1 = OUT(1); // prey CV
    float* o2 = OUT(2); // predator CV

    const double rate = ZIN0(CycleRate);
    const double a = t_clamp(ZIN0(Growth), 0.01, 10.0);        // prey growth
    const double c = t_clamp(ZIN0(Decay), 0.01, 10.0);         // predator death
    const double inter = t_clamp(ZIN0(Interaction), 0.01, 10.0); // predation (b = d)
    const double cfreq = ZIN0(CarrierFreq);
    const double fmIndex = ZIN0(FmIndex);
    const double noise = ZIN0(Noise);
    int ovs = (int)ZIN0(Oversample);
    if (ovs < 1) ovs = 1;
    if (ovs > 32) ovs = 32;

    RGen& rgen = *unit->mParent->mRGen;
    const double sr = unit->m_sr;
    // Scale time so the linearized predator-prey cycle sits at `rate` Hz.
    const double k = 2.0 * T_PI * rate / std::sqrt(a * c);
    const double dt = 1.0 / (sr * (double)ovs);
    const double xEq = c / inter; // prey equilibrium
    const double yEq = a / inter; // predator equilibrium
    const double cinc = cfreq / sr;

    double x = unit->m_prey;
    double y = unit->m_pred;
    double cph = unit->m_cphase;

    for (int s = 0; s < inNumSamples; ++s) {
        for (int n = 0; n < ovs; ++n) {
            auto dx = [&](double xx, double yy) { return k * (a * xx - inter * xx * yy); };
            auto dy = [&](double xx, double yy) { return k * (-c * yy + inter * xx * yy); };
            const double k1x = dx(x, y), k1y = dy(x, y);
            const double k2x = dx(x + 0.5 * dt * k1x, y + 0.5 * dt * k1y), k2y = dy(x + 0.5 * dt * k1x, y + 0.5 * dt * k1y);
            const double k3x = dx(x + 0.5 * dt * k2x, y + 0.5 * dt * k2y), k3y = dy(x + 0.5 * dt * k2x, y + 0.5 * dt * k2y);
            const double k4x = dx(x + dt * k3x, y + dt * k3y), k4y = dy(x + dt * k3x, y + dt * k3y);
            x += (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
            y += (dt / 6.0) * (k1y + 2.0 * k2y + 2.0 * k3y + k4y);
            x = t_blowup_guard(x, 1e6);
            y = t_blowup_guard(y, 1e6);
            if (x < 1e-9) x = 1e-9;
            if (y < 1e-9) y = 1e-9;
        }
        if (noise != 0.0) {
            // demographic fluctuations scale with sqrt(population)
            x += noise * std::sqrt(x) * (rgen.frand() - rgen.frand());
            y += noise * std::sqrt(y) * (rgen.frand() - rgen.frand());
            if (x < 1e-9) x = 1e-9;
            if (y < 1e-9) y = 1e-9;
        }
        const double preyN = x / xEq - 1.0; // bipolar around equilibrium
        const double predN = y / yEq - 1.0;
        const double amp = t_clamp(0.5 * (x / xEq), 0.0, 1.0);
        // prey drives carrier amplitude (AM), predator drives FM index.
        o0[s] = (float)(std::sin(2.0 * T_PI * cph + fmIndex * predN) * amp);
        o1[s] = (float)t_clamp(preyN, -1.0, 1.0);
        o2[s] = (float)t_clamp(predN, -1.0, 1.0);
        cph += cinc;
        cph -= std::floor(cph);
    }

    unit->m_prey = x;
    unit->m_pred = y;
    unit->m_cphase = cph;
}

void Ecostoch_Ctor(Ecostoch* unit) {
    unit->m_sr = SAMPLERATE;
    const double a = t_clamp(ZIN0(Growth), 0.01, 10.0);
    const double c = t_clamp(ZIN0(Decay), 0.01, 10.0);
    const double inter = t_clamp(ZIN0(Interaction), 0.01, 10.0);
    unit->m_prey = 1.5 * (c / inter); // start off equilibrium so it orbits
    unit->m_pred = (a / inter);
    unit->m_cphase = 0.0;

    SETCALC(Ecostoch_next);
    Ecostoch_next(unit, 1);
}

PluginLoad(Ecostoch) {
    ft = inTable;
    DefineSimpleUnit(Ecostoch);
}
