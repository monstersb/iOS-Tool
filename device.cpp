//
//  device.cpp
//  iOS Hijacker
//
//  Created by Monster on 4/8/15.
//  Copyright (c) 2015 Monster. All rights reserved.
//

#include "device.h"

string FRSource[] = {
    "AppleSupport",
    "Network",
    "VPN",
    "WiFi",
    "UserDatabases",
    "CrashReporter",
    "tmp",
    "SystemConfiguration"
};

Device::Device() {
    this->mDevice = NULL;
    this->mLockDown = NULL;
    this->mInstProxy = NULL;
    this->mHouseArrestClient = NULL;
    this->mScreenShotr = NULL;
    this->mSb = NULL;
    this->mNp = NULL;
    this->mFileRelay = NULL;
    this->mStatus = StatusOK;
}

Device::Device(const char *udid) {
    new (this)Device();
    this->mUdid = udid;
    if (idevice_new(&this->mDevice, this->mUdid.c_str()) != IDEVICE_E_SUCCESS) {
        this->mStatus = StatusError;
        return;
    }
    if (lockdownd_client_new_with_handshake(this->mDevice, &this->mLockDown, NULL) != LOCKDOWN_E_SUCCESS) {
        this->mStatus = StatusError;
        return;
    }
    if (instproxy_client_start_service(this->mDevice, &this->mInstProxy, NULL) != INSTPROXY_E_SUCCESS) {
        this->mStatus = StatusError;
        return;
    }
    if (this->getName() != DeviceStatus::StatusOK) {
        this->mStatus = StatusError;
        return;
    }
    if (this->getApps() != DeviceStatus::StatusOK) {
        this->mStatus = StatusError;
        return;
    }
}

Device::~Device() {
    if (this->mInstProxy != NULL) {
        instproxy_client_free(this->mInstProxy);
    }
    if (this->mLockDown != NULL) {
        lockdownd_goodbye(this->mLockDown);
    }
    if (this->mDevice != NULL) {
        idevice_free(this->mDevice);
    }
    if (this->mScreenShotr != NULL) {
        screenshotr_client_free(this->mScreenShotr);
    }
    if (this->mSb != NULL) {
        sbservices_client_free(this->mSb);
    }
    if (this->mNp != NULL) {
        np_client_free(this->mNp);
    }
    if (this->mFileRelay != NULL) {
        file_relay_client_free(this->mFileRelay);
    }
}

DeviceStatus Device::getName() {
    char *name = NULL;
    lockdownd_get_device_name(this->mLockDown, &name);
    this->mName = name;
    free(name);
    return StatusOK;
}

DeviceStatus Device::getApps() {
    plist_t apps = NULL;
    plist_t opts;
    opts = instproxy_client_options_new();
    instproxy_client_options_add(opts, "ApplicationType", "Any", NULL);
    if (instproxy_browse(this->mInstProxy, opts, &apps) != INSTPROXY_E_SUCCESS) {
        instproxy_client_options_free(opts);
        return StatusError;
    }
    int count = plist_array_get_size(apps) / 2;
    for (int i = 0; i < count; i++) {
        plist_t app;
        app = plist_array_get_item(apps, i);
        plist_t appId;
        appId = plist_dict_get_item(app, "CFBundleIdentifier");
        char *sAppId;
        plist_get_string_val(appId, &sAppId);
        this->mApps.insert(sAppId);
        //cout << i << "\t" << sAppId << endl;
        free(sAppId);
        plist_free(appId);
        plist_free(app);
    }
    plist_free(apps);
    instproxy_client_options_free(opts);
    return StatusOK;
}


house_arrest_client_t Device::getHouseArrestClient() {
    house_arrest_client_t client = NULL;
    if (house_arrest_client_start_service(this->mDevice, &client, NULL) != HOUSE_ARREST_E_SUCCESS) {
        return NULL;
    }
    this->mHouseArrestClient = client;
    return client;
}

