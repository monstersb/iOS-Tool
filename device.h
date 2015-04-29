//
//  device.h
//  iOS Hijacker
//
//  Created by Monster on 4/8/15.
//  Copyright (c) 2015 Monster. All rights reserved.
//

#ifndef __iOS_Hijacker__device__
#define __iOS_Hijacker__device__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <set>
#include <list>

#endif /* defined(__iOS_Hijacker__device__) */


#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/house_arrest.h>
#include <libimobiledevice/screenshotr.h>
#include <libimobiledevice/sbservices.h>
#include <libimobiledevice/notification_proxy.h>
#include <libimobiledevice/file_relay.h>
#include <libimobiledevice/mobilebackup2.h>
#include "helper.h"

#ifdef DEVICE_H
#else

using namespace std;

enum DeviceStatus {
    StatusOK,
    StatusError
};

enum FileType {
    TypeFile,
    TypeDirectory,
    TypeInvalid
};

extern string FRSource[];

enum FileRelaySource {
    FRAppleSupport,
    FRNetwork,
    FRVPN,
    FRWiFi,
    FRUserDatabases,
    FRCrashReporter,
    FRTmp,
    FRSystemConfiguration
};


class Device {
private:
    DeviceStatus mStatus;
    string mUdid;
    idevice_t mDevice;
    lockdownd_client_t mLockDown;
    instproxy_client_t mInstProxy;
    screenshotr_client_t mScreenShotr;
    string mName;
    set<string> mApps;
    house_arrest_client_t mHouseArrestClient;
    sbservices_client_t mSb;
    np_client_t mNp;
    file_relay_client_t mFileRelay;

public:
    Device(const char *udid);
    ~Device();
    DeviceStatus status();
    string name();
    set<string> apps();
    DeviceStatus getDirectory(string appId, string path, list<string> &fileList);
    DeviceStatus readFile(string appId, string path, char *&buffer, int &bytesRead);
    uint64_t getFileLength(string appId, string path);
    FileType getFileType(string appId, string path);
    DeviceStatus pullFile(string appId, string srcPath, string DstPath);
    DeviceStatus pullDirectory(string appId, string srcPath, string DstPath);
    DeviceStatus takeScreenShotr(string path);
    DeviceStatus getIcon(string appId, string path);
    DeviceStatus postNotification(string notification);
    DeviceStatus fileRelayRequest(FileRelaySource source);
    DeviceStatus backup();
    
private:
    Device();
    DeviceStatus getName();
    DeviceStatus getApps();
    house_arrest_client_t getHouseArrestClient();
    afc_client_t getAfcFromApp(string appId);
    DeviceStatus getDirectoryWithAfc(afc_client_t afc, string path, list<string> &fileList);
    DeviceStatus readFileWithAfc(string path, afc_client_t afc, char *&buffer, int &bytesRead);
    uint64_t getFileLengthWithAfc(string path, afc_client_t afc);
    FileType getFileTypeWithAfc(afc_client_t afc, string path);
    DeviceStatus pullFileWithAfc(afc_client_t afc, string srcPath, string DstPath);
    DeviceStatus pullDirectoryWithAfc(afc_client_t afc, string srcPath, string DstPath);
};
#endif

#define DEVICE_H