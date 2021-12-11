#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alsa_capture.h"
#include "algo.h"
#include "ringbuffer.h"

#define RINGBUFSIZE (ALGO_CHANNELS*FRAMES*2*6)
#define TEST_FILE "test.raw"
#define DEBUG_MAP 0 // 0, 1, 2

void *read_thread(void *);
void *algo_thread(void *);
void test_map(void);
static rb *rb_buf = NULL;

int run(void)
{
    pthread_t pid1,pid2;
 
    fprintf(stdout, "run!\n");
    rb_buf = rb_new(RINGBUFSIZE);
    algo_init();
    init_cap("default"); //"hw:0,0"
    
    pthread_create(&pid1,NULL,read_thread,(void *)NULL);
    pthread_create(&pid2,NULL,algo_thread,(void *)NULL);

    pthread_join(pid1, NULL);
    pthread_join(pid2, NULL);

    exit_cap();
    algo_destroy();
    rb_del(rb_buf);
    return 0;
}

int main(int argc, char *argv[])
{
    run();
    return 0;
}

void algo_data_map(int16_t buf[FRAMES][CHANNELS], int16_t algo_buf[ALGO_CHANNELS][FRAMES])
{
    int i, j, k;
    #if CHANNELS == 2
    const int map[CHANNELS] = {
            0,
            1,
        };
    #else
    const int map[CHANNELS] = {
            8,
            0,
            1,
            2,
            3,
            -1,
            -1,
            -1,
            
            9,
            4,
            5,
            6,
            7,
            -1,
            -1,
            -1
        };
    #endif
    
    for(i=0; i<FRAMES; i++) {
        for(j=0; j<CHANNELS; j++) {
            k = map[j];
            if(k >= 0) {
                algo_buf[k][i] = buf[i][j];
            }
        }
    }
}

#if 0
void test_map(void)
{
    int16_t buf[FRAMES][CHANNELS];
    int16_t algo_buf[ALGO_CHANNELS][FRAMES];
    int i, len;
    FILE *fp = fopen(TEST_FILE, "rb");
    FILE *fp_ch[ALGO_CHANNELS];
    char tmp[256];
    
    fprintf(stdout, "test_map!\n");
    
    for(i=0; i<ALGO_CHANNELS; i++) {
        sprintf(tmp, "ch%d-map.raw", i);
        fp_ch[i] = fopen(tmp, "wb");
    }
    
    while(fp) {
        len = fread(buf, sizeof(buf), 1, fp);
        if(len > 0) {
            algo_data_map(buf, algo_buf);
            for(i=0; i<ALGO_CHANNELS; i++) {
                fwrite(algo_buf[i], sizeof(algo_buf[i]), 1, fp_ch[i]);
            }
        } else {
            break;
        }
    }
    
    for(i=0; i<ALGO_CHANNELS; i++) {
        fclose(fp_ch[i]);
    }
    fclose(fp);
}
#endif

#if DEBUG_MAP == 1


static volatile int finish = 0;

void *read_thread(void *p){
    int16_t buf[FRAMES][CHANNELS];
    int16_t algo_buf[ALGO_CHANNELS][FRAMES];
    int len;
    FILE *fp = fopen(TEST_FILE, "rb");
    
    fprintf(stdout, "test_file!\n");
    
    while(fp) {
        len = fread(buf, sizeof(buf), 1, fp);
        if(len > 0) {
            algo_data_map(buf, algo_buf);
            rb_write(rb_buf, algo_buf, sizeof(algo_buf));
        } else {
            finish = 1;
            memset(algo_buf, 0, sizeof(algo_buf));
            rb_write(rb_buf, algo_buf, sizeof(algo_buf));
            break;
        }
    }
    
    
    if(fp) {
        fprintf(stdout, "test_file read quit!\n");
        fclose(fp);
    } else {
        finish = 1;
        fprintf(stdout, "test_file read quit! %s no found\n", TEST_FILE);
    }
    return 0;
}

void *algo_thread(void *p){
    int16_t algo_buf[ALGO_CHANNELS][FRAMES] = {0};
    int i, len;
    FILE *fp_ch[ALGO_CHANNELS];
    char tmp[256];
    
    fprintf(stdout, "test_file algo!\n");
    
    for(i=0; i<ALGO_CHANNELS; i++) {
        sprintf(tmp, "ch%d.raw", i);
        fp_ch[i] = fopen(tmp, "wb");
    }
    while(!finish) {
        len = rb_read(rb_buf, algo_buf, sizeof(algo_buf));
        if(len >= sizeof(algo_buf)) {
            for(i=0; i<ALGO_CHANNELS; i++) {
                fwrite(algo_buf[i], sizeof(algo_buf[i]), 1, fp_ch[i]);
            }
        }
    }
    
    for(i=0; i<ALGO_CHANNELS; i++) {
        fclose(fp_ch[i]);
    }
    
    fprintf(stdout, "test_file write quit!\n");
    return 0;
}

#elif DEBUG_MAP == 2

static volatile int finish = 0;


void *read_thread(void *p){
    int16_t buf[FRAMES][CHANNELS];
    int16_t algo_buf[ALGO_CHANNELS][FRAMES];
    int len, count;
    
    count = 0;
    while(count < 3000/16) {
        len = capture(buf, FRAMES);
        if(len >= FRAMES) {
            algo_data_map(buf, algo_buf);
            rb_write(rb_buf, algo_buf, sizeof(algo_buf));
            count++;
            //printf("count=%d\n", count);
        }
    }
    finish = 1;
    printf("read quit\n");
    return 0;
}

void *algo_thread(void *p){
    int16_t algo_buf[ALGO_CHANNELS][FRAMES] = {0};
    int16_t out_buf[FRAMES] = {0};
    int i, len;
    FILE *fp_ch[ALGO_CHANNELS];
    char tmp[256];
    
    for(i=0; i<ALGO_CHANNELS; i++) {
        sprintf(tmp, "ch%d.raw", i);
        fp_ch[i] = fopen(tmp, "wb");
    }
    
    while(!finish) {
        len = rb_read(rb_buf, algo_buf, sizeof(algo_buf));
        if(len >= sizeof(algo_buf)) {
            algo_proc((int16_t*)algo_buf, (int16_t*)out_buf);
            
            for(i=0; i<ALGO_CHANNELS; i++) {
                fwrite(algo_buf[i], sizeof(algo_buf[i]), 1, fp_ch[i]);
            }
        }
    }
    
    for(i=0; i<ALGO_CHANNELS; i++) {
        fclose(fp_ch[i]);
    }
    return 0;
}

#else

void *read_thread(void *p){
    int16_t buf[FRAMES][CHANNELS];
    int16_t algo_buf[ALGO_CHANNELS][FRAMES];
    int len;
    
    while(1) {
        len = capture(buf, FRAMES);
        if(len >= FRAMES) {
            algo_data_map(buf, algo_buf);
            rb_write(rb_buf, algo_buf, sizeof(algo_buf));
        }
    }
    return 0;
}

void *algo_thread(void *p){
    int16_t algo_buf[ALGO_CHANNELS][FRAMES] = {0};
    int16_t out_buf[FRAMES] = {0};
    int len;

    while(1) {
        len = rb_read(rb_buf, algo_buf, sizeof(algo_buf));
        if(len >= sizeof(algo_buf)) {
            algo_proc((int16_t*)algo_buf, (int16_t*)out_buf);
        }
    }
    return 0;
}

#endif
