#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/proc_info.h>

extern int proc_pidinfo(int pid, int flavor, uint64_t arg, void *buffer, int buffersize);
extern int proc_name(int pid, void *buffer, uint32_t buffersize);

static void scan(int pid, const char *name)
{
    uint64_t addr = 0;
    int regions = 0, hits = 0;
    char last[1024] = "";
    for (;;) {
        struct proc_regionwithpathinfo info;
        int n = proc_pidinfo(pid, PROC_PIDREGIONPATHINFO, addr, &info, sizeof(info));
        if (n <= 0)
            break;
        regions++;
        const char *path = info.prp_vip.vip_path;
        if (path[0] && strcmp(path, last) != 0) {
            strlcpy(last, path, sizeof(last));
            if (strstr(path, "herolte")) {
                printf("%s[%d]: mapped %s\n", name, pid, path);
                hits++;
            }
        }
        uint64_t next = info.prp_prinfo.pri_address + info.prp_prinfo.pri_size;
        if (next <= addr)
            break;
        addr = next;
    }
    printf("%s[%d]: %d regions scanned, %d herolte mappings\n", name, pid, regions, hits);
    fflush(stdout);
}

int main(void)
{
    for (int pid = 1; pid < 2000; pid++) {
        char name[64] = "";
        if (proc_name(pid, name, sizeof(name)) <= 0)
            continue;
        if (!strcmp(name, "SpringBoard") || !strcmp(name, "backboardd"))
            scan(pid, name);
    }
    return 0;
}