afc_client_t Device::getAfcFromApp(string appId) {
    afc_client_t afc = NULL;
    if (appId.empty()) {
        if (afc_client_start_service(this->mDevice, &afc, NULL)  != AFC_E_SUCCESS) {
            return NULL;
        }
        return afc;
    }
    this->mHouseArrestClient = this->getHouseArrestClient();
    if (this->mHouseArrestClient == NULL) {
        return NULL;
    }
    if (house_arrest_send_command(this->mHouseArrestClient, "VendContainer", appId.c_str()) != HOUSE_ARREST_E_SUCCESS) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return NULL;
    }
    plist_t result = NULL;
    if (house_arrest_get_result(this->mHouseArrestClient, &result) != HOUSE_ARREST_E_SUCCESS) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return NULL;
    }
    plist_t status = NULL;
    status = plist_dict_get_item(result, "Status");
    if (status == NULL) {
        plist_free(result);
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return NULL;
    }
    char *sStatus = NULL;
    plist_get_string_val(status, &sStatus);
    plist_free(status);
    plist_free(result);
    if (strcmp(sStatus, "Complete")) {
        free(sStatus);
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return NULL;
    }
    free(sStatus);
    if (afc_client_new_from_house_arrest_client(this->mHouseArrestClient, &afc) != AFC_E_SUCCESS) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return NULL;
    }
    return afc;
}

DeviceStatus Device::getDirectoryWithAfc(afc_client_t afc, string path, list<string> &fileList) {
    char **dir = NULL;
    if (afc_read_directory(afc, path.c_str(), &dir) != AFC_E_SUCCESS) {
        afc_client_free(afc);
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return StatusError;
    }
    for (int i = 0; dir[i]; i++) {
        if (strcmp(dir[i], ".") == 0 || strcmp(dir[i], "..") == 0) {
            continue;
        }
        //cout << dir[i] << endl;
        fileList.push_back(dir[i]);
    }
    afc_dictionary_free(dir);
    return StatusOK;
}

DeviceStatus Device::getDirectory(string appId, string path, list<string> &fileList) {
    afc_client_t afc = this->getAfcFromApp(appId);
    if (afc == NULL) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return StatusError;
    }
    DeviceStatus status = this->getDirectoryWithAfc(afc, path, fileList);
    afc_client_free(afc);
    house_arrest_client_free(this->mHouseArrestClient);
    this->mHouseArrestClient = NULL;
    return status;
}


uint64_t Device::getFileLengthWithAfc(string path, afc_client_t afc) {
    uint64_t handle = 0;
    if (afc_file_open(afc, path.c_str(), afc_file_mode_t::AFC_FOPEN_RDONLY, &handle) != AFC_E_SUCCESS) {
        return 0;
    }
    if (afc_file_seek(afc, handle, 0, SEEK_END) != AFC_E_SUCCESS) {
        afc_file_close(afc, handle);
        return 0;
    }
    uint64_t length = 0;
    if (afc_file_tell(afc, handle, &length) != AFC_E_SUCCESS) {
        afc_file_close(afc, handle);
        return 0;
    }
    afc_file_close(afc, handle);
    return length;
}

uint64_t Device::getFileLength(string appId, string path) {
    afc_client_t afc = this->getAfcFromApp(appId);
    if (afc == NULL) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return 0;
    }
    uint64_t length = this->getFileLengthWithAfc(path, afc);
    afc_client_free(afc);
    house_arrest_client_free(this->mHouseArrestClient);
    this->mHouseArrestClient = NULL;
    return length ;
}

FileType Device::getFileTypeWithAfc(afc_client_t afc, string path) {
    uint64_t handle = 0;
    if (afc_file_open(afc, path.c_str(), afc_file_mode_t::AFC_FOPEN_RDONLY, &handle) != AFC_E_SUCCESS) {
        return TypeInvalid;
    }
    afc_file_close(afc, handle);
    char **dir = NULL;
    if (afc_read_directory(afc, path.c_str(), &dir) != AFC_E_SUCCESS) {
        return TypeFile;
    }
    afc_dictionary_free(dir);
    return TypeDirectory;
}

FileType Device::getFileType(string appId, string path) {
    afc_client_t afc = this->getAfcFromApp(appId);
    if (afc == NULL) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return TypeInvalid;
    }
    FileType type = this->getFileTypeWithAfc(afc, path);
    afc_client_free(afc);
    house_arrest_client_free(this->mHouseArrestClient);
    this->mHouseArrestClient = NULL;
    return type;
}

