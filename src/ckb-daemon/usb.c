#include "device.h"
#include "devnode.h"
#include "firmware.h"
#include "input.h"
#include "led.h"
#include "notify.h"
#include "profile.h"
#include "usb.h"

// Mask of features to exclude from all devices
int features_mask = -1;

// OS-specific USB reset
extern int os_resetusb(usbdevice* kb, const char* file, int line);

int usbqueue(usbdevice* kb, uchar* messages, int count){
    // Don't add messages unless the queue has enough room for all of them
    if(!kb->handle || kb->queuecount + count > QUEUE_LEN)
        return -1;
    for(int i = 0; i < count; i++)
        memcpy(kb->queue[kb->queuecount + i], messages + MSG_SIZE * i, MSG_SIZE);
    kb->queuecount += count;
    return 0;
}

int setupusb(usbdevice* kb, short vendor, short product){
    kb->model = (product == P_K65) ? 65 : (product == P_K70 || product == P_K70_NRGB) ? 70 : 95;
    kb->vendor = vendor;
    kb->product = product;
    kb->features = (IS_RGB(vendor, product) ? FEAT_STD_RGB : FEAT_STD_NRGB) & features_mask;
    // Make up a device name if one wasn't assigned
    if(!kb->name[0])
        snprintf(kb->name, NAME_LEN, "Corsair K%d%s", kb->model, HAS_FEATURES(kb, FEAT_RGB) ? " RGB" : "");
    pthread_mutex_init(&kb->mutex, 0);
    pthread_mutex_init(&kb->keymutex, 0);
    pthread_mutex_lock(&kb->mutex);

    // Make /dev path
    if(makedevpath(kb)){
        pthread_mutex_unlock(&kb->mutex);
        pthread_mutex_destroy(&kb->mutex);
        pthread_mutex_destroy(&kb->keymutex);
        return -1;
    }

    // Set up an input device for key events
    if(!inputopen(kb)){
        rmdevpath(kb);
        pthread_mutex_unlock(&kb->mutex);
        pthread_mutex_destroy(&kb->mutex);
        pthread_mutex_destroy(&kb->keymutex);
        return -1;
    }

    // Create the USB queue
    for(int q = 0; q < QUEUE_LEN; q++)
        kb->queue[q] = malloc(MSG_SIZE);

    if(strstr(kb->name, "Bootloader")){
        // Device needs a firmware update. Finish setting up but don't do anything.
        printf("Device needs a firmware update. Please issue a fwupdate command.\n");
        kb->features = FEAT_RGB;
        kb->fwversion = 0;
        kb->pollrate = -1;
        kb->profile.keymap = keymap_system;
        kb->profile.currentmode = getusbmode(0, &kb->profile, keymap_system);
        getusbmode(1, &kb->profile, keymap_system);
        getusbmode(2, &kb->profile, keymap_system);
        return 0;
    }

    updateindicators(kb, 1);
    // Nothing else needs to be done for non-RGB keyboards
    if(!HAS_FEATURES(kb, FEAT_RGB)){
        writefwnode(kb);
        kb->profile.keymap = keymap_system;
        // Fill out RGB features for consistency, even though the keyboard doesn't have them
        kb->pollrate = -1;
        kb->profile.currentmode = getusbmode(0, &kb->profile, keymap_system);
        if(kb->model == 95){
            getusbmode(1, &kb->profile, keymap_system);
            getusbmode(2, &kb->profile, keymap_system);
        }
        return 0;
    }

    // Get the firmware version from the device
    int fail = !!getfwversion(kb);

    // Put the M-keys (K95) as well as the Brightness/Lock keys into software-controlled mode.
    // This packet disables their hardware-based functions.
    uchar msg[MSG_SIZE] = { 0x07, 0x04, 0x02, 0 };
    usbqueue(kb, msg, 1);

    // Wait a little bit and then send this message.
    // The keyboard doesn't always respond immediately.
    DELAY_LONG;
    if(!usbdequeue(kb))
        fail = 1;

    // Set all keys to use the Corsair input. HID input is unused.
    setinput(kb, IN_CORSAIR | IN_HID);

    while(kb->queuecount > 0){
        DELAY_SHORT;
        if(!usbdequeue(kb)){
            fail = 1;
            break;
        }
    }

    // Restore profile (if any)
    DELAY_LONG;
    usbprofile* store = findstore(kb->profile.serial);
    if(store){
        memcpy(&kb->profile, store, sizeof(usbprofile));
        if(kb->model == 95){
            // On the K95, make sure at least 3 modes are available
            getusbmode(1, &kb->profile, keymap_system);
            getusbmode(2, &kb->profile, keymap_system);
        }
        if(hwloadprofile(kb, 0))
            return -2;
    } else {
        // If there is no profile, load it from the device
        kb->profile.keymap = keymap_system;
        kb->profile.currentmode = getusbmode(0, &kb->profile, keymap_system);
        if(kb->model == 95){
            getusbmode(1, &kb->profile, keymap_system);
            getusbmode(2, &kb->profile, keymap_system);
        }
        if(hwloadprofile(kb, 1))
            return -2;
    }
    DELAY_SHORT;
    if(fail)
        return -2;
    updatergb(kb, 1);

    return 0;
}

