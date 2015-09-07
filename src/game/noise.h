#ifndef NOISE_H
#define NOISE_H
#include <stdint.h>

//#include <random>
#include <array>

class CPerlin
{
public:
    CPerlin(uint32_t seed=0);

    double Noise(double x) const { return Noise(x, 0, 0); }
    double Noise(double x, double y) const { return Noise(x, y, 0); }
    double Noise(double x, double y, double z) const;

private:
    std::array<int, 512> m_aNumsPerlin;
};


class CPerlinOctave
{
public:
    CPerlinOctave(int octaves, uint32_t seed=0);

    double Noise(double x) const { return Noise(x, 0, 0); }
    double Noise(double x, double y) const { return Noise(x, y, 0); }
    double Noise(double x, double y, double z) const;

private:
    CPerlin m_Perlin;
    int m_Octaves;
};

#endif // NOISE_H