DeviceStatus Device::readFileWithAfc(string path, afc_client_t afc, char *&buffer, int &bytesRead) {
    buffer = NULL;
    bytesRead = 0;
    FileType type = this->getFileTypeWithAfc(afc, path);
    if (type == TypeInvalid) {
        return StatusError;
    } else if (type == TypeDirectory) {
        return StatusError;
    }
    int64_t length = this->getFileLengthWithAfc(path, afc);
    if (length == 0) {
        return StatusOK;
    }
    uint64_t handle = 0;
    if (afc_file_open(afc, path.c_str(), afc_file_mode_t::AFC_FOPEN_RDONLY, &handle) != AFC_E_SUCCESS) {
        return StatusError;
    }
    buffer = new char[length];
    if (buffer == NULL) {
        afc_file_close(afc, handle);
        return StatusError;
    }
    uint32_t tBytesRead;
    for (; bytesRead < length; bytesRead += tBytesRead) {
        if (afc_file_read(afc, handle, buffer + bytesRead, 0x400000, &tBytesRead) != AFC_E_SUCCESS) {
            bytesRead += tBytesRead;
            break;
        }
    }
    afc_file_close(afc, handle);
    return StatusOK;
}

DeviceStatus Device::readFile(string appId, string path, char *&buffer, int &bytesRead) {
    afc_client_t afc = this->getAfcFromApp(appId);
    if (afc == NULL) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return StatusError;
    }
    DeviceStatus status = this->readFileWithAfc(path, afc, buffer, bytesRead);
    afc_client_free(afc);
    house_arrest_client_free(this->mHouseArrestClient);
    this->mHouseArrestClient = NULL;
    return status;
}

DeviceStatus Device::pullFileWithAfc(afc_client_t afc, string srcPath, string DstPath) {
    int length;
    char *buffer;
    if (this->readFileWithAfc(srcPath, afc, buffer, length) != StatusOK) {
        return StatusError;
    }
    if (createLocalFile(DstPath, buffer, length) == -1) {
        if (length > 0) {
            free(buffer);
        }
        return StatusError;
    }
    if (length > 0) {
        free(buffer);
    }
    return StatusOK;
}

DeviceStatus Device::pullFile(string appId, string srcPath, string DstPath) {
    afc_client_t afc = this->getAfcFromApp(appId);
    if (afc == NULL) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return StatusError;
    }
    DeviceStatus status = this->pullFileWithAfc(afc, srcPath, DstPath);
    afc_client_free(afc);
    house_arrest_client_free(this->mHouseArrestClient);
    this->mHouseArrestClient = NULL;
    return status;
}

DeviceStatus Device::pullDirectory(string appId, string srcPath, string DstPath) {
    afc_client_t afc = this->getAfcFromApp(appId);
    if (afc == NULL) {
        house_arrest_client_free(this->mHouseArrestClient);
        this->mHouseArrestClient = NULL;
        return StatusError;
    }
    DeviceStatus status = this->pullDirectoryWithAfc(afc, srcPath, DstPath);
    afc_client_free(afc);
    house_arrest_client_free(this->mHouseArrestClient);
    this->mHouseArrestClient = NULL;
    return status;
}

DeviceStatus Device::pullDirectoryWithAfc(afc_client_t afc, string srcPath, string dstPath) {
    cout << srcPath << endl;
    list<string> dir;
    list<string>::iterator it;
    this->getDirectoryWithAfc(afc, srcPath, dir);
    for (it = dir.begin(); it != dir.end(); it++) {
        string tSrcPath, tDstPath;
        if (srcPath.length() > 0 && srcPath[srcPath.length() - 1] != '/') {
            tSrcPath = srcPath + "/" + *it;
        } else {
            tSrcPath = srcPath + *it;
        }
        if (dstPath.length() > 0 && dstPath[dstPath.length() - 1] != '/') {
            tDstPath = dstPath + "/" + *it;
        } else {
            tDstPath = dstPath + *it;
        }
        FileType type = this->getFileTypeWithAfc(afc, tSrcPath);
        if (type == TypeInvalid) {
            return StatusError;
        } else if (type == TypeFile) {
            this->pullFileWithAfc(afc, tSrcPath, tDstPath);
        } else if (type == TypeDirectory) {
            this->pullDirectoryWithAfc(afc, tSrcPath, tDstPath);
            continue;
        }
    }
    return StatusOK;
}

DeviceStatus Device::takeScreenShotr(string path) {
    if (this->mScreenShotr == NULL) {
        if (screenshotr_client_start_service(this->mDevice, &this->mScreenShotr, NULL) != SCREENSHOTR_E_SUCCESS) {
            return StatusError;
        }
    }
    char *buffer = NULL;
    uint64_t length = 0;
    if (screenshotr_take_screenshot(this->mScreenShotr, &buffer, &length) != SCREENSHOTR_E_SUCCESS) {
        return StatusError;
    }
    if (createLocalFile(path, buffer, (int)length) != 0) {
        free(buffer);
        return StatusError;
    }
    free(buffer);
    return StatusOK;
}

