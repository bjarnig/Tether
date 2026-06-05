#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Grainstoch : public Unit {
    int m_knum;
    double* m_ampMem;
    double m_emitAcc;      // fractional countdown to the next emission
    long m_grainLen;       // samples in the current pulsaret
    long m_grainPos;       // position within the pulsaret; <0 = in the silent tail
    double m_sr;
};

enum { Pitch, KNum, Duty, Window, AmpStep, AmpJump, Jitter, AmpDist, AmpDistP, Interp };

void Grainstoch_clear(Grainstoch* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Grainstoch_next(Grainstoch* unit, int inNumSamples) {
    float* o = OUT(0);

    double pitch = ZIN0(Pitch);
    if (pitch < 0.01) pitch = 0.01;
    const double duty = t_clamp(ZIN0(Duty), 0.001, 1.0);
    const int window = (int)ZIN0(Window);
    const double ampStep = ZIN0(AmpStep);
    const double ampJump = t_clamp(ZIN0(AmpJump), 0.0, 1.0);
    const double jitter = t_clamp(ZIN0(Jitter), 0.0, 1.0);
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const int interp = (int)ZIN0(Interp);

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    double emitAcc = unit->m_emitAcc;
    long grainLen = unit->m_grainLen;
    long grainPos = unit->m_grainPos;

    for (int s = 0; s < inNumSamples; ++s) {
        if (emitAcc <= 0.0) {
            // walk the whole pulsaret; ampStep/ampJump set grain-to-grain decorrelation
            for (int k = 0; k < knum; ++k) {
                const double r = t_distribution(ampDist, (float)ampDistP, rgen.frand());
                const double walked = ampMem[k] + ampStep * r;
                ampMem[k] = t_mirror(walked + ampJump * (r - walked), -1.0, 1.0);
            }
            double pmul = 1.0 + jitter * (rgen.frand() * 2.0 - 1.0) * 0.9;
            if (pmul < 0.1) pmul = 0.1;
            double period = (sr / pitch) * pmul; // fractional -> exact emission rate
            if (period < 1.0) period = 1.0;
            if (period > sr * 4.0) period = sr * 4.0;
            grainLen = (long)(duty * period);
            if (grainLen < 1) grainLen = 1;
            grainPos = 0;
            emitAcc += period; // carry the fractional remainder
        }

        double outv = 0.0;
        if (grainPos >= 0 && grainPos < grainLen) {
            const double gp = (double)grainPos / (double)grainLen;
            const double cpf = gp * knum;
            int i0 = (int)cpf;
            if (i0 >= knum) i0 = knum - 1;
            int i1 = i0 + 1;
            if (i1 >= knum) i1 = knum - 1;
            const double a = t_interp(interp, ampMem[i0], ampMem[i1], cpf - (double)i0);
            outv = a * t_window(window, gp);
            grainPos++;
        } else {
            grainPos = -1;
        }
        o[s] = (float)outv;
        emitAcc -= 1.0;
    }

    unit->m_emitAcc = emitAcc;
    unit->m_grainLen = grainLen;
    unit->m_grainPos = grainPos;
}

void Grainstoch_Ctor(Grainstoch* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem) {
        SETCALC(Grainstoch_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    for (int i = 0; i < n; ++i)
        unit->m_ampMem[i] = std::sin(2.0 * T_PI * (i + 0.5) / n);

    unit->m_emitAcc = 0.0;
    unit->m_grainLen = 1;
    unit->m_grainPos = -1;

    SETCALC(Grainstoch_next);
    Grainstoch_next(unit, 1);
}

void Grainstoch_Dtor(Grainstoch* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
}

PluginLoad(Grainstoch) {
    ft = inTable;
    DefineDtorUnit(Grainstoch);
}
