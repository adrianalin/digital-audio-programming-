/* sf2float.c :  convert soundfile to floats using portsf library */

#include "includes.h"

/*All these implementations are for stereo .wav files*/

/*basic resonator equation: y(n) = x(n) - b1*y(n-1) - b2*y(n-2)
 *BW - band width
 *sr - sample rate
 *f - filter center frequency
 *
 *R = 1 - PI*(BW/sr)
 *b1 = -[4 * R^2 / (1 + R^2)] * cos(2 * PI * f / sr)
 *b2 = R^2
 *
 *scaling = (1-R^2)*sin(2*pi/sr) -applied on input signal to avoid clipping distortion
 *
 *pag 483 Audio programming (Lazzarinni)
 */
void resonator(float *in_buffer, float *out_buffer, int vecsize, float sr, float freq, float bw)
{
    double r, rsq, rr, costh, scal;
    static float del1[2] = {0 ,0}, del2[2] = {0, 0}; //2 samples feedback delay
    int i = 0;

    rr = 2*(r = 1. - PI*(bw/sr));
    rsq = r*r;
    costh = (rr/(1.+rsq))*cos(PI_2*freq/sr);
    scal = (1-rsq)*sin(acos(costh));

    for(i=0; i<vecsize*2; i = i + 2)
    {
        out_buffer[i] = (float)(in_buffer[i]*scal + rr*costh*del1[0] - rsq*del1[1]); //left channel
        del1[1] = del1[0];
        del1[0] = out_buffer[i];

        out_buffer[i+1] = (float)(in_buffer[i+1]*scal + rr*costh*del2[0] - rsq*del2[1]); //right channel
        del2[1] = del2[0];
        del2[0] = out_buffer[i+1];
    }
}

/*equation: y(n) = a0*x(n) + a1*x(n-1) - b1*y(n-1) - b2*y(n-2)
 *improved version of the resonator above
 *R - filter radius calculated from calculated from the bandwidth
 *scaling = 1 - R
 *a2 = R
 *
 *direct form II:
 *w(n) = x(n) - b1*w(n-1) - b2*w(n-2) [feedback]
 *y(n) = a0*w(n) + a1*w(n-1) + a2*w(n-2) [feedforward]
 */
void bandPass(float *in_buffer, float *out_buffer, int vecsize, float sr, float freq, float bw)
{
    double r, rsq, rr, costh, scal, w1, w2;
    static float del1[2] = {0 ,0}, del2[2] = {0, 0};
    int i = 0;

    rr = 2*(r = 1. - PI*(bw/sr));
    rsq = r*r;
    costh = (rr/(1.+rsq))*cos(PI_2*freq/sr);
    scal = (1 - r);
    for(i=0; i<vecsize*2; i = i + 2)
    {
        w1 = scal*in_buffer[i] + rr*costh*del1[0] - rsq*del1[1]; //left
        out_buffer[i] = (float)(w1 - r*del1[1]);
        del1[1] = del1[0];
        del1[0] = (float) w1;

        w2 = scal*in_buffer[i+1] + rr*costh*del2[0] - rsq*del2[1]; //right
        out_buffer[i+1] = (float)(w2 - r*del2[1]);
        del2[1] = del2[0];
        del2[0] = (float) w2;
    }
}

