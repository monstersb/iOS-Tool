//
//  helper.h
//  iOS Hijacker
//
//  Created by Monster on 4/8/15.
//  Copyright (c) 2015 Monster. All rights reserved.
//

#ifndef __iOS_Hijacker__helper__
#define __iOS_Hijacker__helper__

#include <stdio.h>

#endif /* defined(__iOS_Hijacker__helper__) */


#include <iostream>
#include <string>

#ifdef HELPER_H
#else

using namespace std;

enum LogLevel {
    Information,
    Warning,
    Error,
};

void log(LogLevel level, const char *fmt, ...);

int createDirectory(string path);
int createLocalFile(string path, char *buffer, int length);

#endif

#define HELPER_H