#ifndef ALSA_CAPTURE_H
#define ALSA_CAPTURE_H

#define CHANNELS 16 //2  //声道数目
#define FRAMES   256    //buf的大小

int init_cap(const char *hw);

int capture(void *buffer, size_t frames);

void exit_cap();


#endif
