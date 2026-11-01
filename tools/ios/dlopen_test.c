#include <dlfcn.h>
#include <stdio.h>
int main(void) {
    const char *libs[] = {
        "/private/var/herolte/lib/libdrm.2.dylib",
        "/private/var/herolte/lib/libOSMesa.8.dylib",
        "/private/var/herolte/lib/libheroeagl.dylib",
        "/private/var/herolte/lib/libherosb.dylib",
    };
    for (int i = 0; i < 4; i++) {
        printf("dlopen %s ...\n", libs[i]);
        fflush(stdout);
        void *h = dlopen(libs[i], RTLD_NOW);
        printf("dlopen result: %s\n", h ? "OK" : dlerror());
        fflush(stdout);
    }
    return 0;
}
