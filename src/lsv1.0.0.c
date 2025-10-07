/*
 * Programming Assignment 02: lsv1.7.0
 * Added Feature: Recursive Listing (-R)
 * Previous Features:
 *  - Alphabetical Sorting (qsort)
 *  - Horizontal Display (-x)
 *  - Colored Output
 * 
 * Usage:
 *      $ ./lsv1.7.0
 *      $ ./lsv1.7.0 -l
 *      $ ./lsv1.7.0 -x
 *      $ ./lsv1.7.0 -R
 *      $ ./lsv1.7.0 -Rl /path
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
#include <linux/limits.h>
#include <sys/ioctl.h>
#include <termios.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// 🎨 Color definitions for output
#define COLOR_RESET   "\033[0m"
#define COLOR_DIR     "\033[0;34m"  // Blue for directories
#define COLOR_EXE     "\033[0;32m"  // Green for executables
#define COLOR_TAR     "\033[0;31m"  // Red for tar/archives
#define COLOR_LINK    "\033[0;35m"  // Magenta for symlinks

// Global flags
int recursive_flag = 0; // -R flag

// Function declarations
void do_ls(const char *dir, int mode);
void mode_to_string(mode_t mode, char *str);
void print_long(const char *path, const char *name);
void print_horizontal(const char *dir, char **files, int count, int max_len, int term_width);
void print_down_then_across(const char *dir, char **files, int count, int max_len, int term_width);
void print_colored_name(const char *dir, const char *filename);

// Compare function for qsort
static int compare_filenames(const void *a, const void *b) {
    const char *const *pa = (const char *const *)a;
    const char *const *pb = (const char *const *)b;
    return strcmp(*pa, *pb);
}

int main(int argc, char *argv[]) {
    int opt;
    int mode = 0; // 0 = default, 1 = long (-l), 2 = horizontal (-x)

    // Parse options: l, x, R
    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l': mode = 1; break;
            case 'x': mode = 2; break;
            case 'R': recursive_flag = 1; break;
            default: break;
        }
    }

    // Default: current directory
    if (optind == argc) {
        do_ls(".", mode);
    } else {
        for (int i = optind; i < argc; i++) {
            printf("%s:\n", argv[i]);
            do_ls(argv[i], mode);
            printf("\n");
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
             '-';
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

// Determine file color and print accordingly
void print_colored_name(const char *dir, const char *filename) {
    struct stat st;
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s/%s", dir, filename);

    if (lstat(fullpath, &st) == -1) {
        perror("lstat");
        printf("%s", filename);
        return;
    }

    if (S_ISDIR(st.st_mode))
        printf(COLOR_DIR "%s" COLOR_RESET, filename);
    else if (S_ISLNK(st.st_mode))
        printf(COLOR_LINK "%s" COLOR_RESET, filename);
    else if (st.st_mode & S_IXUSR)
        printf(COLOR_EXE "%s" COLOR_RESET, filename);
    else if (strstr(filename, ".tar") || strstr(filename, ".zip"))
        printf(COLOR_TAR "%s" COLOR_RESET, filename);
    else
        printf("%s", filename);
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

    printf("%s %2lu %s %s %6lld %s ",
           perms,
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "???",
           gr ? gr->gr_name : "???",
           (long long)st.st_size,
           timebuf);
    print_colored_name(path, name);
    printf("\n");
}

// Print down-then-across (default)
void print_down_then_across(const char *dir, char **files, int count, int max_len, int term_width) {
    int num_cols = term_width / (max_len + 2);
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            int idx = c * num_rows + r;
            if (idx < count) {
                print_colored_name(dir, files[idx]);
                int pad = max_len + 2 - (int)strlen(files[idx]);
                for (int k = 0; k < pad; k++) printf(" ");
            }
        }
        printf("\n");
    }
}

// Print across (horizontal) for -x mode
void print_horizontal(const char *dir, char **files, int count, int max_len, int term_width) {
    int col_width = max_len + 2;
    int pos = 0;
    for (int i = 0; i < count; i++) {
        if (pos + col_width > term_width) {
            printf("\n");
            pos = 0;
        }
        print_colored_name(dir, files[i]);
        int pad = col_width - (int)strlen(files[i]);
        for (int k = 0; k < pad; k++) printf(" ");
        pos += col_width;
    }
    printf("\n");
}

// Directory listing logic (supports recursion)
void do_ls(const char *dir, int mode) {
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    if (recursive_flag)
        printf("\n%s:\n", dir);

    // For -l mode
    if (mode == 1) {
        while ((entry = readdir(dp)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;
            print_long(dir, entry->d_name);
        }
        closedir(dp);
        return;
    }

    // Default / -x mode
    size_t count = 0, capacity = 64;
    char **files = malloc(capacity * sizeof(char *));
    if (!files) {
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
            char **tmp = realloc(files, capacity * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                break;
            }
            files = tmp;
        }
        files[count] = strdup(entry->d_name);
        if (!files[count]) break;
        int len = (int)strlen(entry->d_name);
        if (len > max_len) max_len = len;
        count++;
    }
    closedir(dp);

    if (count > 1)
        qsort(files, count, sizeof(char *), compare_filenames);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;

    if (mode == 2)
        print_horizontal(dir, files, count, max_len, term_width);
    else
        print_down_then_across(dir, files, count, max_len, term_width);

    // 🔁 Recursively process subdirectories
    if (recursive_flag) {
        for (size_t i = 0; i < count; i++) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", dir, files[i]);

            struct stat st;
            if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (strcmp(files[i], ".") != 0 && strcmp(files[i], "..") != 0) {
                    do_ls(path, mode);
                }
            }
        }
    }

    for (size_t i = 0; i < count; i++)
        free(files[i]);
    free(files);
}
