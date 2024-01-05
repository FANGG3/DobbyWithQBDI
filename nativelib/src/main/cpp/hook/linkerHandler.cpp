//
// Created by fang on 2024/1/3.
//

#include <cstdio>
#include <dlfcn.h>
#include <linux/elf.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iosfwd>
#include <sys/system_properties.h>
#include <string>
#include "linkerHandler.h"
#include "logger.h"
#include "version.h"
#include "vm.h"
#include "HookInfo.h"
#include <jni.h>


#if defined(__arm__)
typedef Elf32_Word elf_word;
typedef Elf32_Half elf_half;
typedef Elf32_Ehdr elf_ehdr;
typedef Elf32_Shdr elf_shdr;
typedef Elf32_Sym elf_sym;
#elif defined(__i386__)
typedef Elf32_Word elf_word;
typedef Elf32_Half elf_half;
typedef Elf32_Ehdr elf_ehdr;
typedef Elf32_Shdr elf_shdr;
typedef Elf32_Sym elf_sym;
#elif defined(__aarch64__)
typedef Elf64_Word elf_word;
typedef Elf64_Half elf_half;
typedef Elf64_Ehdr elf_ehdr;
typedef Elf64_Shdr elf_shdr;
typedef Elf64_Sym elf_sym;
#else
#error "Unsupported architecture"
#endif
using namespace std;
static std::ofstream *hookStrHandlerOs;
static bool isSave = false;


VM_BEFORE(sig)
//LOGD("VM_BEFORE %lx", ctx->general.x[0]);
//LOGD("VM_BEFORE %lx", ctx->general.x[1]);
    VM_AFTER
//LOGD("VM_AFTER %lx", ctx->general.x[0]);
//VM_END(sig)
}

int aaa = 0;

void onSoLoadedAfter(const char *filename,void *ret){
    auto mapInfo = getSoBaseAddress(filename);
    char buffer[PATH_MAX];
    sprintf(buffer, "linker load %s  start-> 0x%zx  end-> 0x%zx  size -> %lu",
            filename, mapInfo.start, mapInfo.end, (mapInfo.end - mapInfo.start));
    LOGI("%s ", buffer);

//    if (isSave) {
//        if (hookStrHandlerOs != nullptr) {
//            (*hookStrHandlerOs) << buffer;
//        }
//    }
    string f = string(filename);
    if ( f.find("nep") != -1 && aaa == 0){
        HookInfo::getInstance().set_module(filename,mapInfo.start,mapInfo.end);
        recordToFile("name:%s\nbase:%zx\nend:%zx",filename,mapInfo.start,mapInfo.end);

        aaa = 1;
        auto base =  get_addr(filename);
        //int offset = 0x20068C;//get
        int offset = 0x200b0c;
//        int offset = 0x1DB0C4;
        auto symbol = (void*)(base+offset);

        DobbyInstrumentQBDI(symbol,vm_handle_sig);
    }
}
HOOK_DEF(void *, dlopen_CI, const char *filename, int flag) {
    char temp[PATH_MAX];
    void *ret = orig_dlopen_CI(filename, flag);
    onSoLoadedAfter(filename, ret);
    return ret;
}

HOOK_DEF(void*, do_dlopen_CIV, const char *filename, int flag, const void *extinfo) {
    char temp[PATH_MAX];
    void *ret = orig_do_dlopen_CIV(filename, flag, extinfo);
    onSoLoadedAfter(filename, ret);
    return ret;
}

HOOK_DEF(void*, do_dlopen_CIVV, const char *name, int flags, const void *extinfo,
         void *caller_addr) {
    char temp[PATH_MAX];
    void *ret = orig_do_dlopen_CIVV(name, flags, extinfo, caller_addr);
    onSoLoadedAfter(name,ret);
    return ret;
}


static int resolve_symbol_from_symtab(FILE *fp, elf_shdr *symtab, char *strtab,
                                      size_t strtab_size, const char *symname, intptr_t *symval) {
    elf_word i, num_syms;
    elf_sym sym;
    long cur_off;

    int r = -1;

    cur_off = ftell(fp);

    if (fseek(fp, symtab->sh_offset, SEEK_SET) != 0) {
        perror("resolve_symbol_from_symtab: fseek");
        goto _ret;
    }

    num_syms = symtab->sh_size / sizeof(elf_sym);

    for (i = 0; i < num_syms; i++) {
        if (fread(&sym, sizeof(elf_sym), 1, fp) != 1) {
            perror("resolve_symbol_from_symtab: fread");
            goto _ret;
        }
        if (sym.st_name < strtab_size &&
            strcmp(&strtab[sym.st_name], symname) == 0) {
            *symval = sym.st_value;
            break;
        }
    }

    if (fseek(fp, cur_off, SEEK_SET) != 0) {
        perror("resolve_symbol_from_symtab: fseek");
        goto _ret;
    }

    if (i < num_syms)
        r = 0;

    _ret:
    return r;
}

