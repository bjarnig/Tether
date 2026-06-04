// Tether — probability distributions for the stochastic step.
// Maps a uniform deviate f in [0,1) to a shaped deviate in ~[-1,1],
// shaped by a in [0,1].
#pragma once
#include <cmath>

namespace tether {

constexpr double T_PI = 3.14159265358979323846;
constexpr float T_PI_F = 3.14159265358979323846f;

// which: 0 linear, 1 cauchy, 2 logistic, 3 hyperbcos, 4 arcsine, 5 expon, 6 sinus
inline float t_distribution(int which, float a, float f) {
    float temp, c;

    if (a > 1.f)
        a = 1.f;
    if (a < 0.0001f)
        a = 0.0001f;

    switch (which) {
    case 0:
        break;

    case 1:
        c = std::atan(10.f * a);
        temp = (1.f / a) * std::tan(c * (2.f * f - 1.f));
        return temp * 0.1f;

    case 2:
        c = 0.5f + (0.499f * a);
        c = std::log((1.f - c) / c);
        f = ((f - 0.5f) * 0.998f * a) + 0.5f;
        temp = std::log((1.f - f) / f) / c;
        return temp;

    case 3:
        c = std::tan(1.5692255f * a);
        temp = std::tan(1.5692255f * a * f) / c;
        temp = std::log(temp * 0.999f + 0.001f) * (-0.1447648f);
        return 2.f * temp - 1.0f;

    case 4:
        c = std::sin(1.5707963f * a);
        temp = std::sin(T_PI_F * (f - 0.5f) * a) / c;
        return temp;

    case 5:
        c = std::log(1.f - (0.999f * a));
        temp = std::log(1.f - (f * 0.999f * a)) / c;
        return 2.f * temp - 1.f;

    case 6:
        return 2.f * a - 1.f;

    default:
        break;
    }

    return 2.f * f - 1.f;
}

} // namespace tether
