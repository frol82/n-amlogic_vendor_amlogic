/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <dirent.h>
#include <utils/List.h>

#define LOG_TAG "modem_dongle_d"

#include "cutils/log.h"
#include "cutils/properties.h"

#include "DongleManager.h"
#include "NetlinkManager.h"

using namespace android;

struct vidpid {
    int vid;
    int pid;
    char devpath[1024];

    vidpid(){vid=0;pid=0;memset(devpath,0,sizeof(devpath));};
    ~vidpid(){};
};

typedef android::List<vidpid *> VidPidCollection;

VidPidCollection* g_vidpidList = NULL;

int is_name_begain_numbers(const char *name)
{
    if (name[0]>'9' || name[0]<'0') {
        return 0;
    }
    return 1;
}

int handle_modalias(char *cmodalias,int *p_vid, int *p_pid)
{
    char *p_end = NULL;
    char *ptr_p = NULL ;
    char *ptr_v = NULL ;

    SLOGI("handle_modalias,cmodalias is %s",cmodalias);
    ptr_v = strstr(cmodalias,":v");
    if (ptr_v != NULL) {
        ptr_p = strchr(ptr_v,'p');
        ptr_v += 2;
        *(ptr_p)='\0';
        ptr_p += 1;
        *p_vid = strtol(ptr_v, &p_end, 16);
        *(ptr_p+4) = '\0' ;
        *p_pid = strtol(ptr_p, &p_end, 16);
        return 0;
    }
    return -1;
}

int getDeviceVidPidFromBus(void)
{
    const char *pUSB = "/sys/bus/usb/devices" ;
    DIR *pDir = NULL;// /sys/bus/usb/devices/x-x:x.x, dir name is number
    FILE *pfmodalias;// /sys/bus/usb/devices/2-1\:1.0/modalias
    char cdirmodalias[1024] = "";
    char cmodalias[1024] = "";
    struct dirent *pNext_usb = NULL;
    int id =0;
    int succes = 0 ;
    int vid=0,pid=0;

    memset(cdirmodalias,'\0', sizeof(cdirmodalias));
    memset(cmodalias,'\0', sizeof(cmodalias));

    pDir = opendir(pUSB);
    if (NULL == pDir) {
        SLOGI("opendir %s failed", pUSB);
        return -1;
    }

    while (NULL != (pNext_usb= readdir(pDir))) {
        if (!is_name_begain_numbers(pNext_usb->d_name)) {
            continue;
        }

        sprintf(cdirmodalias, "%s/%s/modalias", pUSB,pNext_usb->d_name);

        if (NULL != (pfmodalias = fopen(cdirmodalias,"r"))) {
            /* we get something liks this
             *              usb:v12D1p1001d0000dc00dsc00dp00icFFiscFFipFF */
            fgets(cmodalias,1024,pfmodalias);
            fclose(pfmodalias);
            if (handle_modalias(cmodalias,&vid,&pid)<0) {
                continue;
            }else{
                vidpid *p_vp = new vidpid();
                p_vp->vid = vid;
                p_vp->pid = pid;
                snprintf(p_vp->devpath, sizeof(p_vp->devpath),"%s", cdirmodalias);
                g_vidpidList->push_back(p_vp);
            }
        }
    }
    if (NULL != pDir) {
        closedir(pDir);
    }

    return 0;
}

int main() {

    DongleManager *dm;
    NetlinkManager *nm;

    SLOGI("DongleManager 1.0 firing up");

    /* Create our singleton managers */
    if (!(dm = DongleManager::Instance())) {
        SLOGE("Unable to create DongleManager");
        exit(1);
    };

    if (!(nm = NetlinkManager::Instance())) {
        SLOGE("Unable to create NetlinkManager");
        exit(1);
    };

    if (nm->start()) {
        SLOGE("Unable to start NetlinkManager (%s)", strerror(errno));
        exit(1);
    }
    g_vidpidList = new VidPidCollection();
    getDeviceVidPidFromBus();
    if (!g_vidpidList->empty()) {
        VidPidCollection::iterator it = g_vidpidList->begin();
        vidpid *pvp = NULL;
        for (; it != g_vidpidList->end(); it++ ) {
            pvp = *it;
            SLOGI("vid:%04x, pid:%04x", pvp->vid, pvp->pid);
            if (0 == dm->switch_usbmode(pvp->vid, pvp->pid, pvp->devpath)) {
                break;
            }
        }
        sleep(30);
        for (it = g_vidpidList->begin();it != g_vidpidList->end(); it++) {
            delete *it;
        }
        delete g_vidpidList;
    }

    // Eventually we'll become the monitoring thread
    while (1) {
        sleep(1000);
    }

    SLOGI("DongleManager exiting");
    exit(0);
}
