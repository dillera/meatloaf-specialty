
#include "fnFsLittleFS.h"

#include <esp_vfs.h>
#include <errno.h>

#include "esp_littlefs.h"

#include "../../include/debug.h"


#define LITTLEFS_MAXPATH 512

// Our global LITTLEFS interface
FileSystemLittleFS fnLITTLEFS;

FileSystemLittleFS::FileSystemLittleFS()
{
    // memset(_dir,0,sizeof(DIR));
}

bool FileSystemLittleFS::dir_open(const char * path, const char * pattern, uint16_t diropts)
{
    // We ignore sorting options since we don't expect user browsing on LITTLEFS
    char * fpath = _make_fullpath(path);
    _dir = opendir(fpath);
    free(fpath);
    return(_dir != nullptr);
}

fsdir_entry * FileSystemLittleFS::dir_read()
{
    if(_dir == nullptr)
        return nullptr;

    struct dirent *d;
    d = readdir(_dir);
    if(d != nullptr)
    {
        strlcpy(_direntry.filename, d->d_name, sizeof(_direntry.filename));

        _direntry.isDir = (d->d_type & DT_DIR) ? true : false;

        _direntry.size = 0;
        _direntry.modified_time = 0;

        // timestamps aren't stored when files are uploaded during firmware deployment
        char * fpath = _make_fullpath(_direntry.filename);
        struct stat s;
        if(stat(fpath, &s) == 0)
        {
            _direntry.size = s.st_size;
            _direntry.modified_time = s.st_mtime;
        }
        #ifdef DEBUG
            // Debug_printv("stat \"%s\" errno %d\n", fpath, errno);
        #endif
        return &_direntry;
    }
    return nullptr;
}

void FileSystemLittleFS::dir_close()
{
    closedir(_dir);
    _dir = nullptr;
}

uint16_t FileSystemLittleFS::dir_tell()
{
    return 0;
}

bool FileSystemLittleFS::dir_seek(uint16_t)
{
    return false;
}

FILE * FileSystemLittleFS::file_open(const char* path, const char* mode)
{
    char * fpath = _make_fullpath(path);
    FILE * result = fopen(fpath, mode);
    free(fpath);
    return result;
}

bool FileSystemLittleFS::exists(const char* path)
{
    char * fpath = _make_fullpath(path);
    struct stat st;
    int i = stat(fpath, &st);
#ifdef DEBUG
    //Debug_printv("FileSystemLittleFS::exists returned %d on \"%s\" (%s)\n", i, path, fpath);
#endif
    free(fpath);
    return (i == 0);
}

bool FileSystemLittleFS::remove(const char* path)
{
    char * fpath = _make_fullpath(path);
    int i = ::remove(fpath);
#ifdef DEBUG
    Debug_printv("FileSystemLittleFS::remove returned %d on \"%s\" (%s)\n", i, path, fpath);
#endif
    free(fpath);
    return (i == 0);
}

bool FileSystemLittleFS::rename(const char* pathFrom, const char* pathTo)
{
    char * spath = _make_fullpath(pathFrom);
    char * dpath = _make_fullpath(pathTo);
    int i = ::rename(spath, dpath);
#ifdef DEBUG
    Debug_printv("FileSystemLittleFS::rename returned %d on \"%s\" -> \"%s\" (%s -> %s)\n", i, pathFrom, pathTo, spath, dpath);
#endif
    free(spath);
    free(dpath);
    return (i == 0);
}

uint64_t FileSystemLittleFS::total_bytes()
{
    size_t total = 0, used = 0;
	esp_littlefs_info(NULL, &total, &used);
    return (uint64_t)total;
}

uint64_t FileSystemLittleFS::used_bytes()
{
    size_t total = 0, used = 0;
	esp_littlefs_info(NULL, &total, &used);
    return (uint64_t)used;
}

bool FileSystemLittleFS::start()
{
    if(_started)
        return true;

    // Set our basepath
    // strlcpy(_basepath, "/flash", sizeof(_basepath));

    esp_vfs_littlefs_conf_t conf = {
      .base_path = "",
      .partition_label = "flash",
      .format_if_mount_failed = false,
      .dont_mount = false
    };
    
    esp_err_t e = esp_vfs_littlefs_register(&conf);

    if (e != ESP_OK)
    {
        #ifdef DEBUG
        Debug_printv("Failed to mount LittleFS partition, err = %d\n", e);
        #endif
        _started = false;
    }
    else
    {
        _started = true;
    #ifdef DEBUG        
        Debug_println("LittleFS mounted.");
        /*
        size_t total = 0, used = 0;
        esp_littlefs_info(NULL, &total, &used);
        Debug_printv("  partition size: %u, used: %u, free: %u\n", total, used, total-used);
        */
    #endif
    }

    return _started;
}
