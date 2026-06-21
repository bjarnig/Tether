#include "SC_PlugIn.h"
#include "shared.hpp"
#include "nonlinear.hpp"

static InterfaceTable* ft;

struct Lifestoch : public Unit {
    int m_max;
    int m_count;
    double* m_amp;
    double* m_energy;
    int m_index;
    double m_phase;
    double m_inc;
    double m_curAmp;
    double m_nextAmp;
    double m_sr;
};

enum { Freq, MaxCPs, Growth, Crowding, BirthThresh, Mutate, Interp };

// One generation of population dynamics, run when the waveform cycle wraps.
static inline void life_step(Lifestoch* unit, double growth, double crowding,
                             double birthThresh, double mutate, RGen& rgen) {
    double* amp = unit->m_amp;
    double* energy = unit->m_energy;
    int count = unit->m_count;
    const int max = unit->m_max;

    // Logistic metabolism: net energy is positive below carrying capacity.
    for (int i = 0; i < count; ++i)
        energy[i] += growth - crowding * (double)count;

    // Deaths: starved agents are removed (swap with the tail).
    int i = 0;
    while (i < count) {
        if (energy[i] <= 0.0 && count > 1) {
            amp[i] = amp[count - 1];
            energy[i] = energy[count - 1];
            --count;
        } else {
            ++i;
        }
    }

    // Births: well-fed agents split, child inherits a mutated amplitude.
    const int orig = count;
    for (int j = 0; j < orig && count < max; ++j) {
        if (energy[j] > birthThresh) {
            energy[j] *= 0.5;
            amp[count] = t_clamp(amp[j] + mutate * (rgen.frand() * 2.0 - 1.0), -1.0, 1.0);
            energy[count] = energy[j];
            ++count;
        }
    }

    unit->m_count = count;
}

void Lifestoch_clear(Lifestoch* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Lifestoch_next(Lifestoch* unit, int inNumSamples) {
    float* o = OUT(0);

    const double freq = ZIN0(Freq);
    const double growth = ZIN0(Growth);
    const double crowding = t_clamp(ZIN0(Crowding), 1e-6, 10.0);
    const double birthThresh = ZIN0(BirthThresh);
    const double mutate = ZIN0(Mutate);
    const int interp = (int)ZIN0(Interp);

    RGen& rgen = *unit->mParent->mRGen;
    const double sr = unit->m_sr;
    double phase = unit->m_phase;
    double inc = unit->m_inc;
    double curAmp = unit->m_curAmp;
    double nextAmp = unit->m_nextAmp;
    int index = unit->m_index;

    for (int s = 0; s < inNumSamples; ++s) {
        int g = 0;
        while (phase >= 1.0 && g++ < 64) {
            phase -= 1.0;
            ++index;
            if (index >= unit->m_count) {
                index = 0;
                life_step(unit, growth, crowding, birthThresh, mutate, rgen);
            }
            const int count = unit->m_count;
            curAmp = nextAmp;
            nextAmp = unit->m_amp[index];
            // count breakpoints per period -> fundamental stays at freq as the
            // population (and thus the segment subdivision) breathes.
            double newInc = freq * (double)count / sr;
            inc = newInc > 64.0 ? 64.0 : (newInc < 0.0 ? 0.0 : newInc);
        }
        o[s] = (float)t_interp(interp, curAmp, nextAmp, phase);
        phase += inc;
    }

    unit->m_phase = phase;
    unit->m_inc = inc;
    unit->m_curAmp = curAmp;
    unit->m_nextAmp = nextAmp;
    unit->m_index = index;
}

void Lifestoch_Ctor(Lifestoch* unit) {
    unit->m_sr = SAMPLERATE;

    int max = (int)ZIN0(MaxCPs);
    if (max < 2) max = 2;
    if (max > 256) max = 256;
    unit->m_max = max;

    unit->m_amp = (double*)RTAlloc(unit->mWorld, max * sizeof(double));
    unit->m_energy = (double*)RTAlloc(unit->mWorld, max * sizeof(double));
    if (!unit->m_amp || !unit->m_energy) {
        SETCALC(Lifestoch_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }

    int count = max / 2;
    if (count < 2) count = 2;
    unit->m_count = count;
    const double birthThresh = ZIN0(BirthThresh);
    for (int i = 0; i < count; ++i) {
        unit->m_amp[i] = std::sin(2.0 * T_PI * (double)i / (double)count);
        unit->m_energy[i] = 0.5 * birthThresh; // mid-life so it settles, not a birth/death burst
    }
    unit->m_index = 0;
    unit->m_phase = 1.0; // force first segment setup
    unit->m_inc = 0.0;
    unit->m_curAmp = unit->m_nextAmp = 0.0;

    SETCALC(Lifestoch_next);
    Lifestoch_next(unit, 1);
}

void Lifestoch_Dtor(Lifestoch* unit) {
    if (unit->m_amp) RTFree(unit->mWorld, unit->m_amp);
    if (unit->m_energy) RTFree(unit->mWorld, unit->m_energy);
}

PluginLoad(Lifestoch) {
    ft = inTable;
    DefineDtorUnit(Lifestoch);
}