int revertusb(usbdevice* kb){
    if(!HAS_FEATURES(kb, FEAT_RGB))
        return 0;
    // Empty the USB queue first
    while(kb->queuecount > 0){
        DELAY_SHORT;
        if(usbdequeue(kb) <= 0)
            return -1;
    }
    DELAY_MEDIUM;
    setinput(kb, IN_HID);
    while(kb->queuecount > 0){
        DELAY_SHORT;
        if(usbdequeue(kb) <= 0)
            return -1;
    }
    // Set the M-keys back into hardware mode and restore the RGB profile. It has to be sent twice for some reason.
    uchar msg[MSG_SIZE] = { 0x07, 0x04, 0x01, 0 };
    usbqueue(kb, msg, 1);
    usbqueue(kb, msg, 1);
    // Flush the USB queue
    while(kb->queuecount > 0){
        DELAY_MEDIUM;
        if(usbdequeue(kb) <= 0)
            return -1;
    }
    return 0;
}

int _resetusb(usbdevice* kb, const char* file, int line){
    // Perform a USB reset
    DELAY_LONG;
    int res = os_resetusb(kb, file, line);
    if(res)
        return res;
    DELAY_LONG;
    // Empty the queue. Re-send the firmware message as well as the input messages.
    kb->queuecount = 0;
    if(getfwversion(kb))
        return -1;
    uchar msg[MSG_SIZE] = { 0x07, 0x04, 0x02, 0 };
    usbqueue(kb, msg, 1);
    setinput(kb, IN_CORSAIR | IN_HID);
    // If the hardware profile hasn't been loaded yet, load it here
    res = 0;
    if(!kb->hw){
        if(findstore(kb->profile.serial))
            res = hwloadprofile(kb, 0);
        else
            res = hwloadprofile(kb, 1);
    }
    updatergb(kb, 1);
    return res ? -1 : 0;
}

#define MAX_TRIES 25
int usb_tryreset(usbdevice* kb){
    printf("USB failure. ");
    for(int try = 0; try < MAX_TRIES; try++){
        printf("Attempting reset (%d of %d)...\n", try + 1, MAX_TRIES);
        DELAY_LONG;
        int res = resetusb(kb);
        if(!res){
            printf("Reset success\n");
            return 0;
        }
        if(res == -2)
            break;
    }
    printf("Reset failed. Disconnecting.\n");
    return -1;
}

int closeusb(usbdevice* kb){
    // Close file handles
    if(!kb->infifo)
        return 0;
    pthread_mutex_lock(&kb->keymutex);
    if(kb->handle){
        printf("Disconnecting %s (S/N: %s)\n", kb->name, kb->profile.serial);
        inputclose(kb);
        updateconnected();
        // Delete USB queue
        for(int i = 0; i < QUEUE_LEN; i++)
            free(kb->queue[i]);
        // Move the profile data into the device store (unless it wasn't set due to needing a firmware update)
        if(kb->fwversion == 0)
            freeprofile(&kb->profile);
        else {
            usbprofile* store = addstore(kb->profile.serial, 0);
            memcpy(store, &kb->profile, sizeof(usbprofile));
        }
        // Close USB device
        closehandle(kb);
        notifyconnect(kb, 0);
    } else
        updateconnected();
    // Delete the control path
    rmdevpath(kb);

    pthread_mutex_unlock(&kb->keymutex);
    pthread_mutex_destroy(&kb->keymutex);
    pthread_mutex_unlock(&kb->mutex);
    pthread_mutex_destroy(&kb->mutex);
    memset(kb, 0, sizeof(usbdevice));
    return 0;
}
