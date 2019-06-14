#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include "../shared/inline-hook/inlineHook.h"
#include "../shared/utils/utils.h"

const char *fromPath = "/data/app/com.beatgames.beatsaber-?/base.apk/assets/bin/Data/";
const char *toPath = "/sdcard/Android/data/com.beatgames.beatsaber/files/assets/";


_Bool startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return (lenstr < lenpre) ? 0 : strncmp(pre, str, lenpre) == 0;
}
_Bool pathMatch(char* path, char* pattern)
{
    if (strlen(path) < strlen(pattern))
        return 0;
    int qmarkLoc = strcspn(pattern, "?");
    int firstHalfComp = strncmp(path, pattern, qmarkLoc);
    if (firstHalfComp == 0)
    {
        if (strncmp(path+qmarkLoc+1, pattern+qmarkLoc+1, strlen(pattern) - (qmarkLoc+1)) == 0)
            return 1;
    }
    return 0;
}

_Bool shouldRemapFile(char* path)
{
    if (pathMatch(path, fromPath))
    {
        return 1;
    }
    return 0;
}

char* getRemapFile(char* path)
{
    if (!shouldRemapFile(path))
    {
        return 0;
    }

    int fromLen = strlen(fromPath);
    int pathLen = strlen(path);
    int toLen = strlen(toPath);
    char *newPath = malloc((fromLen - toLen) + (pathLen - fromLen) + 50);
    strcpy(newPath, toPath);
    strcpy(newPath + toLen, path + fromLen);
    log("---------------------------------REMAPPING------------------------------");
    log("From: %s", path);
    log("To: %s", newPath);
    return newPath;
}

MAKE_HOOK_NAT(open_nat, open, int, char* path, int oflag, mode_t mode)
{
    _Bool triedRemap = 0;
    int fptr = -1;
    char *remapFile = getRemapFile(path);
    if (remapFile)
    {
        log("trying to open remap...");
        triedRemap = 1;
        fptr = open(remapFile, oflag, mode);
        log("remap opened, freeing");
        free(remapFile);        
        log("free now");
    }
    if (fptr <= 0)
    {
        fptr = open_nat(path, oflag, mode);
        if (fptr > 0 && triedRemap == 1)
        {
            log("----------WARNING failed to open remapped location and used original file %s", path);
        }
        if (!startsWith("/sys", path) && !startsWith("/proc", path) && !startsWith("/dev", path))
           log("Opened file at %s returned result %i errno: %i\n", path, fptr, errno);
    }
    else
    {
        log("REMAPPED original path %s", path);
    }

    return fptr;
}

MAKE_HOOK_NAT(stat_nat, stat, int, char *path, struct stat *buf)
{
    _Bool triedRemap = 0;
    int res = -1;
    char *remapFile = getRemapFile(path);
    if (remapFile)
    {
        log("trying to stat remap...");
        triedRemap = 1;
        res = stat(remapFile, buf);
                log("remap opened, freeing");
        free(remapFile);        
        log("free now");      
    }
    if (res != 0)
    {
        res = stat_nat(path, buf);
        if (res == 0 && triedRemap == 1)
        {
            log("----------WARNING failed to stat remapped location and used original file %s", path);
        }
        if (!startsWith("/sys", path) && !startsWith("/proc", path) && !startsWith("/dev", path))
           log("stat file at %s returned result %i errno: %i\n", path, res, errno);
    }
    else
    {
        log("REMAPPED original path %s", path);
    }
    return res;
}
/*
MAKE_HOOK_NAT(Fopen_nat, fopen, FILE*, char* path, int oflag, mode_t mode)
{
    
    FILE* fptr =(FILE*) Fopen_nat(path, oflag, mode);
    if (!startsWith("/sys", path) && !startsWith("/proc", path) && !startsWith("/dev", path))
        log("FFFFFFFFFFFOpened file at %s returned result %i\n", path, (int)fptr);

    return fptr;
}

MAKE_HOOK_NAT(new_game_object, 0xC76558, int)
{
    log("Got GameObject constructor\n");
    return new_game_object();
}

MAKE_HOOK_NAT(new_game_object_str, 0xC7643C, int, char* name)
{
    int retval = new_game_object_str(name);
    log("Got GameObject constructor, name: %s retval: %i\n", name, retval);
    return retval;
}

MAKE_HOOK_NAT(mmap_nat, mmap, void*, void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    log("mmap called on fd: %i\n", fd);
    return mmap_nat(start, length, prot, flags, fd, offset);
}
MAKE_HOOK_NAT(read_nat, read, ssize_t, int fd, void *buf, size_t count)
{
    ssize_t retval = read_nat(fd, buf, count);
    log("read called with (fd: %i, xx, count: %i) and returned %i\n", fd, count, retval);
    return retval;
}

MAKE_HOOK_NAT(lseek_nat, lseek, off_t, int fd, off_t offset, int whence)
{
    off_t retval = lseek_nat(fd, offset, whence);
    log("lseek called with (fd: %i, offset: %i, whence: %i) and returned %i\n",fd, offset, whence, retval);
}

MAKE_HOOK_NAT(health_warning_start, 0x13659B0, int)
{
    log("Got HealthWarningSceneSetup.Start() call!\n");
    return health_warning_start();
}



MAKE_HOOK_NAT(fstat_nat, fstat, int, int filedes, struct stat *buf)
{
    log("got fstat call for fd: %i", filedes);
    return fstat_nat(filedes, buf);
}*/

// MAKE_HOOK_NAT(levelcollectionloaded, 0x12B1118, void*)
// {
//     log("levelcollection constructor called");
//     return levelcollectionloaded();
// }



__attribute__((constructor)) void lib_main()
{
    log("inserted file remapper from %s to %s", fromPath, toPath);
    INSTALL_HOOK_NAT(open_nat);
	INSTALL_HOOK_NAT(stat_nat);
   // INSTALL_HOOK_NAT(Fopen_nat);
    //INSTALL_HOOK_NAT(new_game_object);
    //INSTALL_HOOK_NAT(new_game_object_str);
    //INSTALL_HOOK_NAT(mmap_nat);
    //INSTALL_HOOK_NAT(read_nat);
    //INSTALL_HOOK_NAT(lseek_nat);
    //INSTALL_HOOK_NAT(health_warning_start);
   // INSTALL_HOOK_NAT(fstat_nat);
}