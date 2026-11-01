#include <stdio.h>
#include <string.h>
#include <dirent.h>

static int interesting(const char *s)
{
    static const char *keys[] = { "terminating", "could not", "not found", "tried:",
        "error", "denied", "herolte", "OSMesa", "libdrm", "eagl", "herosb",
        "security policy", "LC_RPATH", "insert", NULL };
    for (int i = 0; keys[i]; i++)
        if (strstr(s, keys[i]))
            return 1;
    return 0;
}

static void dump(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("dyldlog: %s: absent\n", path);
        return;
    }
    char line[2048], tail[30][256];
    int total = 0, shown = 0, nt = 0;
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        total++;
        if (interesting(line) && shown < 200) {
            printf("dyld| %s\n", line);
            shown++;
        }
        snprintf(tail[nt % 30], sizeof tail[0], "%s", line);
        nt++;
    }
    fclose(f);
    printf("dyldlog: %s: %d lines, %d interesting\n", path, total, shown);
    for (int i = (nt > 30 ? nt - 30 : 0); i < nt; i++)
        printf("tail| %s\n", tail[i % 30]);
}

int main(void)
{
    dump("/private/var/tmp/herodyld-backboardd.log");
    dump("/private/var/tmp/herodyld-springboard.log");
    DIR *d = opendir("/private/var/tmp");
    struct dirent *e;
    while (d && (e = readdir(d)))
        if (e->d_name[0] != '.')
            printf("tmp| %s\n", e->d_name);
    if (d)
        closedir(d);
    fflush(stdout);
    return 0;
}
