#include "device.h"
#include "input.h"
#include "led.h"
#include "profile.h"

usbdevice keyboard[DEV_MAX];
pthread_mutex_t kblistmutex = PTHREAD_MUTEX_INITIALIZER;
usbprofile* store = 0;
int storecount = 0;

usbdevice* findusb(const char* serial){
    for(int i = 0; i < DEV_MAX; i++){
        if(keyboard[i].infifo && !strcmp(serial, keyboard[i].profile.serial))
            return keyboard + i;
    }
    return 0;
}

usbprofile* findstore(const char* serial){
    for(int i = 0; i < storecount; i++){
        usbprofile* res = store + i;
        if(!strcmp(res->serial, serial))
            return res;
    }
    return 0;
}

usbprofile* addstore(const char* serial, int autosetup){
    // Try to find the device before adding it
    usbprofile* res = findstore(serial);
    if(res)
        return res;
    // Add device to the list
    store = realloc(store, ++storecount * sizeof(usbprofile));
    res = store + storecount - 1;
    // Initialize device
    memset(res, 0, sizeof(*res));
    if(autosetup)
        eraseprofile(res, 1);
    strcpy(res->serial, serial);
    genid(&res->id);
    return res;
}

void setinput(usbdevice* kb, int input){
    // Set input mode on the keys. 0x80 generates a normal HID interrupt, 0x40 generates a proprietary interrupt. 0xc0 generates both.
    uchar datapkt[6][MSG_SIZE] = { };
    for(int i = 0; i < 5; i++){
        datapkt[i][0] = 0x07;
        datapkt[i][1] = 0x40;
        datapkt[i][2] = 30;
    }
    datapkt[4][2] = 24;
    datapkt[5][0] = 0x07;
    datapkt[5][1] = 0x05;
    datapkt[5][2] = 0x02;
    datapkt[5][4] = 0x03;
    for(int i = 0; i < 30; i++){
        int key = i;
        datapkt[0][i * 2 + 4] = key;
        datapkt[0][i * 2 + 5] = input;
    }
    for(int i = 0; i < 30; i++){
        int key = i + 30;
        datapkt[1][i * 2 + 4] = key;
        datapkt[1][i * 2 + 5] = input;
    }
    for(int i = 0; i < 30; i++){
        int key = i + 60;
        datapkt[2][i * 2 + 4] = key;
        datapkt[2][i * 2 + 5] = input;
    }
    for(int i = 0; i < 30; i++){
        int key = i + 90;
        datapkt[3][i * 2 + 4] = key;
        datapkt[3][i * 2 + 5] = input;
    }
    for(int i = 0; i < 24; i++){
        int key = i + 120;
        datapkt[4][i * 2 + 4] = key;
        // Set the MR button to toggle the MR ring (0x9)
        datapkt[4][i * 2 + 5] = input | (!strcmp(keymap_system[key].name, "mr") ? 0x9 : 0);
    }
#undef IMASK
    usbqueue(kb, datapkt[0], 6);
}
