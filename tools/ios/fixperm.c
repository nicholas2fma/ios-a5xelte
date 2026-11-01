#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

extern int proc_pidinfo(int pid, int flavor, uint64_t arg, void *buffer, int buffersize);
extern int proc_name(int pid, void *buffer, uint32_t buffersize);

/* struct proc_bsdinfo from xnu sys/proc_info.h (not in the iPhoneOS SDK) */
struct bsdinfo {
    uint32_t pbi_flags, pbi_status, pbi_xstatus, pbi_pid, pbi_ppid;
    uint32_t pbi_uid, pbi_gid, pbi_ruid, pbi_rgid, pbi_svuid, pbi_svgid, rfu_1;
    char pbi_comm[16], pbi_name[32];
    uint32_t pbi_nfiles, pbi_pgid, pbi_pjobc, e_tdev, e_tpgid;
    int32_t pbi_nice;
    uint64_t pbi_start_tvsec, pbi_start_tvusec;
};
#define PIDTBSDINFO 3

#define DROP "/private/var/herolte"
#define LIB  DROP "/lib"

static const char *dirs[] = { "/private", "/private/var", "/private/var/tmp", DROP, LIB, NULL };
static const char *files[] = {
    LIB "/libheroeagl.dylib", LIB "/libOSMesa.8.dylib", LIB "/libdrm.2.dylib",
    LIB "/libherosb.dylib", DROP "/wallpaper.png", NULL
};

static void show(const char *p)
{
    struct stat st;
    if (stat(p, &st) != 0) {
        printf("perm: %s: stat errno=%d\n", p, errno);
        return;
    }
    printf("perm: %s mode=%04o uid=%d gid=%d size=%lld\n", p, (unsigned)(st.st_mode & 07777),
           (int)st.st_uid, (int)st.st_gid, (long long)st.st_size);
}

static void probe(int uid)
{
    if (seteuid(uid) != 0) {
        printf("perm: seteuid(%d) errno=%d\n", uid, errno);
        return;
    }
    for (int i = 0; dirs[i]; i++) {
        int fd = open(dirs[i], O_RDONLY);
        printf("perm: uid %d open dir %s -> %s errno=%d\n", uid, dirs[i],
               fd >= 0 ? "OK" : "FAIL", fd >= 0 ? 0 : errno);
        if (fd >= 0)
            close(fd);
    }
    for (int i = 0; files[i]; i++) {
        int fd = open(files[i], O_RDONLY);
        printf("perm: uid %d open %s -> %s errno=%d\n", uid, files[i],
               fd >= 0 ? "OK" : "FAIL", fd >= 0 ? 0 : errno);
        if (fd >= 0)
            close(fd);
    }
    if (seteuid(0) != 0)
        printf("perm: seteuid(0) back errno=%d\n", errno);
}

int main(void)
{
    printf("perm: running as uid %d euid %d\n", (int)getuid(), (int)geteuid());
    int target = 501;
    for (int pid = 1; pid < 2000; pid++) {
        char name[64] = "";
        if (proc_name(pid, name, sizeof(name)) <= 0)
            continue;
        if (strcmp(name, "backboardd") && strcmp(name, "SpringBoard"))
            continue;
        struct bsdinfo bi;
        if (proc_pidinfo(pid, PIDTBSDINFO, 0, &bi, sizeof(bi)) == (int)sizeof(bi)) {
            printf("perm: %s[%d] uid=%d gid=%d ruid=%d\n", name, pid,
                   (int)bi.pbi_uid, (int)bi.pbi_gid, (int)bi.pbi_ruid);
            if (!strcmp(name, "backboardd") && bi.pbi_uid != 0)
                target = (int)bi.pbi_uid;
        } else {
            printf("perm: %s[%d]: no bsdinfo (struct %d)\n", name, pid, (int)sizeof(bi));
        }
    }

    printf("perm: --- before ---\n");
    for (int i = 0; dirs[i]; i++)
        show(dirs[i]);
    for (int i = 0; files[i]; i++)
        show(files[i]);
    probe(target);

    printf("perm: --- fixing ---\n");
    for (int i = 0; dirs[i]; i++)
        if (strncmp(dirs[i], DROP, strlen(DROP)) == 0)
            printf("perm: chmod 0755 %s -> %d (errno=%d)\n", dirs[i],
                   chmod(dirs[i], 0755), errno);
    for (int i = 0; files[i]; i++)
        printf("perm: chmod 0644 %s -> %d (errno=%d)\n", files[i],
               chmod(files[i], 0644), errno);

    printf("perm: --- after ---\n");
    for (int i = 0; dirs[i]; i++)
        show(dirs[i]);
    for (int i = 0; files[i]; i++)
        show(files[i]);
    probe(target);
    fflush(stdout);
    return 0;
}
