//
//  main.cpp
//  iOS Hijacker
//
//  Created by Monster on 4/8/15.
//  Copyright (c) 2015 Monster. All rights reserved.
//

#include <iostream>
#include <unistd.h>

#include <libimobiledevice/libimobiledevice.h>

#include "helper.h"
#include "device.h"
#include "test.h"

using namespace std;

void listener(const idevice_event_t *event, void *userdata) {
    if (event->event == idevice_event_type::IDEVICE_DEVICE_ADD) {
        Device device(event->udid);
        if (device.status() == DeviceStatus::StatusOK) {
            log(LogLevel::Information, "[%s] plug in ", device.name().c_str());
            device.backup();
        } else {
            log(LogLevel::Information, "Unhandled device [%s]", event->udid);
        }
    } else if (event->event == idevice_event_type::IDEVICE_DEVICE_REMOVE) {
        log(LogLevel::Information, "device pull out");
    }
}

int main(int argc, const char * argv[]) {
    idevice_event_subscribe(listener, NULL);
    pause();
    return 0;
}
