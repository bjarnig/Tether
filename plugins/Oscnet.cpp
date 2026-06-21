#include "SC_PlugIn.h"
#include "shared.hpp"
#include "nonlinear.hpp"

static InterfaceTable* ft;

struct Oscnet : public Unit {
    int m_voices;
    double* m_phase;
    double* m_last;
    double* m_detune;  // per-voice drift factor (random walk around 1)
    double m_modPhase;
    double m_fb;
    double m_sr;
};

enum { Freq, Voices, Spread, Topo, Couple, ModFreq, ModDepth, Fold, Drive, Feedback, Drift };

void Oscnet_clear(Oscnet* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Oscnet_next(Oscnet* unit, int inNumSamples) {
    float* o0 = OUT(0); // shaped mix
    float* o1 = OUT(1); // clean mix

    const double freq = ZIN0(Freq);
    const double spread = ZIN0(Spread);
    const int topo = (int)ZIN0(Topo);
    const double couple = ZIN0(Couple);
    const double modFreq = ZIN0(ModFreq);
    const double modDepth = ZIN0(ModDepth);
    const double fold = t_clamp(ZIN0(Fold), 0.0, 1.0);
    const double drive = ZIN0(Drive);
    const double fb = t_clamp(ZIN0(Feedback), 0.0, 0.99);
    const double drift = t_clamp(ZIN0(Drift), 0.0, 1.0);

    RGen& rgen = *unit->mParent->mRGen;
    const int V = unit->m_voices;
    const double sr = unit->m_sr;
    double* phase = unit->m_phase;
    double* last = unit->m_last;
    double* detune = unit->m_detune;
    double modPhase = unit->m_modPhase;
    double fbPrev = unit->m_fb;

    const double center = 0.5 * (double)(V - 1);
    const double driftStep = drift * 1e-4;

    for (int s = 0; s < inNumSamples; ++s) {
        const double mod = std::sin(2.0 * T_PI * modPhase) * modDepth;

        double sumLast = 0.0;
        for (int i = 0; i < V; ++i) sumLast += last[i];

        double out[64];
        double sum = 0.0;
        for (int i = 0; i < V; ++i) {
            // Coupling input depends on the network topology.
            double coup;
            switch (topo) {
            case 1: // ring: nearest neighbours
                coup = 0.5 * (last[(i + V - 1) % V] + last[(i + 1) % V]);
                break;
            case 2: // all-to-all (mean field, excluding self)
                coup = (V > 1) ? (sumLast - last[i]) / (double)(V - 1) : 0.0;
                break;
            default: // pairs: 0<->1, 2<->3, ...
                coup = last[i ^ 1];
                break;
            }

            // Slow stochastic walk on the per-voice detune (organic instability).
            if (driftStep > 0.0) {
                detune[i] += driftStep * (rgen.frand() * 2.0 - 1.0);
                detune[i] = t_clamp(detune[i], 0.5, 2.0);
            }

            const double ratio = std::pow(spread, (double)i - center);
            const double base = freq * ratio * detune[i];
            const double inc = (base / sr) * (1.0 + mod + couple * coup + fb * fbPrev);
            phase[i] += inc;
            phase[i] -= std::floor(phase[i]);
            out[i] = std::sin(2.0 * T_PI * phase[i]);
            sum += out[i];
        }
        for (int i = 0; i < V; ++i) last[i] = out[i];

        const double clean = sum / (double)V;
        const double driven = drive * clean;
        // Nonlinearity morphs from tanh saturation (0) to triangle wavefolding (1).
        const double dist = (1.0 - fold) * t_tanh(driven) + fold * t_mirror(driven, -1.0, 1.0);
        fbPrev = dist;

        o0[s] = (float)dist;
        o1[s] = (float)clean;

        modPhase += modFreq / sr;
        modPhase -= std::floor(modPhase);
    }

    unit->m_modPhase = modPhase;
    unit->m_fb = fbPrev;
}

void Oscnet_Ctor(Oscnet* unit) {
    unit->m_sr = SAMPLERATE;

    int V = (int)ZIN0(Voices);
    if (V < 1) V = 1;
    if (V > 64) V = 64;
    unit->m_voices = V;

    unit->m_phase = (double*)RTAlloc(unit->mWorld, V * sizeof(double));
    unit->m_last = (double*)RTAlloc(unit->mWorld, V * sizeof(double));
    unit->m_detune = (double*)RTAlloc(unit->mWorld, V * sizeof(double));
    if (!unit->m_phase || !unit->m_last || !unit->m_detune) {
        SETCALC(Oscnet_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    for (int i = 0; i < V; ++i) {
        unit->m_phase[i] = (double)i / (double)V; // stagger so coupling isn't a null
        unit->m_last[i] = 0.0;
        unit->m_detune[i] = 1.0;
    }
    unit->m_modPhase = 0.0;
    unit->m_fb = 0.0;

    SETCALC(Oscnet_next);
    Oscnet_next(unit, 1);
}

void Oscnet_Dtor(Oscnet* unit) {
    if (unit->m_phase) RTFree(unit->mWorld, unit->m_phase);
    if (unit->m_last) RTFree(unit->mWorld, unit->m_last);
    if (unit->m_detune) RTFree(unit->mWorld, unit->m_detune);
}

PluginLoad(Oscnet) {
    ft = inTable;
    DefineDtorUnit(Oscnet);
}
