#include "butterworthfilters.h"

/*Butterwort equation: y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*(n-2)
 *pag 484 Audio programming (Lazzarinni)
 *coefficients are from table 6.1 pag 484
 */

ButterworthFilters::ButterworthFilters()
{
    memset((float*)delayInLeft, 0, sizeof(float)*2);
    memset((float*)delayInRight, 0, sizeof(float)*2);
    memset((float*)delayOutLeft, 0, sizeof(float)*2);
    memset((float*)delayOutRight, 0, sizeof(float)*2);
}

void ButterworthFilters::displayCoef()
{
    printf("\n\n Filter coefficients: \n");
    printf(" lambda = %f\n", l);
    printf(" a0 = %f\n", a0);
    printf(" a1 = %f\n", a1);
    printf(" a2 = %f\n", a2);
    printf(" b1 = %f\n", b1);
    printf(" b2 = %f\n", b2);
    printf(" phi = %f\n", phi);
}

void ButterworthFilters::ComputeLPCoef(float sr, float freq) //compute low pass coefficients
{
    l = (1/(tan(PI*freq/sr)));
    a0 = (1/(1+2*l+l*l));
    a1 = (2*a0);
    a2 = (a0);
    b1 = 2*a0*(1-l*l);
    b2 = a0*(1-2*l+l*l);
}

void ButterworthFilters::ComputeHPCoef(float sr, float freq) //compute high pass coefficients
{
    l = tan(PI*freq/sr);
    a0 = 1/(1+2*l+l*l);
    a1 = 2*a0;
    a2 = a0;
    b1 = 2*a0*(l*l-1);
    b2 = a0*(1-2*l+l*l);
}

void ButterworthFilters::ComputeBPCoef(float sr, float freq, float BW)
{
    l = 1/(tan(PI*BW/sr));
    phi = 2*cos(2*PI*freq/sr);
    a0 = 1/(1+l);
    a1 = 0;
    a2 = -a0;
    b1 = -l*phi*a0;
    b2 = a0*(l-1);
}

void ButterworthFilters::ComputeBRCoef(float sr, float freq, float BW)
{
    l = tan(PI*BW/sr);
    phi = 2*cos(2*PI*freq/sr);
    a0 = 1/(1+l);
    a1 = -phi*a0;
    a2 = a0;
    b1 = -phi*a0;
    b2 = a0*(l-1);
}

void ButterworthFilters::StartFiletring(float *in_buffer, float *out_buffer, int vectsize) //start filtering after computing coefficints
{
    for(int i = 0; i < vectsize*2; i = i + 2)
    {
        out_buffer[i] = (float)(a0*in_buffer[i] + a1*delayInLeft[0] + a2*delayInLeft[1] - b1*delayOutLeft[0] - b2*delayOutLeft[1]);
        delayInLeft[1] = delayInLeft[0];
        delayInLeft[0] = in_buffer[i];

        delayOutLeft[1] = delayOutLeft[0];
        delayOutLeft[0] = out_buffer[i];


        out_buffer[i+1] = (float)(a0*in_buffer[i+1] + a1*delayInRight[0] + a2*delayInRight[1] - b1*delayOutRight[0] - b2*delayOutRight[1]);
        delayInRight[1] = delayInRight[0];
        delayInRight[0] = in_buffer[i+1];

        delayOutRight[1] = delayOutRight[0];
        delayOutRight[0] = out_buffer[i+1];
    }
}

