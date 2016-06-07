#include <algorithm>
#include <ctime> // time()

#include <base/math.h>
#include "noise.h"

CPerlin::CPerlin(uint32_t seed)
: m_Engine(seed)
{
    if(!seed)
        seed = time(0);

    auto mid_range = m_aNumsPerlin.begin() + 256;

    std::iota(m_aNumsPerlin.begin(), mid_range, 0); //Generate sequential numbers in the lower half
    std::shuffle(m_aNumsPerlin.begin(), mid_range, m_Engine); //Shuffle the lower half
    std::copy(m_aNumsPerlin.begin(), mid_range, mid_range); //Copy the lower half to the upper half
    //p now has the numbers 0-255, shuffled, and duplicated
}

int CPerlin::GetURandom(int Max, int Min)
{
	std::uniform_int_distribution<int> uni(Max,Min);
	return uni(m_Engine);
}

double CPerlin::Noise(double x, double y, double z) const
{
    //See here for algorithm: http://cs.nyu.edu/~perlin/noise/

    const int32_t X = static_cast<int32_t>(std::floor(x)) & 255;
    const int32_t Y = static_cast<int32_t>(std::floor(y)) & 255;
    const int32_t Z = static_cast<int32_t>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    const double u = fade(x);
    const double v = fade(y);
    const double w = fade(z);

    const int A = m_aNumsPerlin[X] + Y;
    const int AA = m_aNumsPerlin[A] + Z;
    const int AB = m_aNumsPerlin[A + 1] + Z;
    const int B = m_aNumsPerlin[X + 1] + Y;
    const int BA = m_aNumsPerlin[B] + Z;
    const int BB = m_aNumsPerlin[B + 1] + Z;

    const int PAA = m_aNumsPerlin[AA];
    const int PBA = m_aNumsPerlin[BA];
    const int PAB = m_aNumsPerlin[AB];
    const int PBB = m_aNumsPerlin[BB];
    const int PAA1 = m_aNumsPerlin[AA + 1];
    const int PBA1 = m_aNumsPerlin[BA + 1];
    const int PAB1 = m_aNumsPerlin[AB + 1];
    const int PBB1 = m_aNumsPerlin[BB + 1];

    const auto a = lerp(v,
        lerp(u, grad(PAA, x, y, z), grad(PBA, x-1, y, z)),
        lerp(u, grad(PAB, x, y-1, z), grad(PBB, x-1, y-1, z))
    );

    const auto b = lerp(v,
        lerp(u, grad(PAA1, x, y, z-1), grad(PBA1, x-1, y, z-1)),
        lerp(u, grad(PAB1, x, y-1, z-1), grad(PBB1, x-1, y-1, z-1))
    );

    return lerp(w, a, b);
}

CPerlinOctave::CPerlinOctave(int octaves, uint32_t seed)
:
    m_Perlin(seed),
    m_Octaves(octaves)
{

}

double CPerlinOctave::Noise(double x, double y, double z) const
{
    double result = 0.0;
    double amp = 1.0;

    int i = m_Octaves;
    while(i--)
    {
        result += m_Perlin.Noise(x, y, z) * amp;
        x *= 2.0;
        y *= 2.0;
        z *= 2.0;
        amp *= 0.5;
    }

    return result;
}
