#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysctl.h>

/* struct proc_regioninfo from xnu sys/proc_info.h (the iPhoneOS SDK has no header) */
struct region_info {
    uint32_t pri_protection, pri_max_protection, pri_inheritance, pri_flags;
    uint64_t pri_offset;
    uint32_t pri_behavior, pri_user_wired_count, pri_user_tag, pri_pages_resident,
             pri_pages_shared_now_private, pri_pages_swapped_out, pri_pages_dirtied,
             pri_ref_count, pri_shadow_depth, pri_share_mode, pri_private_pages_resident,
             pri_shared_pages_resident, pri_obj_id, pri_depth;
    uint64_t pri_address;
    uint64_t pri_size;
};
#define PIDREGIONINFO 7

extern int proc_pidinfo(int pid, int flavor, uint64_t arg, void *buffer, int buffersize);
extern int proc_name(int pid, void *buffer, uint32_t buffersize);
extern int proc_regionfilename(int pid, uint64_t address, void *buffer, uint32_t buffersize);

static int interesting(const char *p)
{
    return strstr(p, "herolte") || strstr(p, "OSMesa") || strstr(p, "eagl")
        || strstr(p, "herosb") || strstr(p, "libdrm");
}

static void scan(int pid, const char *name)
{
    uint64_t addr = 0;
    int regions = 0, named = 0, hits = 0, printed = 0;
    char last[1024] = "";
    for (;;) {
        struct region_info info;
        if (proc_pidinfo(pid, PIDREGIONINFO, addr, &info, sizeof(info)) != (int)sizeof(info))
            break;
        regions++;
        char path[1024] = "";
        if (proc_regionfilename(pid, info.pri_address, path, sizeof(path)) > 0 && path[0]) {
            named++;
            if (strcmp(path, last)) {
                strlcpy(last, path, sizeof(last));
                if (interesting(path)) {
                    printf("%s[%d]: HIT %s\n", name, pid, path);
                    hits++;
                } else if (printed < 10) {
                    printf("%s[%d]: path %s\n", name, pid, path);
                    printed++;
                }
            }
        }
        uint64_t next = info.pri_address + info.pri_size;
        if (next <= addr)
            break;
        addr = next;
    }
    printf("%s[%d]: %d regions, %d named, %d hits\n", name, pid, regions, named, hits);
    fflush(stdout);
}

/* argv+env of another process: sysctl kern.procargs2 (root can read any pid) */
static void dump_env(int pid, const char *name)
{
    static char buf[65536];
    size_t len = sizeof(buf);
    int mib[3] = { 1 /* CTL_KERN */, 49 /* KERN_PROCARGS2 */, pid };
    if (sysctl(mib, 3, buf, &len, NULL, 0) != 0) {
        printf("%s[%d]: procargs2 failed\n", name, pid);
        fflush(stdout);
        return;
    }
    int argc = 0;
    memcpy(&argc, buf, sizeof(argc));
    char *p = buf + sizeof(argc), *end = buf + len;
    printf("%s[%d]: exec %s\n", name, pid, p);
    while (p < end && *p) p++;
    while (p < end && !*p) p++;
    for (int i = 0; i < argc && p < end; i++) {
        while (p < end && *p) p++;
        while (p < end && !*p) p++;
    }
    int n = 0, shown = 0;
    while (p < end && *p && n < 300) {
        if (strstr(p, "DYLD") || strstr(p, "herolte")) {
            printf("%s[%d]: env %s\n", name, pid, p);
            shown++;
        }
        n++;
        while (p < end && *p) p++;
        while (p < end && !*p) p++;
    }
    printf("%s[%d]: %d env vars, %d interesting\n", name, pid, n, shown);
    fflush(stdout);
}

int main(void)
{
    /* self-check: this binary IS /private/var/herolte/test, so it must HIT */
    scan(getpid(), "self");
    for (int pid = 1; pid < 2000; pid++) {
        char name[64] = "";
        if (proc_name(pid, name, sizeof(name)) <= 0)
            continue;
        if (!strcmp(name, "SpringBoard") || !strcmp(name, "backboardd")) {
            dump_env(pid, name);
            scan(pid, name);
        }
    }
    return 0;
}
