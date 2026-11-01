#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* struct proc_regioninfo from xnu sys/proc_info.h (the iPhoneOS SDK has no header for it) */
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

static void scan(int pid, const char *name)
{
    uint64_t addr = 0;
    int regions = 0, hits = 0, last_n = 0;
    char last[1024] = "";
    for (;;) {
        struct region_info info;
        last_n = proc_pidinfo(pid, PIDREGIONINFO, addr, &info, sizeof(info));
        if (last_n != (int)sizeof(info))
            break;
        regions++;
        char path[1024] = "";
        if (proc_regionfilename(pid, info.pri_address, path, sizeof(path)) > 0 && strcmp(path, last) != 0) {
            strlcpy(last, path, sizeof(last));
            if (strstr(path, "herolte")) {
                printf("%s[%d]: mapped %s\n", name, pid, path);
                hits++;
            }
        }
        uint64_t next = info.pri_address + info.pri_size;
        if (next <= addr)
            break;
        addr = next;
    }
    printf("%s[%d]: %d regions scanned, %d herolte mappings (last pidinfo %d, struct %d)\n",
           name, pid, regions, hits, last_n, (int)sizeof(struct region_info));
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
