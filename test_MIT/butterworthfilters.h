#ifndef BUTTERWORTHFILTERS_H
#define BUTTERWORTHFILTERS_H

#include "includes.h"

class ButterworthFilters
{
private:
    float delayInLeft[2];
    float delayInRight[2];
    float delayOutLeft[2];
    float delayOutRight[2];
    double l, a0, a1, a2, b1, b2, phi;

public:
    ButterworthFilters();
    void ComputeLPCoef(float sr, float freq);
    void ComputeHPCoef(float sr, float freq);
    void ComputeBPCoef(float sr, float freq, float BW);
    void ComputeBRCoef(float sr, float freq, float BW);
    void displayCoef();
    void StartFiletring(float* in_buffer, float* out_buffer, int vectsize);
};

#endif // BUTTERWORTHFILTERS_H
