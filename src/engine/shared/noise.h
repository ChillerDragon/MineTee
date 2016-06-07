#ifndef NOISE_H
#define NOISE_H
#include <stdint.h>

#include <random>
#include <array>

class CPerlin
{
public:
    CPerlin(uint32_t seed=0);

    double Noise(double x) const { return Noise(x, 0, 0); }
    double Noise(double x, double y) const { return Noise(x, y, 0); }
    double Noise(double x, double y, double z) const;
    int GetURandom(int Max, int Min);

private:
    std::array<int, 512> m_aNumsPerlin;
    std::mt19937 m_Engine;
};


class CPerlinOctave
{
public:
    CPerlinOctave(int octaves, uint32_t seed=0);

    CPerlin* Perlin() { return &m_Perlin; }
    double Noise(double x) const { return Noise(x, 0, 0); }
    double Noise(double x, double y) const { return Noise(x, y, 0); }
    double Noise(double x, double y, double z) const;

private:
    CPerlin m_Perlin;
    int m_Octaves;
};

#endif // NOISE_H
