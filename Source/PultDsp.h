#pragma once

// Klangbild Pult – passiver Röhren-EQ im klassischen Programm-EQ-Stil
//
// Tiefen:  Anhebung (Kuhschwanz, leicht resonant) und Absenkung (flacher, höher angesetzt) auf derselben
//          Frequenz – beide zusammen ergeben den bekannten Trick: Bauch unten, Delle darüber.
// Höhen:   Anhebung als Glocke mit einstellbarer Breite, Absenkung als sanftes Kuhschwanz-Filter.

#include <array>
#include <cmath>
#include <complex>
#include "EQDsp.h"

namespace pult
{

inline constexpr std::array<double, 4> lowFreqs  { 20.0, 30.0, 60.0, 100.0 };
inline constexpr std::array<double, 7> highFreqs { 3000.0, 4000.0, 5000.0, 8000.0, 10000.0, 12000.0, 16000.0 };
inline constexpr std::array<double, 3> attenFreqs { 5000.0, 10000.0, 20000.0 };

struct Settings
{
    int lowFreq = 2; float lowBoost = 0, lowAtten = 0;
    int highFreq = 4; float bandwidth = 5, highBoost = 0;
    int attenFreq = 1; float highAtten = 0;

    bool operator== (const Settings& o) const
    {
        return lowFreq == o.lowFreq && highFreq == o.highFreq && attenFreq == o.attenFreq
            && eqf (lowBoost, o.lowBoost) && eqf (lowAtten, o.lowAtten) && eqf (bandwidth, o.bandwidth)
            && eqf (highBoost, o.highBoost) && eqf (highAtten, o.highAtten);
    }
    static bool eqf (float a, float b) { return ! (a < b) && ! (b < a); }
};

struct Section
{
    std::array<eq::Coeffs, 4> c;   // Tiefen-Anhebung, Tiefen-Absenkung, Höhen-Anhebung, Höhen-Absenkung
};

inline double clampFreq (double f, double sr) { return std::min (f, 0.45 * sr); }

inline Section design (const Settings& s, double sr)
{
    Section out;
    const double lf = lowFreqs[(size_t) std::clamp (s.lowFreq, 0, 3)];
    if (s.lowBoost > 0.0f) out.c[0] = eq::makeLowShelf (sr, lf * 1.5, 1.1, s.lowBoost * 1.4);
    if (s.lowAtten > 0.0f) out.c[1] = eq::makeLowShelf (sr, lf * 3.2, 0.45, -s.lowAtten * 1.0);

    const double hf = clampFreq (highFreqs[(size_t) std::clamp (s.highFreq, 0, 6)], sr);
    // Breite 0 = schmal (Q 2,5) … 10 = breit (Q 0,35)
    const double q = 2.5 * std::pow (0.35 / 2.5, s.bandwidth / 10.0);
    if (s.highBoost > 0.0f) out.c[2] = eq::makePeak (sr, hf, q, s.highBoost * 1.6);

    const double af = clampFreq (attenFreqs[(size_t) std::clamp (s.attenFreq, 0, 2)], sr);
    if (s.highAtten > 0.0f) out.c[3] = eq::makeHighShelf (sr, af, 0.6, -s.highAtten * 1.6);
    return out;
}

inline double magnitudeDb (const Section& sec, double f, double sr)
{
    const std::complex<double> z1 = std::polar (1.0, -2.0 * 3.14159265358979323846 * f / sr);
    const std::complex<double> z2 = z1 * z1;
    std::complex<double> h (1.0, 0.0);
    for (const auto& c : sec.c)
        h *= (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
    return 20.0 * std::log10 (std::max (std::abs (h), 1.0e-9));
}

struct Channel
{
    std::array<eq::BiquadState, 4> st;
    void reset() { for (auto& s : st) s.reset(); }
    float process (const Section& sec, float x)
    {
        for (size_t i = 0; i < 4; ++i) x = st[i].process (sec.c[i], x);
        return x;
    }
};

// Röhrenstufe: weiche, leicht unsymmetrische Kennlinie (geradzahlige Obertöne), Kleinsignal-Verstärkung 1
inline float tube (float x, float drive)
{
    constexpr float bias = 0.12f;
    const float d = 0.1f + 0.9f * drive;        // 100 %: bei −6 dBFS etwa 3 % k2, 2 % k3
    const float t0 = std::tanh (bias);
    const float slope = 1.0f - t0 * t0;
    return (std::tanh (d * x + bias) - t0) / (d * slope);
}

// entfernt den Gleichanteil, den die unsymmetrische Röhrenkennlinie bei hohem Pegel erzeugt
struct DcBlock
{
    float x1 = 0, y1 = 0, r = 0.9997f;
    void prepare (double sr) { r = (float) std::exp (-2.0 * 3.14159265358979323846 * 5.0 / sr); x1 = y1 = 0; }
    float process (float x) { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
};

} // namespace pult
