#include "SC_PlugIn.h"
#include "shared.hpp"

static InterfaceTable* ft;

struct Cyclegen : public Unit {
    int m_knum;
    double* m_ampMem;
    int m_segInCycle;   // 0..knum; reaching knum completes a cycle
    int m_cycleRepeat;  // cycles elapsed at the current frequency
    int m_seqPos;       // index into the frequency sequence
    int m_seqDir;       // +1 / -1 for pingpong
    int m_started;
    double m_curFreq;
    CPState m_main;
    double m_sr;
};

// Fixed inputs, then NumFreqs, then the splatted frequency sequence (last).
enum {
    KNum, AmpStep, AmpDist, AmpDistP, AmpJump, Stiffness,
    TemplateType, TemplateSkew, FreqMul, Repeats, SeqMode, Interp,
    NumFreqs, FreqBase
};

void Cyclegen_clear(Cyclegen* unit, int inNumSamples) {
    ClearUnitOutputs(unit, inNumSamples);
}

void Cyclegen_next(Cyclegen* unit, int inNumSamples) {
    float* o = OUT(0);

    const double ampStep = ZIN0(AmpStep);
    const int ampDist = (int)ZIN0(AmpDist);
    const double ampDistP = ZIN0(AmpDistP);
    const double ampJump = t_clamp(ZIN0(AmpJump), 0.0, 1.0);
    const double stiffness = t_clamp(ZIN0(Stiffness), 0.0, 1.0);
    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    const double freqMul = ZIN0(FreqMul);
    int repeats = (int)ZIN0(Repeats);
    if (repeats < 1) repeats = 1;
    const int seqMode = (int)ZIN0(SeqMode);
    const int interp = (int)ZIN0(Interp);
    int numFreqs = (int)ZIN0(NumFreqs);
    if (numFreqs < 1) numFreqs = 1;

    RGen& rgen = *unit->mParent->mRGen;
    const int knum = unit->m_knum;
    const double sr = unit->m_sr;
    double* ampMem = unit->m_ampMem;
    int segInCycle = unit->m_segInCycle;
    int cycleRepeat = unit->m_cycleRepeat;
    int seqPos = unit->m_seqPos;
    int seqDir = unit->m_seqDir;
    int started = unit->m_started;
    double curFreq = unit->m_curFreq;
    CPState main = unit->m_main;

    for (int s = 0; s < inNumSamples; ++s) {
        if (main.remain <= 0) {
            if (segInCycle >= knum) {       // completed a full cycle
                segInCycle = 0;
                cycleRepeat++;
                if (cycleRepeat >= repeats) { // step the sequence
                    cycleRepeat = 0;
                    if (!started) {
                        started = 1;
                        seqPos = (seqMode == 1) ? (numFreqs - 1) : 0;
                        if (seqMode == 3) seqPos = (int)(rgen.frand() * numFreqs);
                        seqDir = (seqMode == 1) ? -1 : 1;
                    } else {
                        switch (seqMode) {
                        case 1: seqPos = (seqPos - 1 + numFreqs) % numFreqs; break; // reverse
                        case 2:                                                     // pingpong
                            seqPos += seqDir;
                            if (seqPos >= numFreqs - 1) { seqPos = numFreqs - 1; seqDir = -1; }
                            else if (seqPos <= 0) { seqPos = 0; seqDir = 1; }
                            break;
                        case 3: seqPos = (int)(rgen.frand() * numFreqs); break;     // random
                        default: seqPos = (seqPos + 1) % numFreqs; break;          // forward
                        }
                    }
                    if (seqPos < 0) seqPos = 0;
                    if (seqPos >= numFreqs) seqPos = numFreqs - 1;
                    curFreq = ZIN0(FreqBase + seqPos) * freqMul;
                    if (curFreq < 0.0001) curFreq = 0.0001;
                }
            }
            const int cp = segInCycle;
            main.curAmp = main.nextAmp;

            const double tAmp = t_template(templateType, (cp + 0.5) / knum, skew);
            const double base = ampMem[cp];
            const double r = t_distribution(ampDist, (float)ampDistP, rgen.frand());
            const double walked = base - stiffness * (base - tAmp) + ampStep * r;
            double a = walked + ampJump * (r - walked); // jump toward an absolute draw
            a = t_mirror(a, -1.0, 1.0);
            ampMem[cp] = a;
            main.nextAmp = a;

            segInCycle++;
            long len = (long)(sr / (curFreq * knum));
            main.seglen = len < 1 ? 1 : len;
            main.remain = main.seglen;
        }
        o[s] = (float)t_read(main, interp);
        --main.remain;
    }

    unit->m_segInCycle = segInCycle;
    unit->m_cycleRepeat = cycleRepeat;
    unit->m_seqPos = seqPos;
    unit->m_seqDir = seqDir;
    unit->m_started = started;
    unit->m_curFreq = curFreq;
    unit->m_main = main;
}

void Cyclegen_Ctor(Cyclegen* unit) {
    unit->m_sr = SAMPLERATE;

    int n = (int)ZIN0(KNum);
    if (n < 1) n = 1;
    if (n > 1024) n = 1024;
    unit->m_knum = n;

    unit->m_ampMem = (double*)RTAlloc(unit->mWorld, n * sizeof(double));
    if (!unit->m_ampMem) {
        SETCALC(Cyclegen_clear);
        ClearUnitOutputs(unit, 1);
        return;
    }
    const int templateType = (int)ZIN0(TemplateType);
    const double skew = ZIN0(TemplateSkew);
    for (int i = 0; i < n; ++i)
        unit->m_ampMem[i] = t_template(templateType, (i + 0.5) / n, skew);

    unit->m_segInCycle = n;            // force a frequency load on the first segment
    unit->m_cycleRepeat = 1 << 29;     // force the first step regardless of repeats
    unit->m_seqPos = 0;
    unit->m_seqDir = 1;
    unit->m_started = 0;
    unit->m_curFreq = ZIN0(FreqBase);
    t_cp_reset(unit->m_main);

    SETCALC(Cyclegen_next);
    Cyclegen_next(unit, 1);
}

void Cyclegen_Dtor(Cyclegen* unit) {
    if (unit->m_ampMem) RTFree(unit->mWorld, unit->m_ampMem);
}

PluginLoad(Cyclegen) {
    ft = inTable;
    DefineDtorUnit(Cyclegen);
}
