// Tether — Gendreve
// Breakpoint oscillator whose control points random-walk but are pulled back
// toward a live template. Stiffness sets the pitch<->noise balance. Output 0 is
// the signal, output 1 a template-locked reference (out0 - out1 = residual).
#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Gendreve : public Unit {
    int m_knum;
    double* m_ampMem;
    double* m_durMem;
    CPState m_main;
    CPState m_ref;
    double m_sr;
};

enum {
    MinFreq, MaxFreq, KNum, AmpStiff, DurStiff, AmpNoise, DurNoise,
    AmpDist, AmpDistP, DurDist, DurDistP, TemplateType, TemplateSkew,
    CenterDur, Interp
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
    const double ampNoise = ZIN0(AmpNoise);
    const double durNoise = ZIN0(DurNoise);
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const int durDist = (int)ZIN0(DurDist);
    const double durDistP = ZIN0(DurDistP);
    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    const double centerDur = t_clamp(ZIN0(CenterDur), 0.0, 1.0);
    const int interp = (int)ZIN0(Interp);

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    double* durMem = unit->m_durMem;
    CPState main = unit->m_main;
    CPState ref = unit->m_ref;

    for (int s = 0; s < inNumSamples; ++s) {
        if (main.remain <= 0) {
            main.index = (main.index + 1) % knum;
            const int i = main.index;
            main.curAmp = main.nextAmp;

            const double tAmp = t_template(templateType, (i + 0.5) / knum, skew);
            double a = ampMem[i];
            a += -ampStiff * (a - tAmp) + ampNoise * t_distribution(ampDist, ampDistP, rgen.frand());
            a = t_mirror(a, -1.0, 1.0);
            ampMem[i] = a;
            main.nextAmp = a;

            double d = durMem[i];
            d += -durStiff * (d - centerDur) + durNoise * t_distribution(durDist, durDistP, rgen.frand());
            d = t_clamp(d, 0.0, 1.0);
            durMem[i] = d;

            main.seglen = t_seglen(sr, minf, maxf, d, knum);
            main.remain = main.seglen;
        }
        if (ref.remain <= 0) {
            ref.index = (ref.index + 1) % knum;
            const int i = ref.index;
            ref.curAmp = ref.nextAmp;
            ref.nextAmp = t_template(templateType, (i + 0.5) / knum, skew);
            ref.seglen = t_seglen(sr, minf, maxf, centerDur, knum);
            ref.remain = ref.seglen;
        }
        o0[s] = (float)t_read(main, interp);
        o1[s] = (float)t_read(ref, interp);
        --main.remain;
        --ref.remain;
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

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    unit->m_durMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem || !unit->m_durMem) {
        SETCALC(Gendreve_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }

    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    const double centerDur = t_clamp(ZIN0(CenterDur), 0.0, 1.0);
    for (int i = 0; i < n; ++i) {
        unit->m_ampMem[i] = t_template(templateType, (i + 0.5) / n, skew);
        unit->m_durMem[i] = centerDur;
    }
    t_cp_reset(unit->m_main);
    t_cp_reset(unit->m_ref);

    SETCALC(Gendreve_next);
    Gendreve_next(unit, 1);
}

void Gendreve_Dtor(Gendreve* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
    if (unit->m_durMem) RTFree(unit->mWorld, unit->m_durMem);
}

PluginLoad(Gendreve) {
    ft = inTable;
    DefineDtorUnit(Gendreve);
}
