/*
This example reads from the default PCM device
and writes to standard output for 5 seconds of data.
*/
/* Use the newer ALSA API */
#include <stdio.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#include "alsa/asoundlib.h"
#include "alsa_capture.h"

#define RATE    16000 //采样频率

static snd_pcm_t *handle;
static snd_pcm_uframes_t frames;

int init_cap(const char *hw)
{
    int rc = 0;   
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t buffer_frames;
    
    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&handle, hw, SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr,  "unable to open pcm device: %s/n",  snd_strerror(rc));
        exit(1);
    }
    
    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);
    
    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);
    
    /* Set the desired hardware parameters. */
    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);
    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params,
                                 SND_PCM_FORMAT_S16_LE);
    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, CHANNELS);
    
    /* 44100 bits/second sampling rate (CD quality) */
    val = RATE;
    snd_pcm_hw_params_set_rate_near(handle, params,  &val, &dir);
    
    /* Set period size to 32 frames. */
    frames = FRAMES;
    snd_pcm_hw_params_set_period_size_near(handle,  params, &frames, &dir);
    
    buffer_frames = frames * 2;
    rc = snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer_frames);
    if (rc < 0) {
        perror("\nsnd_pcm_hw_params_set_buffer_size_near:");
        return -1;
    }
    
    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr,  "unable to set hw parameters: %s/n",
                snd_strerror(rc));
        exit(1);
    }
    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params,  &frames, &dir);
    printf("read frames = %d\n", (int)frames);

    return 0;
}

int capture(void *buffer, size_t frames)
{
    int rc;
    
    rc = snd_pcm_readi(handle, buffer, frames);
    //printf("read rc=%d\n", rc);
    
    if (rc == -EPIPE) {
        /* EPIPE means overrun */
        fprintf(stderr, "overrun occurred/n");
        snd_pcm_prepare(handle);
    } else if (rc < 0) {
        fprintf(stderr, "error from read: %s/n", snd_strerror(rc));
    } else if (rc != (int)frames) {
        fprintf(stderr, "short read, read %d frames/n", rc);
    }

    return rc;
}

void exit_cap()
{
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
}
