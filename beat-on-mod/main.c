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
#include <regex.h>

#define ASSETS_REGEX "\\/data\\/app\\/com\\.beatgames\\.beatsaber-[0-9]\\/base\\.apk\\/assets\\/bin\\/Data\\/(.*)"
const char* ASSETS_TARGET = "/sdcard/BeatOnData/BeatSaberAssets/";

#define SONGS_REGEX "\\/data\\/app\\/com\\.beatgames\\.beatsaber-[0-9]\\/base\\.apk\\/assets\\/bin\\/Data\\/CustomSongs\\/(.*)"
const char* SONGS_TARGET = "/sdcard/BeatOnData/CustomSongs/";

#define STARTSWITH(pre, str) ((strlen(str) < strlen(pre)) ? 0 : strncmp(pre, str, strlen(pre)) == 0)

regex_t assetsRegex;
regex_t songsRegex;
uint globalOpid = 0;

MAKE_HOOK_NAT(open_nat, open, int, char* path, int oflag, mode_t mode)
{
    int opid = globalOpid++;
    int filePtr = -1;
    char bfr[500];
    regmatch_t pmatch[2];
    if (regexec(&songsRegex, path, 2, pmatch, 0) == 0) {
        log("%i ** OPEN: SONG remap path match **", opid);
        log("%i From: %s\n", opid, path);        
        memset(bfr, 0, sizeof bfr);
        strcpy(bfr, SONGS_TARGET);
        strcpy(bfr+strlen(SONGS_TARGET), path + pmatch[1].rm_so);
        log("%i To: %s\n", opid, bfr);
        filePtr = open_nat(bfr, oflag, mode);
        if (filePtr < 0) {
            log("%i ** OPEN: No SONG remap, remapped file did not open **", opid);
            log("%i errno: %i %s", opid, errno, strerror(errno));
            return open_nat(path, oflag, mode);
        } else
        {
            log("%i ************** OPEN: SONG REMAP SUCCESS **************", opid);
            return filePtr;
        }
    } else if (regexec(&assetsRegex, path, 2, pmatch, 0) == 0) {
        log("%i ** OPEN: ASSET remap path match **", opid);
        log("%i From: %s\n", opid, path);        
        memset(bfr, 0, sizeof bfr);
        strcpy(bfr, ASSETS_TARGET);
        strcpy(bfr+strlen(ASSETS_TARGET), path + pmatch[1].rm_so);
        log("%i To: %s\n", opid, bfr);
        filePtr = open_nat(bfr, oflag, mode);
        if (filePtr < 0) {
            log("%i ** OPEN: No ASSET remap, remapped file did not open **", opid);
            log("%i errno: %i %s", opid, errno, strerror(errno));
            return open_nat(path, oflag, mode);
        } else
        {
            log("%i ************** OPEN: ASSET REMAP SUCCESS **************", opid);
            return filePtr;
        }
    } else {
        if (!STARTSWITH("/sys", path) && !STARTSWITH("/proc", path) && !STARTSWITH("/dev", path))
            log("%i Normal open: %s", opid, path);

        return open_nat(path, oflag, mode);
    }
}

MAKE_HOOK_NAT(stat_nat, stat, int, char *path, struct stat *buf)
{
    int opid = globalOpid++;
    int statResult = -1;
    char bfr[500];
    regmatch_t pmatch[2];
    if (regexec(&songsRegex, path, 2, pmatch, 0) == 0) {
        log("%i ^^ STAT: SONG remap path match ^^", opid);
        log("%i From: %s\n", opid, path);        
        memset(bfr, 0, sizeof bfr);
        strcpy(bfr, SONGS_TARGET);
        strcpy(bfr+strlen(SONGS_TARGET), path + pmatch[1].rm_so);
        log("%i To: %s\n", opid, bfr);
        statResult = stat_nat(bfr, buf);
        if (statResult < 0) {
            log("%i ^^ STAT: No SONG remap, remapped file did not stat ^^", opid);
            log("%i errno: %i %s", opid, errno, strerror(errno));
            return stat_nat(path, buf);
        } else
        {
            log("%i ^^^^^^^^^^^^^^ STAT: SONG REMAP SUCCESS ^^^^^^^^^^^^^^", opid);
            return statResult;
        }
    } else if (regexec(&assetsRegex, path, 2, pmatch, 0) == 0) {
        log("%i ^^ STAT: ASSET remap path match ^^", opid);
        log("%i From: %s\n", opid, path);        
        memset(bfr, 0, sizeof bfr);
        strcpy(bfr, ASSETS_TARGET);
        strcpy(bfr+strlen(ASSETS_TARGET), path + pmatch[1].rm_so);
        log("%i To: %s\n", opid, bfr);
        statResult = stat_nat(bfr, buf);
        if (statResult < 0) {
            log("%i ^^ STAT: No ASSET remap, remapped file did not stat ^^", opid);
            log("%i errno: %i %s", opid, errno, strerror(errno));
            return stat_nat(path, buf);
        } else
        {
            log("%i ^^^^^^^^^^^^^^ STAT: ASSET REMAP SUCCESS ^^^^^^^^^^^^^^", opid);
            return statResult;
        }
    } else {
        if (!STARTSWITH("/sys", path) && !STARTSWITH("/proc", path) && !STARTSWITH("/dev", path))
            log("%i Normal stat: %s", opid, path);

        return stat_nat(path, buf);
    }
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
    log("Init of Beat On file remapper");
    if (regcomp(&assetsRegex, ASSETS_REGEX,REG_ICASE|REG_EXTENDED) != 0) {
        log("Failed to compile regex for assets!");
    }
    if (regcomp(&songsRegex, SONGS_REGEX,REG_ICASE|REG_EXTENDED) != 0) {
        log("Failed to compile regex for songs!");
    }
    
    INSTALL_HOOK_NAT(open_nat);
    log("open hook installed!");
	INSTALL_HOOK_NAT(stat_nat);
    log("stat hook installed!");
   // INSTALL_HOOK_NAT(Fopen_nat);
    //INSTALL_HOOK_NAT(new_game_object);
    //INSTALL_HOOK_NAT(new_game_object_str);
    //INSTALL_HOOK_NAT(mmap_nat);
    //INSTALL_HOOK_NAT(read_nat);
    //INSTALL_HOOK_NAT(lseek_nat);
    //INSTALL_HOOK_NAT(health_warning_start);
   // INSTALL_HOOK_NAT(fstat_nat);
}