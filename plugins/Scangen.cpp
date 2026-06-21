#include "SC_PlugIn.h"
#include "shared.hpp"
#include "nonlinear.hpp"

static InterfaceTable* ft;

struct Scangen : public Unit {
    int m_n;
    double* m_p;  // mass positions
    double* m_v;  // mass velocities
    double m_scanPhase;
    double m_sr;
};

enum { Freq, Size, EvoRate, Tension, Damping, Center, Nonlin, Excite };

void Scangen_clear(Scangen* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Scangen_next(Scangen* unit, int inNumSamples) {
    float* o = OUT(0);

    const double freq = ZIN0(Freq);
    const double rate = t_clamp(ZIN0(EvoRate), 0.0, 0.5);  // lattice evolution speed
    const double tension = t_clamp(ZIN0(Tension), 0.0, 0.49);
    const double damping = t_clamp(ZIN0(Damping), 0.0, 1.0);
    const double center = t_clamp(ZIN0(Center), 0.0, 1.0); // restoring force to rest
    const double nonlin = ZIN0(Nonlin);
    const double excite = ZIN0(Excite);

    RGen& rgen = *unit->mParent->mRGen;
    const int N = unit->m_n;
    const double sr = unit->m_sr;
    double* p = unit->m_p;
    double* v = unit->m_v;
    double sph = unit->m_scanPhase;
    const double sinc = freq / sr;

    for (int s = 0; s < inNumSamples; ++s) {
        // Step the mass-spring ring one increment (speed decoupled from scan pitch).
        for (int i = 0; i < N; ++i) {
            const double left = p[(i + N - 1) % N];
            const double right = p[(i + 1) % N];
            const double acc = tension * (left + right - 2.0 * p[i])
                - center * p[i] - nonlin * p[i] * p[i] * p[i];
            double vv = v[i] * (1.0 - rate * damping) + rate * acc;
            if (excite != 0.0) vv += rate * excite * (rgen.frand() * 2.0 - 1.0);
            v[i] = vv;
        }
        for (int i = 0; i < N; ++i)
            p[i] = t_blowup_guard(p[i] + rate * v[i], 1e3);

        // Scan the displacement profile around the ring at audio rate.
        const double pos = sph * (double)N;
        const int i0 = (int)pos;
        const double frac = pos - (double)i0;
        const double a = p[i0 % N];
        const double b = p[(i0 + 1) % N];
        o[s] = (float)t_tanh(a + (b - a) * frac);

        sph += sinc;
        sph -= std::floor(sph);
    }

    unit->m_scanPhase = sph;
}

void Scangen_Ctor(Scangen* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(Size);
    if (n < 4) n = 4;
    if (n > 1024) n = 1024;
    unit->m_n = n;

    unit->m_p = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_v = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_p || !unit->m_v) {
        SETCALC(Scangen_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    // Seed with a low-amplitude bump so there is something to scan immediately.
    for (int i = 0; i < n; ++i) {
        unit->m_p[i] = 0.3 * std::sin(2.0 * T_PI * (double)i / (double)n);
        unit->m_v[i] = 0.0;
    }
    unit->m_scanPhase = 0.0;

    SETCALC(Scangen_next);
    Scangen_next(unit, 1);
}

void Scangen_Dtor(Scangen* unit) {
    if (unit->m_p) RTFree(unit->mWorld, unit->m_p);
    if (unit->m_v) RTFree(unit->mWorld, unit->m_v);
}

PluginLoad(Scangen) {
    ft = inTable;
    DefineDtorUnit(Scangen);
}
