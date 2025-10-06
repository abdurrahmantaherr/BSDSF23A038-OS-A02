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
#include <sys/ioctl.h>
#include <termios.h>

struct fileinfo {
    char *name;
    struct stat st;
};

// ------------------- PERMISSION STRING --------------------
static void mode_to_perm(mode_t m, char *buf) {
    if (S_ISDIR(m)) buf[0] = 'd';
    else if (S_ISLNK(m)) buf[0] = 'l';
    else if (S_ISCHR(m)) buf[0] = 'c';
    else if (S_ISBLK(m)) buf[0] = 'b';
    else if (S_ISFIFO(m)) buf[0] = 'p';
    else if (S_ISSOCK(m)) buf[0] = 's';
    else buf[0] = '-';

    buf[1] = (m & S_IRUSR) ? 'r' : '-';
    buf[2] = (m & S_IWUSR) ? 'w' : '-';
    buf[3] = (m & S_IXUSR) ? 'x' : '-';
    buf[4] = (m & S_IRGRP) ? 'r' : '-';
    buf[5] = (m & S_IWGRP) ? 'w' : '-';
    buf[6] = (m & S_IXGRP) ? 'x' : '-';
    buf[7] = (m & S_IROTH) ? 'r' : '-';
    buf[8] = (m & S_IWOTH) ? 'w' : '-';
    buf[9] = (m & S_IXOTH) ? 'x' : '-';

    if (m & S_ISUID) buf[3] = (buf[3] == 'x') ? 's' : 'S';
    if (m & S_ISGID) buf[6] = (buf[6] == 'x') ? 's' : 'S';
    if (m & S_ISVTX) buf[9] = (buf[9] == 'x') ? 't' : 'T';

    buf[10] = '\0';
}

// ------------------- TIME FORMAT --------------------
static void format_time(time_t mtime, char *buf, size_t buflen) {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&mtime, &tm);

    if (llabs((long long)(now - mtime)) > 15552000LL)
        strftime(buf, buflen, "%b %e  %Y", &tm);
    else
        strftime(buf, buflen, "%b %e %H:%M", &tm);
}

// ------------------- LONG LISTING MODE --------------------
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
        if (de->d_name[0] == '.') continue;

        if (n + 1 > cap) {
            cap = cap ? cap * 2 : 64;
            arr = realloc(arr, cap * sizeof *arr);
            if (!arr) { perror("realloc"); closedir(d); return; }
        }

        arr[n].name = strdup(de->d_name);
        snprintf(full, sizeof full, "%s/%s", path, de->d_name);
        if (lstat(full, &arr[n].st) == -1)
            memset(&arr[n].st, 0, sizeof arr[n].st);
        n++;
    }
    closedir(d);

    int max_links = 0, max_owner = 0, max_group = 0, max_size = 0;
    for (size_t i = 0; i < n; ++i) {
        char tmp[64];
        int len;

        len = snprintf(tmp, sizeof tmp, "%lu", (unsigned long)arr[i].st.st_nlink);
        if (len > max_links) max_links = len;

        struct passwd *pw = getpwuid(arr[i].st.st_uid);
        len = pw ? strlen(pw->pw_name) : snprintf(tmp, sizeof tmp, "%u", arr[i].st.st_uid);
        if (len > max_owner) max_owner = len;

        struct group *gr = getgrgid(arr[i].st.st_gid);
        len = gr ? strlen(gr->gr_name) : snprintf(tmp, sizeof tmp, "%u", arr[i].st.st_gid);
        if (len > max_group) max_group = len;

        len = snprintf(tmp, sizeof tmp, "%lld", (long long)arr[i].st.st_size);
        if (len > max_size) max_size = len;
    }

    char perm[11], timestr[64], ownerbuf[32], groupbuf[32];
    for (size_t i = 0; i < n; ++i) {
        mode_to_perm(arr[i].st.st_mode, perm);

        struct passwd *pw = getpwuid(arr[i].st.st_uid);
        const char *owner = pw ? pw->pw_name : (snprintf(ownerbuf, sizeof ownerbuf, "%u", arr[i].st.st_uid), ownerbuf);

        struct group *gr = getgrgid(arr[i].st.st_gid);
        const char *group = gr ? gr->gr_name : (snprintf(groupbuf, sizeof groupbuf, "%u", arr[i].st.st_gid), groupbuf);

        format_time(arr[i].st.st_mtime, timestr, sizeof timestr);

        printf("%s %*lu %-*s %-*s %*lld %s %s\n",
               perm,
               max_links, (unsigned long)arr[i].st.st_nlink,
               max_owner, owner,
               max_group, group,
               max_size, (long long)arr[i].st.st_size,
               timestr,
               arr[i].name);
    }

    for (size_t i = 0; i < n; ++i)
        free(arr[i].name);
    free(arr);
}

// ------------------- COLUMN DISPLAY MODE (DOWN THEN ACROSS) --------------------
static void simple_list(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        perror("opendir");
        return;
    }

    struct dirent *de;
    char **names = NULL;
    int count = 0, capacity = 0;
    size_t maxlen = 0;

    // Step 1: Read all filenames and find longest name
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 64;
            names = realloc(names, capacity * sizeof(char *));
        }
        names[count] = strdup(de->d_name);
        size_t len = strlen(de->d_name);
        if (len > maxlen) maxlen = len;
        count++;
    }
    closedir(d);
    if (count == 0) return;

    // Step 2: Get terminal width
    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
        term_width = ws.ws_col;

    int spacing = 2;
    int col_width = (int)maxlen + spacing;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    // Step 3: Print down then across
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx < count)
                printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }

    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

// ------------------- MAIN --------------------
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

