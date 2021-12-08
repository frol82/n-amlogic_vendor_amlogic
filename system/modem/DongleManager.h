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
#ifndef _DONGLEMANAGER_H
#define _DONGLEMANAGER_H

#include <pthread.h>
#include <sysutils/NetlinkEvent.h>
#include "UsbMdmMgr.h"

class DongleManager {
private:
    static DongleManager *sInstance;

private:
    UsbMdmMgr * mUsbMdmMgr;
    DongleManager(){mUsbMdmMgr = new UsbMdmMgr();};

public:
    ~DongleManager(){delete mUsbMdmMgr;};
    void handleUsbEvent(NetlinkEvent *evt);
    int switch_usbmode(int vid, int pid, const char *devpath);

    static DongleManager *Instance();
};

#endif // _DONGLEMANAGER_H
