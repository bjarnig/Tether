// Tether — shared primitives for breakpoint/segment oscillators.
#pragma once
#include <cmath>
#include "Distributions.hpp"

namespace tether {

inline double t_clamp(double x, double lo, double hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

// Fold x back into [lo, hi].
inline double t_mirror(double x, double lo, double hi) {
    const double range = hi - lo;
    if (range <= 0.0)
        return lo;
    double y = x - lo;
    const double twice = 2.0 * range;
    y -= twice * std::floor(y / twice);
    if (y > range)
        y = twice - y;
    return lo + y;
}

// Template shape at normalized phase p in [0,1). skew bends phase (pulse width).
// type: 0 sine, 1 saw, 2 triangle, 3 square, 4 flat.
inline double t_template(int type, double p, double skew) {
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

// Independent behaviour per bound: 0 mirror, 1 wrap, 2 clip, 3 randomize.
// r is a uniform deviate in [0,1) for mode 3. Final clamp guards overshoot.
inline double t_barrier(double x, double lo, double hi, int modeLo, int modeHi, float r) {
    const double range = hi - lo;
    if (range <= 1e-9)
        return lo;
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

// Per-traversal interpolation state. remain counts samples to the next boundary.
struct CPState {
    int index = 0;
    long remain = 0;
    long seglen = 1;
    double curAmp = 0.0;
    double nextAmp = 0.0;

    void reset() {
        index = 0;
        remain = 0;
        seglen = 1;
        curAmp = nextAmp = 0.0;
    }
};

// interp: 0 hold, 1 linear, 2 cosine.
inline double t_read(const CPState& s, int interp) {
    const double t = (s.seglen > 0) ? double(s.seglen - s.remain) / double(s.seglen) : 0.0;
    const double d = s.nextAmp - s.curAmp;
    switch (interp) {
    case 0: return s.curAmp;
    case 2: return s.curAmp + d * (0.5 - 0.5 * std::cos(T_PI * t));
    default: return s.curAmp + d * t;
    }
}

} // namespace tether
