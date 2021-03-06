#include "../ckb/ckb-anim.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

void ckb_info(){
    // Plugin info
    CKB_NAME("Raindrop");
    CKB_VERSION("0.7");
    CKB_COPYRIGHT("2014-2015", "MSC");
    CKB_LICENSE("GPLv2");
    CKB_GUID("{5D6695AF-0496-41E2-BEE7-F7D0ABAA49E9}");
    CKB_DESCRIPTION("A rain-like effect.");

    // Effect parameters
    CKB_PARAM_RGB("color", "Drop color:", "", 255, 255, 255);
    CKB_PARAM_DOUBLE("intensity", "Intensity:", "%", 100, 0, 100);
    CKB_PARAM_DOUBLE("size", "Size:", "%", 25, 1, 100);
    CKB_PARAM_LONG("frequency", "Frequency:", "per second", 10, 1, 100);

    // Timing/input parameters
    CKB_KPMODE(CKB_KP_POSITION);
    CKB_TIMEMODE(CKB_TIME_ABSOLUTE);
    CKB_REPEAT(FALSE);
    CKB_LIVEPARAMS(TRUE);
    CKB_DEFAULT_TRIGGER(TRUE);
    CKB_DEFAULT_TRIGGER_KP(TRUE);
}

#define DROP_MAX 1000

int spawn = 0;
double period = 0.1;
double maxsize = 100.;
double speed = 100.;
unsigned char ar = 255, ag = 255, ab = 255;
float aa = 1.;
struct {
    int active;
    float x, y;
    float size;
    float msize;
} drop[DROP_MAX];

void drop_add(float x, float y, int slow){
    for(int i = 0; i < DROP_MAX; i++){
        if(drop[i].active)
            continue;
        drop[i].active = 1;
        drop[i].x = x;
        drop[i].y = y;
        float msize = maxsize * (0.9 + (rand() / (double)RAND_MAX * 0.2));
        drop[i].size = -msize / 2. * slow;
        drop[i].msize = msize;
        return;
    }
}

void ckb_init(ckb_runctx* context){
    srand(time(NULL));
}

void ckb_parameter(ckb_runctx* context, const char* name, const char* value){
    CKB_PARSE_RGB("color", &ar, &ag, &ab){}
    double intensity, size;
    long frequency;
    CKB_PARSE_DOUBLE("intensity", &intensity){
        aa = intensity / 100.;
    }
    CKB_PARSE_DOUBLE("size", &size){
        double sizex = context->width / 2.;
        double sizey = context->height / 2.;
        maxsize = size / 100. * sqrt(sizex * sizex + sizey * sizey);
        speed = maxsize;
    }
    CKB_PARSE_LONG("frequency", &frequency){
        period = 1. / frequency;
    }
    CKB_PARSE_BOOL("trigger", &spawn){}
}

void ckb_keypress(ckb_runctx* context, ckb_key* key, int x, int y, int state){
    if(state)
        drop_add(x, y, 0);
}

void ckb_start(ckb_runctx* context){
}

double tick = 0.;

int ckb_frame(ckb_runctx* context, double delta){
    if(delta <= 0.)
        return 0;
    CKB_KEYCLEAR(context);
    tick += delta;
    if(tick > period && spawn){
        drop_add(rand() / (double)RAND_MAX * context->width, rand() / (double)RAND_MAX * context->height, 1);
        tick -= period;
    }
    for(unsigned i = 0; i < DROP_MAX; i++){
        if(drop[i].active){
            drop[i].size += delta * speed;
            if(drop[i].size > drop[i].msize){
                drop[i].active = 0;
                continue;
            }
            unsigned count = context->keycount;
            ckb_key* keys = context->keys;
            for(ckb_key* key = keys; key < keys + count; key++){
                float distance = drop[i].size - sqrt(pow(key->x - drop[i].x, 2.f) + pow(key->y - drop[i].y, 2.f));
                if(distance < 0.)
                    distance = -distance / 2.;
                float scale = drop[i].size / 4.f;
                if(scale < 10.)
                    scale = 10.;
                distance /= scale;
                if(distance <= 1.){
                    float ascale = (1.f - drop[i].size / drop[i].msize);
                    if(ascale > 1.f)
                        ascale = 1.f;
                    ckb_alpha_blend(key, (1.f - distance) * ascale * aa * 255., ar, ag, ab);
                }
            }
        }
    }
    return 0;
}