static ssize_t read_strtab(FILE *fp, elf_shdr *shdr, char **datap) {
    elf_word sh_size;
    long cur_off;
    char *data;


    sh_size = shdr->sh_size;

    if ((size_t) sh_size > SIZE_MAX - 1) {
        fprintf(stderr, "read_strtab: %s", strerror(EFBIG));
        goto _ret;
    }


    cur_off = ftell(fp);

    if (fseek(fp, shdr->sh_offset, SEEK_SET) != 0) {
        perror("read_strtab: fseek");
        goto _ret;
    }

    if ((data = (char *) malloc(sh_size + 1)) == NULL) {
        perror("read_strtab: malloc");
        goto _ret;
    }

    if (fread(data, 1, sh_size, fp) != sh_size) {
        perror("read_strtab: fread");
        goto _free;
    }

    data[sh_size] = 0;

    if (fseek(fp, cur_off, SEEK_SET) != 0) {
        perror("read_strtab: fseek");
        goto _free;
    }

    *datap = data;

    return (ssize_t) sh_size;

    _free:
    free(data);

    _ret:
    return -1;
}


static int resolve_symbol_from_sections(FILE *fp, elf_shdr *shdrs,
                                        elf_half num_sects, const char *symname, intptr_t *symval) {
    elf_half i;
    elf_shdr *shdr, *strtab_shdr;
    char *strtab;
    ssize_t strtab_size;

    int r = -1;

    for (i = 0; i < num_sects; i++) {
        shdr = &shdrs[i];

        if (shdr->sh_type == SHT_SYMTAB && shdr->sh_link < num_sects) {
            strtab_shdr = &shdrs[shdr->sh_link];

            if ((strtab_size = read_strtab(fp, strtab_shdr, &strtab)) < 0)
                goto _ret;

            r = resolve_symbol_from_symtab(fp, shdr, strtab, (size_t) strtab_size,
                                           symname, symval);

            free(strtab);

            if (r == 0)
                goto _ret;
        }

    }

    _ret:
    return r;
}


int resolve_symbol(const char *filename, const char *symname, intptr_t *symval) {
    FILE *fp;
    elf_ehdr ehdr;
    elf_shdr *shdrs;
    elf_half shnum;

    int r = -1;

    if ((fp = fopen(filename, "r")) == NULL) {
        perror("resolve_symbol: fopen");
        goto _ret;
    }

    if (fread(&ehdr, sizeof(ehdr), 1, fp) != 1) {
        perror("resolve_symbol: fread");
        goto _close;
    }

    if (fseek(fp, ehdr.e_shoff, SEEK_SET) != 0) {
        perror("resolve_symbol: fseek");
        goto _close;
    }

    shnum = ehdr.e_shnum;

    if ((shdrs = (elf_shdr *) (calloc(shnum, sizeof(elf_shdr)))) == NULL) {
        perror("resolve_symbol: calloc");
        goto _close;
    }

    if (fread(shdrs, sizeof(elf_shdr), shnum, fp) != shnum) {
        perror("resolve_symbol: fread");
        goto _free;
    }

    r = resolve_symbol_from_sections(fp, shdrs, shnum, symname, symval);

    _free:
    free(shdrs);

    _close:
    fclose(fp);

    _ret:
    return r;
}
intptr_t get_addr(const char *name) {
    char buf[BUFSIZ], *tok[6];
    int i;
    FILE *fp;
    intptr_t r = NULL;


    fp = fopen("/proc/self/maps", "r");
    if(fp == NULL) {
        perror("get_linker_addr: fopen");
        goto ret;
    }


    while (fgets(buf, sizeof(buf), fp)) {
        i = strlen(buf);
        if (i > 0 && buf[i - 1] == '\n')
            buf[i - 1] = 0;

        tok[0] = strtok(buf, " ");
        for (i = 1; i < 6; i++)
            tok[i] = strtok(NULL, " ");

        if (tok[5] && strstr(tok[5], name) != NULL) {
            LOGE("get linker maps item -> %s ",buf);

            r = (intptr_t) strtoul(tok[0], NULL, 16);
            goto close;
        }
    }

    close:
    fclose(fp);

    ret:
    return r;
}


