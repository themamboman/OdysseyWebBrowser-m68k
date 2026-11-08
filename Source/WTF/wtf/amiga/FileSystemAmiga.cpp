/*
 * FileSystemAmiga.cpp
 * AmigaOS 3.x / m68k filesystem implementation for WebKit WTF layer.
 * Uses dos.library APIs instead of POSIX.
 */

#include "config.h"
#include <wtf/FileSystem.h>

#if OS(AMIGA)

#include <wtf/FileMetadata.h>
#include <wtf/Optional.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/StringHash.h>
#include <wtf/HashMap.h>
#include <wtf/EnumTraits.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <limits.h>

namespace WTF {

namespace FileSystemImpl {

// Helper: Convert WTF String to AmigaOS C string
static CString amigaPath(const String& path)
{
    if (path.isNull())
        return CString();
    return path.utf8();
}

// Helper: Check if AmigaOS path is valid
static bool isValidAmigaPath(const CString& path)
{
    return path.data() && path.data()[0] != '\0';
}

// Helper: Convert BPTR to PlatformFileHandle (int)
static PlatformFileHandle bptrToHandle(BPTR bptr)
{
    return static_cast<PlatformFileHandle>(reinterpret_cast<intptr_t>(bptr));
}

// Helper: Convert PlatformFileHandle to BPTR
static BPTR handleToBptr(PlatformFileHandle handle)
{
    return reinterpret_cast<BPTR>(static_cast<intptr_t>(handle));
}

bool fileExists(const String& path)
{
    if (path.isNull())
        return false;

    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return false;

    BPTR lock = Lock(fsRep.data(), ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return true;
    }
    return false;
}

bool deleteFile(const String& path)
{
    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return false;

    return DeleteFile(fsRep.data()) == DOSTRUE;
}

PlatformFileHandle openFile(const String& path, FileOpenMode mode)
{
    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return invalidPlatformFileHandle;

    LONG amigaMode = 0;
    switch (mode) {
    case FileOpenMode::Read:
        amigaMode = MODE_OLDFILE;
        break;
    case FileOpenMode::Write:
        amigaMode = MODE_NEWFILE;
        break;
    }

    BPTR handle = Open(fsRep.data(), amigaMode);
    if (!handle)
        return invalidPlatformFileHandle;

    return bptrToHandle(handle);
}

void closeFile(PlatformFileHandle& handle)
{
    if (isHandleValid(handle)) {
        BPTR amigaHandle = handleToBptr(handle);
        Close(amigaHandle);
        handle = invalidPlatformFileHandle;
    }
}

long long seekFile(PlatformFileHandle handle, long long offset, FileSeekOrigin origin)
{
    if (!isHandleValid(handle))
        return -1;

    BPTR amigaHandle = handleToBptr(handle);
    LONG whence = 0;
    switch (origin) {
    case FileSeekOrigin::Beginning:
        whence = OFFSET_BEGINNING;
        break;
    case FileSeekOrigin::Current:
        whence = OFFSET_CURRENT;
        break;
    case FileSeekOrigin::End:
        whence = OFFSET_END;
        break;
    }

    LONG result = Seek(amigaHandle, offset, whence);
    return static_cast<long long>(result);
}

bool truncateFile(PlatformFileHandle handle, long long offset)
{
    if (!isHandleValid(handle))
        return false;

    BPTR amigaHandle = handleToBptr(handle);
    // AmigaOS doesn't have direct ftruncate; seek to position
    return Seek(amigaHandle, offset, OFFSET_BEGINNING) != -1;
}

int writeToFile(PlatformFileHandle handle, const char* data, int length)
{
    if (!isHandleValid(handle) || !data || length <= 0)
        return -1;

    BPTR amigaHandle = handleToBptr(handle);
    LONG written = Write(amigaHandle, (APTR)data, length);
    return written > 0 ? written : -1;
}

int readFromFile(PlatformFileHandle handle, char* data, int length)
{
    if (!isHandleValid(handle) || !data || length <= 0)
        return -1;

    BPTR amigaHandle = handleToBptr(handle);
    LONG read = Read(amigaHandle, (APTR)data, length);
    return read > 0 ? read : -1;
}

#if USE(FILE_LOCK)
bool lockFile(PlatformFileHandle handle, OptionSet<FileLockMode> lockMode)
{
    if (!isHandleValid(handle))
        return false;
    UNUSED_PARAM(handle);
    UNUSED_PARAM(lockMode);
    return true;
}

bool unlockFile(PlatformFileHandle handle)
{
    if (!isHandleValid(handle))
        return false;
    UNUSED_PARAM(handle);
    return true;
}
#endif

bool deleteEmptyDirectory(const String& path)
{
    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return false;

    return DeleteFile(fsRep.data()) == DOSTRUE;
}

bool getFileSize(const String& path, long long& result)
{
    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return false;

    BPTR lock = Lock(fsRep.data(), ACCESS_READ);
    if (!lock)
        return false;

    struct FileInfoBlock* fib = static_cast<struct FileInfoBlock*>(AllocDosObject(DOS_FIB, nullptr));
    if (!fib) {
        UnLock(lock);
        return false;
    }

    BOOL success = Examine(lock, fib);
    UnLock(lock);

    if (success) {
        result = static_cast<long long>(fib->fib_Size);
        FreeDosObject(DOS_FIB, fib);
        return true;
    }

    FreeDosObject(DOS_FIB, fib);
    return false;
}

bool getFileSize(PlatformFileHandle handle, long long& result)
{
    if (!isHandleValid(handle))
        return false;

    BPTR amigaHandle = handleToBptr(handle);
    LONG currentPos = Seek(amigaHandle, 0, OFFSET_CURRENT);
    if (currentPos == -1)
        return false;

    LONG endPos = Seek(amigaHandle, 0, OFFSET_END);
    if (endPos == -1) {
        Seek(amigaHandle, currentPos, OFFSET_BEGINNING);
        return false;
    }

    result = static_cast<long long>(endPos);
    Seek(amigaHandle, currentPos, OFFSET_BEGINNING);
    return true;
}

Optional<WallTime> getFileCreationTime(const String& path)
{
    UNUSED_PARAM(path);
    return WTF::nullopt;
}

Optional<WallTime> getFileModificationTime(const String& path)
{
    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return WTF::nullopt;

    BPTR lock = Lock(fsRep.data(), ACCESS_READ);
    if (!lock)
        return WTF::nullopt;

    struct FileInfoBlock* fib = static_cast<struct FileInfoBlock*>(AllocDosObject(DOS_FIB, nullptr));
    if (!fib) {
        UnLock(lock);
        return WTF::nullopt;
    }

    BOOL success = Examine(lock, fib);
    UnLock(lock);

    if (!success) {
        FreeDosObject(DOS_FIB, fib);
        return WTF::nullopt;
    }

    struct DateStamp* ds = &fib->fib_Date;
    double seconds = ds->ds_Days * 86400.0 + ds->ds_Minute * 60.0 + ds->ds_Tick / 50.0;
    const double amigaToUnixOffset = 2922 * 86400.0;
    WallTime time = WallTime::fromRawSeconds(seconds + amigaToUnixOffset);

    FreeDosObject(DOS_FIB, fib);
    return time;
}

static FileMetadata::Type toFileMetadataType(const struct FileInfoBlock* fib)
{
    if (fib->fib_DirEntryType >= 0)
        return FileMetadata::Type::Directory;
    return FileMetadata::Type::File;
}

static Optional<FileMetadata> fileMetadataUsingLock(const String& path)
{
    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return WTF::nullopt;

    BPTR lock = Lock(fsRep.data(), ACCESS_READ);
    if (!lock)
        return WTF::nullopt;

    struct FileInfoBlock* fib = static_cast<struct FileInfoBlock*>(AllocDosObject(DOS_FIB, nullptr));
    if (!fib) {
        UnLock(lock);
        return WTF::nullopt;
    }

    BOOL success = Examine(lock, fib);
    UnLock(lock);

    if (!success) {
        FreeDosObject(DOS_FIB, fib);
        return WTF::nullopt;
    }

    String filename = pathGetFileName(path);
    bool isHidden = !filename.isEmpty() && filename[0] == '.';

    struct DateStamp* ds = &fib->fib_Date;
    double seconds = ds->ds_Days * 86400.0 + ds->ds_Minute * 60.0 + ds->ds_Tick / 50.0;
    const double amigaToUnixOffset = 2922 * 86400.0;
    WallTime time = WallTime::fromRawSeconds(seconds + amigaToUnixOffset);

    Optional<FileMetadata> metadata = FileMetadata {
        time,
        static_cast<long long>(fib->fib_Size),
        isHidden,
        toFileMetadataType(fib)
    };

    FreeDosObject(DOS_FIB, fib);
    return metadata;
}

Optional<FileMetadata> fileMetadata(const String& path)
{
    return fileMetadataUsingLock(path);
}

Optional<FileMetadata> fileMetadataFollowingSymlinks(const String& path)
{
    return fileMetadataUsingLock(path);
}

bool createSymbolicLink(const String& targetPath, const String& symbolicLinkPath)
{
    UNUSED_PARAM(targetPath);
    UNUSED_PARAM(symbolicLinkPath);
    return false;
}

String pathByAppendingComponent(const String& path, const String& component)
{
    if (path.endsWith('/') || path.endsWith(':'))
        return path + component;
    return path + "/" + component;
}

String pathByAppendingComponents(StringView path, const Vector<StringView>& components)
{
    StringBuilder builder;
    builder.append(path);
    for (auto& component : components) {
        builder.append('/');
        builder.append(component);
    }
    return builder.toString();
}

bool makeAllDirectories(const String& path)
{
    CString fullPath = amigaPath(path);
    if (!isValidAmigaPath(fullPath))
        return false;

    BPTR lock = Lock(fullPath.data(), ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return true;
    }

    /* Create each missing directory from the volume/assign downwards, e.g. for
       "PROGDIR:Conf/LocalStorage": "PROGDIR:Conf", then "PROGDIR:Conf/LocalStorage".
       The part up to and including ':' is a volume or assign, never created. */
    const char* full = fullPath.data();
    size_t length = strlen(full);
    const char* colon = strchr(full, ':');
    size_t position = colon ? static_cast<size_t>(colon - full) + 1 : 0;

    while (position < length) {
        const char* slash = strchr(full + position, '/');
        size_t end = slash ? static_cast<size_t>(slash - full) : length;
        if (end > position) {
            CString prefix(full, end);
            lock = Lock(prefix.data(), ACCESS_READ);
            if (!lock)
                lock = CreateDir(prefix.data()); /* returns an exclusive lock on success */
            if (!lock)
                return false;
            UnLock(lock);
        }
        position = end + 1;
    }

    return true;
}

String pathGetFileName(const String& path)
{
    auto position = path.reverseFind('/');
    if (position == notFound)
        position = path.reverseFind(':');
    return path.substring(position + 1);
}

String directoryName(const String& path)
{
    CString fsRep = amigaPath(path);
    if (!isValidAmigaPath(fsRep))
        return String();

    const char* start = fsRep.data();
    const char* lastSlash = nullptr;
    for (const char* p = start; *p; ++p) {
        if (*p == '/' || *p == ':')
            lastSlash = p;
    }

    if (!lastSlash)
        return String();

    return String::fromUTF8(start, lastSlash - start);
}

Vector<String> listDirectory(const String& path, const String& filter)
{
    Vector<String> entries;
    CString cpath = amigaPath(path);
    if (!isValidAmigaPath(cpath))
        return entries;

    BPTR lock = Lock(cpath.data(), ACCESS_READ);
    if (!lock)
        return entries;

    struct FileInfoBlock* fib = static_cast<struct FileInfoBlock*>(AllocDosObject(DOS_FIB, nullptr));
    if (!fib) {
        UnLock(lock);
        return entries;
    }

    if (Examine(lock, fib)) {
        while (ExNext(lock, fib)) {
            const char* name = fib->fib_FileName;
            if (!strcmp(name, ".") || !strcmp(name, ".."))
                continue;

            String pathStr = String::fromUTF8(cpath.data());
            String fullPath = pathByAppendingComponent(pathStr, String::fromUTF8(name));
            auto string = stringFromFileSystemRepresentation(fullPath.utf8().data());
            if (!string.isNull())
                entries.append(WTFMove(string));
        }
    }

    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    return entries;
}

String stringFromFileSystemRepresentation(const char* path)
{
    if (!path)
        return String();
    return String::fromUTF8(path);
}

CString fileSystemRepresentation(const String& path)
{
    if (path.contains(':') && path.startsWith('/')) {
        String sub = path.substring(1);
        return sub.utf8();
    }
    return path.utf8();
}

bool moveFile(const String& oldPath, const String& newPath)
{
    auto oldFilename = fileSystemRepresentation(oldPath);
    if (!isValidAmigaPath(oldFilename))
        return false;

    auto newFilename = fileSystemRepresentation(newPath);
    if (!isValidAmigaPath(newFilename))
        return false;

    return Rename(oldFilename.data(), newFilename.data()) == DOSTRUE;
}

bool getVolumeFreeSpace(const String& path, uint64_t& freeSpace)
{
    UNUSED_PARAM(path);
    UNUSED_PARAM(freeSpace);
    return false;
}

String openTemporaryFile(const String& tmpPath, const String& prefix, PlatformFileHandle& handle, const String& suffix)
{
    UNUSED_PARAM(suffix);

    char buffer[PATH_MAX];
    CString tmpPathStr = fileSystemRepresentation(tmpPath);
    CString prefixStr = fileSystemRepresentation(prefix);

    if (!isValidAmigaPath(tmpPathStr) || prefixStr.length() == 0) {
        handle = invalidPlatformFileHandle;
        return String();
    }

    static int tempCounter = 0;
    if (snprintf(buffer, PATH_MAX, "%s/%s%d.tmp", tmpPathStr.data(), prefixStr.data(), ++tempCounter) >= PATH_MAX) {
        handle = invalidPlatformFileHandle;
        return String();
    }

    BPTR amigaHandle = Open(buffer, MODE_NEWFILE);
    if (!amigaHandle) {
        handle = invalidPlatformFileHandle;
        return String();
    }

    handle = bptrToHandle(amigaHandle);
    return String::fromUTF8(buffer);
}

static HashMap<String, String>& tmpPathPrefixes()
{
    static NeverDestroyed<HashMap<String, String>> map;
    return map;
}

String temporaryFilePathForPrefix(const String& prefix)
{
    if (tmpPathPrefixes().contains(prefix))
        return tmpPathPrefixes().get(prefix);
    return { };
}

void setTemporaryFilePathForPrefix(const char* tmpPath, const String& prefix)
{
    tmpPathPrefixes().set(prefix, String::fromUTF8(tmpPath));
}

String openTemporaryFile(const String& prefix, PlatformFileHandle& handle, const String& suffix)
{
    const char* tmpDir = "T:";
    if (tmpPathPrefixes().contains(prefix))
        return openTemporaryFile(tmpPathPrefixes().get(prefix), prefix, handle, suffix);
    return openTemporaryFile(String::fromUTF8(tmpDir), prefix, handle, suffix);
}

bool hardLink(const String& source, const String& destination)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(destination);
    return false;
}

bool hardLinkOrCopyFile(const String& source, const String& destination)
{
    if (hardLink(source, destination))
        return true;

    if (source.isEmpty() || destination.isEmpty())
        return false;

    auto fsSource = fileSystemRepresentation(source);
    if (!isValidAmigaPath(fsSource))
        return false;

    auto fsDestination = fileSystemRepresentation(destination);
    if (!isValidAmigaPath(fsDestination))
        return false;

    BPTR srcHandle = Open(fsSource.data(), MODE_OLDFILE);
    if (!srcHandle)
        return false;

    BPTR dstHandle = Open(fsDestination.data(), MODE_NEWFILE);
    if (!dstHandle) {
        Close(srcHandle);
        return false;
    }

    char buffer[4096];
    LONG bytesRead;
    BOOL success = TRUE;
    while ((bytesRead = Read(srcHandle, buffer, sizeof(buffer))) > 0) {
        if (Write(dstHandle, buffer, bytesRead) != bytesRead) {
            success = FALSE;
            break;
        }
    }

    Close(srcHandle);
    Close(dstHandle);

    if (!success) {
        DeleteFile(fsDestination.data());
        return false;
    }

    return true;
}

Optional<int32_t> getFileDeviceId(const CString& fsFile)
{
    UNUSED_PARAM(fsFile);
    return WTF::nullopt;
}

} // namespace FileSystemImpl
} // namespace WTF

#endif // OS(AMIGA)