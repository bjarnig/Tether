// Tether — Seggen
// A frequency sequence sets exact pitch one cycle at a time; within each cycle
// the waveform is a ring of knum control points that random-walk (optionally
// jumping). Pitch is sequenced and precise; timbre evolves.
#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Seggen : public Unit {
    int m_knum;
    double* m_ampMem;
    int m_segInCycle;   // 0..knum; reaching knum loads the next frequency
    int m_seqPos;       // index into the frequency sequence
    double m_curFreq;
    CPState m_main;
    double m_sr;
};

// Fixed inputs, then NumFreqs, then the splatted frequency sequence (last).
enum { KNum, AmpStep, AmpDist, AmpDistP, AmpJump, Interp, NumFreqs, FreqBase };

void Seggen_clear(Seggen* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Seggen_next(Seggen* unit, int inNumSamples) {
    float* o = OUT(0);

    const double ampStep = ZIN0(AmpStep);
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const double ampJump = t_clamp(ZIN0(AmpJump), 0.0, 1.0);
    const int interp = (int)ZIN0(Interp);
    int numFreqs = (int)ZIN0(NumFreqs);
    if (numFreqs < 1) numFreqs = 1;

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    int segInCycle = unit->m_segInCycle;
    int seqPos = unit->m_seqPos;
    double curFreq = unit->m_curFreq;
    CPState main = unit->m_main;

    for (int s = 0; s < inNumSamples; ++s) {
        if (main.remain <= 0) {
            if (segInCycle >= knum) {
                segInCycle = 0;
                seqPos = (seqPos + 1) % numFreqs;
                curFreq = ZIN0(FreqBase + seqPos);
                if (curFreq < 0.0001) curFreq = 0.0001;
            }
            const int cp = segInCycle;
            main.curAmp = main.nextAmp;

            const double r = t_distribution(ampDist, (float)ampDistP, rgen.frand());
            const double walked = ampMem[cp] + ampStep * r;
            double a = walked + ampJump * (r - walked); // absolute draw spans [-1,1]
            a = t_mirror(a, -1.0, 1.0);
            ampMem[cp] = a;
            main.nextAmp = a;

            segInCycle++;
            long len = (long)(sr / (curFreq * knum)); // equal segments per cycle
            main.seglen = len < 1 ? 1 : len;
            main.remain = main.seglen;
        }
        o[s] = (float)t_read(main, interp);
        --main.remain;
    }

    unit->m_segInCycle = segInCycle;
    unit->m_seqPos = seqPos;
    unit->m_curFreq = curFreq;
    unit->m_main = main;
}

void Seggen_Ctor(Seggen* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem) {
        SETCALC(Seggen_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    for (int i = 0; i < n; ++i)
        unit->m_ampMem[i] = std::sin(2.0 * T_PI * (i + 0.5) / n);

    unit->m_segInCycle = n;   // force a frequency load on the first segment
    unit->m_seqPos = -1;
    unit->m_curFreq = ZIN0(FreqBase);
    t_cp_reset(unit->m_main);

    SETCALC(Seggen_next);
    Seggen_next(unit, 1);
}

void Seggen_Dtor(Seggen* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
}

PluginLoad(Seggen) {
    ft = inTable;
    DefineDtorUnit(Seggen);
}
