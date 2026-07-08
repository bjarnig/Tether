#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

// Light mean reversion on the momentum: bounds it without opening the periodic
// windows that stronger dissipation carves out of the chaotic region.
static const double GENDREVE_GAMMA = 0.9999;

struct Gendreve : public Unit {
    int m_knum;
    double* m_ampP;
    double* m_ampTh;
    double* m_durP;
    double* m_durTh;
    CPState m_main;
    CPState m_ref;
    double m_sr;
};

enum {
    MinFreq, MaxFreq, KNum, AmpStiff, DurStiff, AmpKick, DurKick,
    TemplateType, TemplateSkew, CenterDur, Interp
};

void Gendreve_clear(Gendreve* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Gendreve_next(Gendreve* unit, int inNumSamples) {
    float* o0 = OUT(0);
    float* o1 = OUT(1);

    const double minf = ZIN0(MinFreq);
    const double maxf = ZIN0(MaxFreq);
    const double ampStiff = t_clamp(ZIN0(AmpStiff), 0.0, 1.0);
    const double durStiff = t_clamp(ZIN0(DurStiff), 0.0, 1.0);
    const double ampKick = ZIN0(AmpKick);
    const double durKick = ZIN0(DurKick);
    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    const double centerDur = t_clamp(ZIN0(CenterDur), 0.0, 1.0);
    const int interp = (int)ZIN0(Interp);

    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampP = unit->m_ampP;
    double* ampTh = unit->m_ampTh;
    double* durP = unit->m_durP;
    double* durTh = unit->m_durTh;
    CPState main = unit->m_main;
    CPState ref = unit->m_ref;

    for (int s = 0; s < inNumSamples; ++s) {
        int gm = 0;
        while (main.phase >= 1.0 && gm++ < 64) {
            main.phase -= 1.0;
            main.index = (main.index + 1) % knum;
            const int i = main.index;
            main.curAmp = main.nextAmp;

            const double tAmp = t_template(templateType, (i + 0.5) / knum, skew);

            t_stdmap(ampP[i], ampTh[i], ampKick * std::sin(ampTh[i]), GENDREVE_GAMMA, 1.0);
            // The tether scales the deviation instead of contracting the map state:
            // stiffness 1 lands exactly on the template, and the chaos is untouched.
            const double dev = t_mirror(ampP[i], -1.0, 1.0);
            main.nextAmp = t_mirror(tAmp + (1.0 - ampStiff) * dev, -1.0, 1.0);

            t_stdmap(durP[i], durTh[i], durKick * std::sin(durTh[i]), GENDREVE_GAMMA, 1.0);
            const double ddev = t_mirror(durP[i], -1.0, 1.0);
            const double d = t_mirror(centerDur + (1.0 - durStiff) * 0.5 * ddev, 0.0, 1.0);

            main.inc = t_phaseinc(sr, minf, maxf, d, knum);
        }
        int gr = 0;
        while (ref.phase >= 1.0 && gr++ < 64) {
            ref.phase -= 1.0;
            ref.index = (ref.index + 1) % knum;
            const int i = ref.index;
            ref.curAmp = ref.nextAmp;
            ref.nextAmp = t_template(templateType, (i + 0.5) / knum, skew);
            ref.inc = t_phaseinc(sr, minf, maxf, centerDur, knum);
        }
        o0[s] = (float)t_read(main, interp);
        o1[s] = (float)t_read(ref, interp);
        main.phase += main.inc;
        ref.phase += ref.inc;
    }

    unit->m_main = main;
    unit->m_ref = ref;
}

void Gendreve_Ctor(Gendreve* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampP = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_ampTh = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durP = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durTh = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampP || !unit->m_ampTh || !unit->m_durP || !unit->m_durTh) {
        SETCALC(Gendreve_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }

    // Golden-ratio angles: a low-discrepancy spread so the rotors start decorrelated.
    for (int i = 0; i < n; ++i) {
        unit->m_ampP[i] = 0.0;
        unit->m_ampTh[i] = 2.0 * T_PI * std::fmod((i + 1) * T_PHI, 1.0);
        unit->m_durP[i] = 0.0;
        unit->m_durTh[i] = 2.0 * T_PI * std::fmod((i + 1) * T_PHI * 0.5 + 0.37, 1.0);
    }
    t_cp_reset(unit->m_main);
    t_cp_reset(unit->m_ref);

    SETCALC(Gendreve_next);
    Gendreve_next(unit, 1);
}

void Gendreve_Dtor(Gendreve* unit) {
    if (unit->m_ampP) RTFree(unit->mWorld, unit->m_ampP);
    if (unit->m_ampTh) RTFree(unit->mWorld, unit->m_ampTh);
    if (unit->m_durP) RTFree(unit->mWorld, unit->m_durP);
    if (unit->m_durTh) RTFree(unit->mWorld, unit->m_durTh);
}

PluginLoad(Gendreve) {
    ft = inTable;
    DefineDtorUnit(Gendreve);
}
