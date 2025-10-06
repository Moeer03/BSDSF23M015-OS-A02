/*
* Programming Assignment 02: lsv1.1.0
* Added Feature: Long Listing (-l)
* Usage:
*       $ ./lsv1.1.0
*       $ ./lsv1.1.0 -l
*       $ ./lsv1.1.0 /home
*       $ ./lsv1.1.0 -l /home /etc
*/

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <linux/limits.h>   // for PATH_MAX
#include <sys/ioctl.h>
#include <termios.h>


#ifndef PATH_MAX
#define PATH_MAX 4096
#endif




void do_ls(const char *dir, int longflag);
void mode_to_string(mode_t mode, char *str);
void print_long(const char *path, const char *name);

int main(int argc, char *argv[])
{
    int opt;
    int longflag = 0;

    // parse options (-l for now, more can be added later)
    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                longflag = 1;
                break;
            default:
                break;
        }
    }

    // no directory args, default to "."
    if (optind == argc) {
        do_ls(".", longflag);
    } else {
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], longflag);
            puts("");
        }
    }

    return 0;
}

// Convert mode_t to string like -rwxr-xr-x
void mode_to_string(mode_t mode, char *str) {
    str[0] = S_ISDIR(mode) ? 'd' :
             S_ISLNK(mode) ? 'l' :
             S_ISCHR(mode) ? 'c' :
             S_ISBLK(mode) ? 'b' :
             S_ISFIFO(mode)? 'p' :
             '-' ;   // skip sockets if S_ISSOCK not available
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}


// Print one file entry in long listing format
void print_long(const char *path, const char *name) {
    struct stat st;
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", path, name);

    if (lstat(fullpath, &st) == -1) {
        perror("lstat");
        return;
    }

    char perms[11];
    mode_to_string(st.st_mode, perms);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);

    char timebuf[64];
    struct tm *tm = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);

    printf("%s %2lu %s %s %6lld %s %s\n",
           perms,
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "???",
           gr ? gr->gr_name : "???",
           (long long)st.st_size,
           timebuf,
           name);
}

// Directory listing logic
void do_ls(const char *dir, int longflag) {
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    if (longflag) {
        // Long listing stays same
        while ((entry = readdir(dp)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;
            print_long(dir, entry->d_name);
        }
        closedir(dp);
        return;
    }

    // --- Feature 3: Non-long listing with columns ---

    // Step 1: Read all filenames into dynamic array
    size_t count = 0, capacity = 50;
    char **filenames = malloc(capacity * sizeof(char *));
    if (!filenames) {
        perror("malloc");
        closedir(dp);
        return;
    }

    int max_len = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        if (count >= capacity) {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
            if (!filenames) {
                perror("realloc");
                closedir(dp);
                return;
            }
        }

        filenames[count] = strdup(entry->d_name);
        if (!filenames[count]) {
            perror("strdup");
            closedir(dp);
            return;
        }

        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;

        count++;
    }
    closedir(dp);

    if (count == 0) {
        free(filenames);
        return;
    }

    // Step 2: Get terminal width
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;  // fallback if 0

    // Step 3: Compute number of columns and rows
    int num_cols = term_width / (max_len + 2);
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    // Step 4: Print down then across
    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            int idx = c * num_rows + r;
            if (idx < (int)count)
                printf("%-*s", max_len + 2, filenames[idx]);
        }
        printf("\n");
    }

    // Step 5: Free memory
    for (size_t i = 0; i < count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}
