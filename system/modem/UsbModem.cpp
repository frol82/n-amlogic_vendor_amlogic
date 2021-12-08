/*
 * Copyright (C) 2011 Amlogic, Inc.
 *
 *  UsbModem
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
 *
 *  Author:  Frank Chen <frank.chen@amlogic.com>
 */

#include <string.h>
#include <malloc.h>

#include "UsbModem.h"

UsbModem::UsbModem(  int vid, int pid ) {
    m_vid = vid;
    m_pid = pid;
}

UsbModem::~UsbModem() {
}

int UsbModem::getVid() {
    return m_vid;
}

int UsbModem::getPid() {
    return m_pid;
}


