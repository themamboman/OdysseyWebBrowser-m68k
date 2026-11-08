/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebUtils.h"
#include <sys/types.h>
#if !defined(__AMIGAOS3__)
#include <dirent.h>
#endif
#include <errno.h>
#include <wtf/FileSystem.h>
#include <string.h>
#include <string>

#include "stdio.h"

using namespace std;

bool isAbsolute(const char* url)
{
    if (!url)
        return false;
    if (url[0] == '/')
        return true;
    return false;
}  

bool isDirectory(const char* url)
{
    string urlDir = url;
    if (!strncmp("file://", url, 7))
       urlDir = urlDir.substr(7);

#if !defined(__AMIGAOS3__)
    DIR* dir = opendir(urlDir.c_str());
    if (!dir) {
        if (errno == ENOTDIR || errno == ENOENT)
            return false;
        return true;
    }
    closedir(dir);
    return true;
#else
    /* Same dirent.h gap as utils.cpp::get_available_dictionaries() -- see
     * that comment / WebCore_status.md. WTF::FileSystem::fileIsDirectory()
     * is the real check over dos.library for this target. This drops the
     * original's odd "opendir failed for some other reason => assume
     * true" fallback, which fileIsDirectory() has no equivalent for; a
     * path that genuinely is not a directory now correctly returns false
     * instead of true in that edge case. */
    return FileSystem::fileIsDirectory(WTF::String::fromUTF8(urlDir.c_str()), FileSystem::ShouldFollowSymbolicLinks::Yes);
#endif
}

