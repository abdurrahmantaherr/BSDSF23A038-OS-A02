// src/ls-v1.0.0.c
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>

struct fileinfo {
    char *name;
    struct stat st;
};

static void mode_to_perm(mode_t m, char *buf) {
    // file type
    if (S_ISDIR(m)) buf[0] = 'd';
    else if (S_ISLNK(m)) buf[0] = 'l';
    else if (S_ISCHR(m)) buf[0] = 'c';
    else if (S_ISBLK(m)) buf[0] = 'b';
    else if (S_ISFIFO(m)) buf[0] = 'p';
    else if (S_ISSOCK(m)) buf[0] = 's';
    else buf[0] = '-';

    // user
    buf[1] = (m & S_IRUSR) ? 'r' : '-';
    buf[2] = (m & S_IWUSR) ? 'w' : '-';
    buf[3] = (m & S_IXUSR) ? 'x' : '-';

    // group
    buf[4] = (m & S_IRGRP) ? 'r' : '-';
    buf[5] = (m & S_IWGRP) ? 'w' : '-';
    buf[6] = (m & S_IXGRP) ? 'x' : '-';

    // others
    buf[7] = (m & S_IROTH) ? 'r' : '-';
    buf[8] = (m & S_IWOTH) ? 'w' : '-';
    buf[9] = (m & S_IXOTH) ? 'x' : '-';

    // special bits: setuid, setgid, sticky
    if (m & S_ISUID) buf[3] = (buf[3] == 'x') ? 's' : 'S';
    if (m & S_ISGID) buf[6] = (buf[6] == 'x') ? 's' : 'S';
    if (m & S_ISVTX) buf[9] = (buf[9] == 'x') ? 't' : 'T';

    buf[10] = '\0';
}

static void format_time(time_t mtime, char *buf, size_t buflen) {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&mtime, &tm);

    // if older than ~6 months (approx 15552000 seconds) or in future, show year
    if (llabs((long long)(now - mtime)) > 15552000LL) {
        // "%b %e  %Y" (two spaces before year to visually line up like ls)
        strftime(buf, buflen, "%b %e  %Y", &tm);
    } else {
        strftime(buf, buflen, "%b %e %H:%M", &tm);
    }
}

static int num_digits_longlong(long long x) {
    if (x == 0) return 1;
    int d = 0;
    if (x < 0) { x = -x; d++; }
    while (x) { x /= 10; d++; }
    return d;
}

// long listing implementation for one directory path
static void long_listing(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno));
        return;
    }

    struct dirent *de;
    struct fileinfo *arr = NULL;
    size_t cap = 0, n = 0;
    char full[PATH_MAX];

    while ((de = readdir(d)) != NULL) {
        // skip hidden files (same as default ls), unless you want -a behavior
        if (de->d_name[0] == '.') continue;

        if (n + 1 > cap) {
            cap = cap ? cap * 2 : 64;
            arr = realloc(arr, cap * sizeof *arr);
            if (!arr) { perror("realloc"); closedir(d); return; }
        }

        arr[n].name = strdup(de->d_name);
        if (!arr[n].name) { perror("strdup"); closedir(d); return; }

        snprintf(full, sizeof full, "%s/%s", path, de->d_name);
        if (lstat(full, &arr[n].st) == -1) {
            // if lstat fails, still store zeroed stat to avoid crash
            perror(full);
            memset(&arr[n].st, 0, sizeof arr[n].st);
        }
        n++;
    }
    closedir(d);

    // compute column widths
    int max_links = 0, max_owner = 0, max_group = 0, max_size = 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned long links = (unsigned long)arr[i].st.st_nlink;
        int d_links = snprintf(NULL, 0, "%lu", links);
        if (d_links > max_links) max_links = d_links;

        struct passwd *pw = getpwuid(arr[i].st.st_uid);
        int len_owner = pw ? (int)strlen(pw->pw_name) : snprintf(NULL, 0, "%u", (unsigned)arr[i].st.st_uid);
        if (len_owner > max_owner) max_owner = len_owner;

        struct group *gr = getgrgid(arr[i].st.st_gid);
        int len_group = gr ? (int)strlen(gr->gr_name) : snprintf(NULL, 0, "%u", (unsigned)arr[i].st.st_gid);
        if (len_group > max_group) max_group = len_group;

        int d_size = num_digits_longlong((long long)arr[i].st.st_size);
        if (d_size > max_size) max_size = d_size;
    }

    // print entries
    char perm[11], timestr[64], ownerbuf[32], groupbuf[32];
    for (size_t i = 0; i < n; ++i) {
        mode_to_perm(arr[i].st.st_mode, perm);

        unsigned long links = (unsigned long)arr[i].st.st_nlink;
        struct passwd *pw = getpwuid(arr[i].st.st_uid);
        const char *owner = NULL;
        if (pw) owner = pw->pw_name;
        else { snprintf(ownerbuf, sizeof ownerbuf, "%u", (unsigned)arr[i].st.st_uid); owner = ownerbuf; }

        struct group *gr = getgrgid(arr[i].st.st_gid);
        const char *group = NULL;
        if (gr) group = gr->gr_name;
        else { snprintf(groupbuf, sizeof groupbuf, "%u", (unsigned)arr[i].st.st_gid); group = groupbuf; }

        format_time(arr[i].st.st_mtime, timestr, sizeof timestr);

        printf("%s %*lu %-*s %-*s %*lld %s %s\n",
               perm,
               max_links, links,
               max_owner, owner,
               max_group, group,
               max_size, (long long)arr[i].st.st_size,
               timestr,
               arr[i].name);
    }

    // cleanup
    for (size_t i = 0; i < n; ++i) free(arr[i].name);
    free(arr);
}

// simple (default) list: print filenames separated by newline
static void simple_list(const char *path) {
    DIR *d = opendir(path);
    if (!d) { fprintf(stderr, "opendir(%s): %s\n", path, strerror(errno)); return; }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        printf("%s\n", de->d_name);
    }
    closedir(d);
}

int main(int argc, char **argv) {
    int opt;
    int long_listing_flag = 0;

    while ((opt = getopt(argc, argv, "l")) != -1) {
        if (opt == 'l') long_listing_flag = 1;
    }

    const char *path = ".";
    if (optind < argc) path = argv[optind];

    if (long_listing_flag) long_listing(path);
    else simple_list(path);

    return 0;
}

