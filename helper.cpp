//
//  helper.cpp
//  iOS Hijacker
//
//  Created by Monster on 4/8/15.
//  Copyright (c) 2015 Monster. All rights reserved.
//
#include <sys/stat.h>

#include "helper.h"

void log(LogLevel level, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    char buffer[0x100];
    vsnprintf(buffer, sizeof(buffer), fmt, vl);
    va_end(vl);
    cout << "[#] " << buffer << endl;
}

int createDirectory(string path) {
    int id = (int)path.find_last_of('/');
    if (id > 0) {
        createDirectory(path.substr(0, id));
    }
    return mkdir(path.c_str(), 0777);
}

int createLocalFile(string path, char *buffer, int length) {
    createDirectory(path.substr(0, path.find_last_of('/')));
    FILE *fp = NULL;
    fp = fopen(path.c_str(), "w");
    if (fp == NULL) {
        return -1;
    }
    if (fwrite(buffer, sizeof(char), length, fp) != length) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}