int main(int argc, char* argv[])
{
    ButterworthFilters BFilter;

    PSF_PROPS props;
    int framesread;
    DWORD nFrames = 512; //512 frames input buffer
    long totalread;
    int ifd = -1,ofd = -1;
    int error = 0;
    float* in_buffer = NULL;
    float* out_buffer = NULL;
    float* frame = NULL;

    if(argc < 4)
    {
        printf("insufficient arguments.\n"
               "usage:\n\t"
               "test_MIT option infile outfile\n");
        return 1;
    }

    int part = atoi(argv[1]);

    if(psf_init())
    {
        printf("unable to start portsf\n");
        return 1;
    }

    ifd = psf_sndOpen(argv[2],&props,0);
    printf("\nSample rate: %d\n",props.srate);
    if(ifd < 0)
    {
        printf("Error: unable to open infile %s\n",argv[2]);
        return 1;
    }

    ofd = psf_sndCreate(argv[3],&props,1,0,PSF_CREATE_RDWR);
    if(ofd < 0)
    {
        printf("Error: unable to create outfile %s\n",argv[3]);
        error++;
        goto exit;
    }

    if(props.samptype == PSF_SAMP_16){
        printf("Info: infile is in 16 bit format.\n");
    }
    if(props.samptype == PSF_SAMP_24){
        printf("Info: infile is in 24 bit format.\n");
    }
    if(props.samptype == PSF_SAMP_32){
        printf("Info: infile is in 32 bit format.\n");
    }
    if(props.samptype == PSF_SAMP_IEEE_FLOAT){
        printf("Info: infile is in floats format.\n");
    }

    out_buffer = (float*) malloc(props.chans * sizeof(float) * nFrames); //alocate buffers for multichannel files
    memset((float*)out_buffer, 0, (props.chans * sizeof(float) * nFrames));
    in_buffer = (float*) malloc(props.chans * sizeof(float) * nFrames);
    memset((float*)in_buffer, 0, props.chans * sizeof(float) * nFrames);
    frame = (float*) malloc(props.chans * sizeof(float)); //nFrames + props.chans * sizeof(float) bytes

    if(frame==NULL)
    {
        puts("No memory!\n");
        error++;
        goto exit;
    }
    if(in_buffer==NULL)
    {
        puts("No memory!\n");
        error++;
        goto exit;
    }
    if(out_buffer==NULL)
    {
        puts("No memory!\n");
        error++;
        goto exit;
    }

    totalread = 0;
    framesread = 1;


    switch(part) //option entered from terminal
    {

    case 0: //resonator
    {
        float frequency;
        float bandWidth;
        cout<<"Resonator:"<<endl;
        cout<<"frequency = ";
        cin>>frequency;
        cout<<"band width = ";
        cin>>bandWidth;
        printf("\ncopying...\n");

        while (framesread > 0)
        {
            framesread = psf_sndReadFloatFrames(ifd, in_buffer, nFrames);
            totalread += framesread;
            printf("%ld\r",totalread);
            resonator(in_buffer, out_buffer, framesread, (float)props.srate, frequency, bandWidth);

            if(psf_sndWriteFloatFrames(ofd, out_buffer, framesread) != framesread)
            {
                printf("Error writing to outfile\n");
                error++;
                break;
            }
        }
        break;
    }

    case 1: //bandpass
    {
        float frequency;
        float bandWidth;
        cout<<"Band Pass:"<<endl;
        cout<<"frequency = ";
        cin>>frequency;
        cout<<"band width = ";
        cin>>bandWidth;
        printf("\ncopying...\n");

        while (framesread > 0)
        {
            framesread = psf_sndReadFloatFrames(ifd, in_buffer, nFrames);
            totalread += framesread;
            printf("%ld\r",totalread);
            bandPass(in_buffer, out_buffer, framesread, (float)props.srate, frequency, bandWidth);

            if(psf_sndWriteFloatFrames(ofd, out_buffer, framesread) != framesread)
            {
                printf("Error writing to outfile\n");
                error++;
                break;
            }
        }
        break;
    }

    case 2: //Butterworth low pass
    {
        float frequency;
        cout<<"Butterworth low pass:"<<endl;
        cout<<"frequency = ";
        cin>>frequency;
        printf("\ncopying...\n");

        BFilter.ComputeLPCoef((float)props.srate, frequency); //change frequency in 2-nd argument
        while (framesread > 0)
        {
            framesread = psf_sndReadFloatFrames(ifd, in_buffer, nFrames);
            totalread += framesread;
            printf("%ld\r",totalread);

            BFilter.StartFiletring(in_buffer, out_buffer, framesread);

            if(psf_sndWriteFloatFrames(ofd, out_buffer, framesread) != framesread)
            {
                printf("Error writing to outfile\n");
                error++;
                break;
            }
        }
        BFilter.displayCoef();
        break;
    }

    case 3: //Butterworth high pass --don't work well (maybe wrong coefficients?)
    {
        float frequency;
        cout<<"Butterworth high pass (don't work well'):"<<endl;
        cout<<"frequency = ";
        cin>>frequency;
        printf("\ncopying...\n");

        BFilter.ComputeHPCoef((float)props.srate, frequency); //change frequency in 2-nd argument
        while (framesread > 0)
        {
            framesread = psf_sndReadFloatFrames(ifd, in_buffer, nFrames);
            totalread += framesread;
            printf("%ld\r",totalread);

            BFilter.StartFiletring(in_buffer, out_buffer, framesread);

            if(psf_sndWriteFloatFrames(ofd, out_buffer, framesread) != framesread)
            {
                printf("Error writing to outfile\n");
                error++;
                break;
            }
        }
        BFilter.displayCoef();
        break;
    }

    case 4: //Butterworth band pass
    {
        float frequency;
        float bandWidth;
        cout<<"Butterworth band pass:"<<endl;
        cout<<"frequency = ";
        cin>>frequency;
        cout<<"band width = ";
        cin>>bandWidth;
        printf("\ncopying...\n");

        BFilter.ComputeBPCoef((float)props.srate, frequency, bandWidth); //change frequency in 2-nd argument
        while (framesread > 0)
        {
            framesread = psf_sndReadFloatFrames(ifd, in_buffer, nFrames);
            totalread += framesread;
            printf("%ld\r",totalread);

            BFilter.StartFiletring(in_buffer, out_buffer, framesread);

            if(psf_sndWriteFloatFrames(ofd, out_buffer, framesread) != framesread)
            {
                printf("Error writing to outfile\n");
                error++;
                break;
            }
        }
        BFilter.displayCoef();
        break;
    }

    case 5: //Butterworth band reject --this don't work well (maybe wrong coefficients?)
    {
        float frequency;
        float bandWidth;
        cout<<"Butterworth band reject (don't work well):"<<endl;
        cout<<"frequency = ";
        cin>>frequency;
        cout<<"band width = ";
        cin>>bandWidth;
        printf("\ncopying...\n");

        BFilter.ComputeBRCoef((float)props.srate, frequency, bandWidth); //change frequency in 2-nd argument
        while (framesread > 0)
        {
            framesread = psf_sndReadFloatFrames(ifd, in_buffer, nFrames);
            totalread += framesread;
            printf("%ld\r",totalread);

            BFilter.StartFiletring(in_buffer, out_buffer, framesread);

            if(psf_sndWriteFloatFrames(ofd, out_buffer, framesread) != framesread)
            {
                printf("Error writing to outfile\n");
                error++;
                break;
            }
        }
        BFilter.displayCoef();
        break;
    }

    default:
        printf("default case");
        break;

    }

    printf("\n");

    if(framesread < 0)	{
        printf("Error reading infile. Outfile is incomplete.\n");
        error++;
    }
    else {

        printf("Done. %ld sample frames copied \n\n",totalread);
    }


exit:
    if(ifd >= 0)
        psf_sndClose(ifd);
    if(ofd >= 0)
        psf_sndClose(ofd);
    if(frame)
        free(frame);
    if(in_buffer)
        free(in_buffer);
    if(out_buffer)
        free(out_buffer);
    psf_finish();
    return error;
}