DeviceStatus Device::getIcon(string appId, string path) {
    if (this->mSb == NULL) {
        if (sbservices_client_start_service(this->mDevice, &this->mSb, NULL) != SBSERVICES_E_SUCCESS) {
            return StatusError;
        }
    }
    char *buffer = NULL;
    uint64_t length = 0;
    if (sbservices_get_icon_pngdata(this->mSb, appId.c_str(), &buffer, &length) != SBSERVICES_E_SUCCESS) {
        return StatusError;
    }
    if (createLocalFile(path, buffer, (int)length) != 0) {
        free(buffer);
        return StatusError;
    }
    free(buffer);
    return StatusOK;
}

DeviceStatus Device::postNotification(string notification) {
    if (this->mSb == NULL) {
        if (np_client_start_service(this->mDevice, &this->mNp, NULL) != NP_E_SUCCESS) {
            return StatusError;
        }
    }
    np_error_t status = np_post_notification(this->mNp, notification.c_str());
    if (status != NP_E_SUCCESS) {
        return StatusError;
    }
    return StatusOK;
}

DeviceStatus Device::fileRelayRequest(FileRelaySource source) {
    if (this->mFileRelay == NULL) {
        if (file_relay_client_start_service(this->mDevice, &this->mFileRelay, NULL) != FILE_RELAY_E_SUCCESS) {
            return StatusError;
        }
    }
    char const * tSource[2] = {
        FRSource[(int)source].c_str(),
        NULL
    };
    idevice_connection_t conn = NULL;
    if (file_relay_request_sources(this->mFileRelay, (char const**)tSource, &conn) != FILE_RELAY_E_SUCCESS) {
        return StatusError;
    }
    return StatusOK;
}


void np_notify_cb (const char *notification, void *user_data) {
    
}

DeviceStatus Device::backup() {
    mobilebackup2_client_t mb_client = NULL;
    DeviceStatus status = StatusError;
    if (mobilebackup2_client_start_service(this->mDevice, &mb_client, NULL) == MOBILEBACKUP2_E_SUCCESS) {
        afc_client_t afc = this->getAfcFromApp("");
        if (afc) {
            uint64_t handle;
            if (afc_file_open(afc, "/com.apple.itunes.lock_sync", AFC_FOPEN_RW, &handle) == AFC_E_SUCCESS
                && afc_file_lock(afc, handle, AFC_LOCK_EX) == AFC_E_SUCCESS) {
                if (mobilebackup2_send_request(mb_client, "Backup", this->mUdid.c_str(), this->mUdid.c_str(), NULL) == MOBILEBACKUP2_E_SUCCESS) {
                    plist_t msg = NULL;
                    char *dl = NULL;
                    if (mobilebackup2_receive_message(mb_client, &msg, &dl) == MOBILEBACKUP2_E_SUCCESS) {
                        plist_t files = plist_array_get_item(msg, 1);
                        int count = plist_array_get_size(files);
                        
                        for (int i = 0; i < count; i++) {
                            plist_t val = plist_array_get_item(files, i);
                            if (plist_get_node_type(val) != PLIST_STRING) {
                                continue;
                            }
                            char *str = NULL;
                            plist_get_string_val(val, &str);
                            if (!str) {
                                continue;
                            }
                            
                            
                            
                            
                            
                            
                            
                            uint32_t nlen = 0, bytes = 0;
                            mobilebackup2_error_t err;
                            err = mobilebackup2_send_raw(mb_client, (const char*)&nlen, sizeof(nlen), &bytes);
                            const char *path = "/tmp/test";
                            uint32_t pathlen = strlen(path);
                            err = mobilebackup2_send_raw(mb_client, path, pathlen, &bytes);
                            
                            
                            
                            
                            
                            
                            
                            
                            
                            free(str);
                        }
                        
                        uint32_t zero = 0, sent = 0;
                        mobilebackup2_send_raw(mb_client, (char*)&zero, sizeof(zero), &sent);
                        plist_free(msg);
                        free(dl);
                        status = StatusOK;
                    }
                }
                afc_file_close(afc, handle);
            }
        }
        mobilebackup2_client_free(mb_client);
    }
    return status;
}

string Device::name() {
    return this->mName;
}

set<string> Device::apps() {
    return this->mApps;
}

DeviceStatus Device::status() {
    return this->mStatus;
}


