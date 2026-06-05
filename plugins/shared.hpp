// Tether — shared helpers for the breakpoint/segment UGen family.
#pragma once
#include <cmath>

static const double T_PI = 3.14159265358979323846;

// Probability distributions for the stochastic step. Maps a uniform deviate
// f in [0,1) to a shaped deviate in ~[-1,1], shaped by a in [0,1].
// which: 0 linear, 1 cauchy, 2 logistic, 3 hyperbcos, 4 arcsine, 5 expon, 6 sinus
static inline float t_distribution(int which, float a, float f) {
    float temp, c;
    if (a > 1.f) a = 1.f;
    if (a < 0.0001f) a = 0.0001f;
    switch (which) {
    case 0:
        break;
    case 1:
        c = std::atan(10.f * a);
        return ((1.f / a) * std::tan(c * (2.f * f - 1.f))) * 0.1f;
    case 2:
        c = 0.5f + (0.499f * a);
        c = std::log((1.f - c) / c);
        f = ((f - 0.5f) * 0.998f * a) + 0.5f;
        return std::log((1.f - f) / f) / c;
    case 3:
        c = std::tan(1.5692255f * a);
        temp = std::tan(1.5692255f * a * f) / c;
        temp = std::log(temp * 0.999f + 0.001f) * (-0.1447648f);
        return 2.f * temp - 1.0f;
    case 4:
        c = std::sin(1.5707963f * a);
        return std::sin((float)T_PI * (f - 0.5f) * a) / c;
    case 5:
        c = std::log(1.f - (0.999f * a));
        return 2.f * (std::log(1.f - (f * 0.999f * a)) / c) - 1.f;
    case 6:
        return 2.f * a - 1.f;
    default:
        break;
    }
    return 2.f * f - 1.f;
}

static inline double t_clamp(double x, double lo, double hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

// Fold x back into [lo, hi].
static inline double t_mirror(double x, double lo, double hi) {
    const double range = hi - lo;
    if (range <= 0.0) return lo;
    double y = x - lo;
    const double twice = 2.0 * range;
    y -= twice * std::floor(y / twice);
    if (y > range) y = twice - y;
    return lo + y;
}

// Template shape at normalized phase p in [0,1). skew bends phase (pulse width).
// type: 0 sine, 1 saw, 2 triangle, 3 square, 4 flat.
static inline double t_template(int type, double p, double skew) {
    skew = t_clamp(skew, 0.001, 0.999);
    const double w = (p < skew) ? (0.5 * p / skew) : (0.5 + 0.5 * (p - skew) / (1.0 - skew));
    switch (type) {
    case 1: return 2.0 * w - 1.0;
    case 2: return 2.0 * std::fabs(2.0 * w - 1.0) - 1.0;
    case 3: return (p < skew) ? 1.0 : -1.0;
    case 4: return 0.0;
    default: return std::sin(2.0 * T_PI * w);
    }
}

// Per-bound barrier: 0 mirror, 1 wrap, 2 clip, 3 randomize (r in [0,1)).
static inline double t_barrier(double x, double lo, double hi, int modeLo, int modeHi, float r) {
    const double range = hi - lo;
    if (range <= 1e-9) return lo;
    if (x > hi) {
        const double over = x - hi;
        switch (modeHi) {
        case 1: x = lo + std::fmod(over, range); break;
        case 2: x = hi; break;
        case 3: x = lo + r * range; break;
        default: x = hi - std::fmod(over, range); break;
        }
    } else if (x < lo) {
        const double under = lo - x;
        switch (modeLo) {
        case 1: x = hi - std::fmod(under, range); break;
        case 2: x = lo; break;
        case 3: x = lo + r * range; break;
        default: x = lo + std::fmod(under, range); break;
        }
    }
    return t_clamp(x, lo, hi);
}

// Phase increment per sample for a control-point rate r in [0,1]; knum CPs per
// period. Fractional (no integer truncation) so the pitch is exact at any rate.
// Capped well above Nyquist to bound the advance loop.
static inline double t_phaseinc(double sr, double minf, double maxf, double r, int knum) {
    double cpRate = (minf + (maxf - minf) * r) * (double)knum;
    if (cpRate < 0.0) cpRate = 0.0;
    const double inc = cpRate / sr;
    return inc > 64.0 ? 64.0 : inc;
}

// Symmetric alpha-stable (Lévy) deviate via Chambers-Mallows-Stuck, from two
// uniform deviates r1,r2 in [0,1). alpha 2 = Gaussian, 1 = Cauchy; lower = heavier
// tails (rare large jumps). Bounded to keep it real-time safe.
static inline double t_stable(double alpha, double r1, double r2) {
    alpha = t_clamp(alpha, 0.5, 2.0);
    const double u = T_PI * (r1 - 0.5) * 0.999;
    const double w = -std::log(r2 + 1e-9);
    double x;
    if (std::fabs(alpha - 1.0) < 1e-6) {
        x = std::tan(u);
    } else {
        const double t1 = std::sin(alpha * u) / std::pow(std::cos(u), 1.0 / alpha);
        const double t2 = std::pow(std::cos(u - alpha * u) / w, (1.0 - alpha) / alpha);
        x = t1 * t2;
    }
    return t_clamp(x, -30.0, 30.0);
}

// Per-traversal state. phase is the position within the current segment [0,1);
// inc is the per-sample phase increment (fractional). Advance when phase >= 1.
struct CPState {
    int index;
    double phase;
    double inc;
    double curAmp;
    double nextAmp;
};

static inline void t_cp_reset(CPState& s) {
    s.index = 0;
    s.phase = 1.0; // >= 1 forces the first segment to be set up on the first sample
    s.inc = 0.0;
    s.curAmp = s.nextAmp = 0.0;
}

// Interpolate a..b by t in [0,1]. mode: 0 hold, 1 linear, 2 cosine.
static inline double t_interp(int mode, double a, double b, double t) {
    switch (mode) {
    case 0: return a;
    case 2: return a + (b - a) * (0.5 - 0.5 * std::cos(T_PI * t));
    default: return a + (b - a) * t;
    }
}

// Grain envelope at p in [0,1]. type: 0 rect, 1 hann, 2 triangle, 3 gaussian.
static inline double t_window(int type, double p) {
    switch (type) {
    case 1: return 0.5 - 0.5 * std::cos(2.0 * T_PI * p);
    case 2: return 1.0 - std::fabs(2.0 * p - 1.0);
    case 3: { const double x = (p - 0.5) * 5.0; return std::exp(-0.5 * x * x); }
    default: return 1.0;
    }
}

// interp: 0 hold, 1 linear, 2 cosine.
static inline double t_read(const CPState& s, int interp) {
    return t_interp(interp, s.curAmp, s.nextAmp, s.phase);
}
