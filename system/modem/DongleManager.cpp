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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <dirent.h>

#include <linux/kdev_t.h>

#define LOG_TAG "modem_dongle_d"

#include <openssl/md5.h>

#include <cutils/fs.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#include <sysutils/NetlinkEvent.h>
#include "DongleManager.h"

static char MASS_STORAGE_CLASS_ID = '8';

DongleManager *DongleManager::sInstance = NULL;

DongleManager *DongleManager::Instance() {
    if (!sInstance)
        sInstance = new DongleManager();
    return sInstance;
}

int DongleManager::switch_usbmode(int vid, int pid, const char *devpath)
{
    int ret=mUsbMdmMgr->switchMode(vid,pid,devpath);
    return ret;
}

void DongleManager::handleUsbEvent(NetlinkEvent *evt)
{
    const char *devpath = evt->findParam("DEVPATH");
    const char *devtype = evt->findParam("DEVTYPE");
    const char *productStr = evt->findParam("PRODUCT");
    const char *interfaceStr = evt->findParam("INTERFACE");

    int vid = 0 ;
    int pid = 0;
    int ret = -1 ;

    //we just judge the usb interface as a ums device
    if (!devtype || !productStr || !interfaceStr)
        return;

    if (strcmp(devtype, "usb_interface"))
        return;

    //if it is mass storage interface
    if (interfaceStr[0] == MASS_STORAGE_CLASS_ID) {
        sscanf(productStr, "%x/%x/", &vid, &pid);
        if (NetlinkEvent::NlActionAdd == evt->getAction()) {
            if (0 <= (ret=mUsbMdmMgr->switchMode(vid,pid,devpath))) {
                return ;
            }
        } else if (NetlinkEvent::NlActionRemove == evt->getAction()) {
            mUsbMdmMgr->delMdmDev(vid, pid);
        }
    }
    else if (interfaceStr != NULL ) {
        sscanf(productStr, "%x/%x/", &vid, &pid);

        if (NetlinkEvent::NlActionAdd == evt->getAction()) {
            mUsbMdmMgr->addMdmDev(vid, pid);
        } else if (NetlinkEvent::NlActionRemove == evt->getAction()) {
            mUsbMdmMgr->delMdmDev(vid, pid);
        }
    }
}