bool relocate_linker(const char *linker_path){
    intptr_t linker_addr, dlopen_off, symbol;
    if ((linker_addr = get_addr(linker_path)) == 0) {
        LOGE("cannot found linker addr  %s", linker_path);
        return false;
    }
    if (resolve_symbol(linker_path, "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv",
                       &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIVV,
                          (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__Z9do_dlopenPKciPK17android_dlextinfoPv",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIVV,
                          (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__ZL10dlopen_extPKciPK17android_dlextinfoPv",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIVV,
                          (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (
            resolve_symbol(linker_path, "__dl__Z20__android_dlopen_extPKciPK17android_dlextinfoPKv",
                           &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIVV,
                          (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (
            resolve_symbol(linker_path, "__dl___loader_android_dlopen_ext",
                           &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIVV,
                          (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__Z9do_dlopenPKciPK17android_dlextinfo",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIV,
                          (void **) &orig_do_dlopen_CIV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__Z8__dlopenPKciPKv",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIV,
                          (void **) &orig_do_dlopen_CIV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl___loader_dlopen",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_do_dlopen_CIV,
                          (void **) &orig_do_dlopen_CIV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl_dlopen",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        DobbyHook((void *) symbol, (void *) new_dlopen_CI,
                          (void **) &orig_dlopen_CI);
        return true;
    }
    return false;
}

HOOK_DEF(void,call_array,const char* array_name,
         void* functions,
         size_t count,
         bool reverse,
         const char* realpath){

    LOGD("call init function: %s %s ",array_name,realpath);
    orig_call_array(array_name,functions,count,reverse,realpath);

}

bool relocate_call_array(const char* linker_path){
    //_dl__ZL10call_arrayIPFviPPcS1_EEvPKcPT_mbS5_
    intptr_t linker_addr, dlopen_off, symbol;
    if ((linker_addr = get_addr(linker_path)) == 0) {
        LOGE("cannot found linker addr  %s", linker_path);
        return false;
    }

    if (resolve_symbol(linker_path, "__dl__ZL10call_arrayIPFviPPcS1_EEvPKcPT_mbS5_",
                       &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        LOGI("_dl__ZL10call_arrayIPFviPPcS1_EEvPKcPT_mbS5_ %p",symbol);
        DobbyHook((void *) symbol, (void *) new_call_array,
                  (void **) &orig_call_array);
        return true;
    }
    return false;
}

//art12_GLOBAL__N_111ScopedCheck5CheckERNS_18ScopedObjectAccessEbPKcPNS0_12JniValueTypeE.__uniq.99033978352804627313491551960229047428

HOOK_DEF(bool ,artcheck){
    return true;
}

bool relocate_artcheck(const char* libart_path){
    //_dl__ZL10call_arrayIPFviPPcS1_EEvPKcPT_mbS5_
    intptr_t linker_addr, dlopen_off, symbol;
    if ((linker_addr = get_addr(libart_path)) == 0) {
        LOGE("cannot found libart.so addr  %s", libart_path);
        return false;
    }

    if (resolve_symbol(libart_path, "art12_GLOBAL__N_111ScopedCheck5CheckERNS_18ScopedObjectAccessEbPKcPNS0_12JniValueTypeE",
                       &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        LOGI("_dl__ZL10call_arrayIPFviPPcS1_EEvPKcPT_mbS5_ %p",symbol);
        DobbyHook((void *) symbol, (void *) new_artcheck,
                  (void **) &orig_artcheck);
        return true;
    }else{
        //238894
        DobbyHook((void *) (linker_addr+0x238894), (void *) new_artcheck,
                  (void **) &orig_artcheck);
        return true;
    };
    return false;
}


static int SDK_INT = -1;

int get_sdk_level() {
    if (SDK_INT > 0) {
        return SDK_INT;
    }
    char sdk[PROP_VALUE_MAX] = {0};
    __system_property_get("ro.build.version.sdk", sdk);
    SDK_INT = atoi(sdk);
    return SDK_INT;
}

char *getLinkerPath() {
    char *linker;

    //get_sdk_level 是dlfc自己实现的方法
    //android_get_device_api_level是系统方法,低版本的NDK没有此方法。
#if defined(__aarch64__)
    if (get_sdk_level() >= ANDROID_R) {
        linker = (char*)"/apex/com.android.runtime/bin/linker64";
    } else if (get_sdk_level() >= ANDROID_Q) {
        linker = (char*)"/apex/com.android.runtime/bin/linker64";
    } else {
        linker = (char*)"/system/bin/linker64";
    }
#else
    if (get_sdk_level() >= ANDROID_R) {
        linker = (char*)"/apex/com.android.runtime/bin/linker";
    } else if (get_sdk_level() >= ANDROID_Q) {
        linker = (char*)"/apex/com.android.runtime/bin/linker";
    } else {
        linker = (char*)"/system/bin/linker";
    }
#endif

    return linker;
}
void linkerHandler_init() {

    bool isSuccess = relocate_linker(getLinkerPath());

    LOGE(">>>>>>>>> linker handler init  !  %s",(isSuccess?"true":"false"));

    isSuccess = relocate_call_array(getLinkerPath());
    LOGE(">>>>>>>>> linker handler relocate_call_array  !  %s",(isSuccess?"true":"false"));

    isSuccess = relocate_artcheck("/apex/com.android.art/lib64/libart.so");
    LOGE(">>>>>>>>> art handler relocate_artcheck  !  %s",(isSuccess?"true":"false"));
}


