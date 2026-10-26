/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 */
#include "cf.h"
#include "mcn_hook.h"

#define CF_UTF8 0x08000100u

static CFStringRef (*p_str)(void *, const char *, unsigned);
static void (*p_release)(CFTypeRef);
static CFMutableDictionaryRef (*p_dict_new)(void *, CFIndex, const void *, const void *);
static CFIndex (*p_dict_count)(CFDictionaryRef);
static void (*p_dict_keys)(CFDictionaryRef, const void **, const void **);
static CFTypeRef (*p_dict_get)(CFDictionaryRef, CFTypeRef);
static void (*p_dict_set)(CFMutableDictionaryRef, CFTypeRef, CFTypeRef);
static int64_t (*p_dict_get_i64)(CFDictionaryRef, CFTypeRef, OSStatus *);
static void (*p_dict_set_i64)(CFMutableDictionaryRef, CFTypeRef, int64_t);
static void (*p_dict_remove)(CFMutableDictionaryRef, CFTypeRef);
static CFMutableArrayRef (*p_array_new)(void *, CFIndex, const void *);
static CFMutableArrayRef (*p_array_copy)(void *, CFIndex, CFArrayRef);
static CFIndex (*p_array_count)(CFArrayRef);
static CFTypeRef (*p_array_get)(CFArrayRef, CFIndex);
static void (*p_array_append)(CFMutableArrayRef, CFTypeRef);
static const void *p_key_callbacks;
static const void *p_value_callbacks;
static const void *p_array_callbacks;
static CFTypeRef *p_false;
static CFTypeRef *p_true;
static int ready;

void *cf_sym(const char *lib, const char *name)
{
    void *h = dlopen(lib, RTLD_LAZY | RTLD_GLOBAL);
    void *p = h ? dlsym(h, name) : NULL;

    if (!p) {
        mcn_log2("missing symbol ", name);
    }
    return p;
}

int cf_ready(void)
{
    if (ready) {
        return 1;
    }
    p_str = cf_sym(SUPPORT_LIB, "CFStringCreateWithCString");
    p_release = cf_sym(SUPPORT_LIB, "CFRelease");
    p_dict_new = cf_sym(SUPPORT_LIB, "CFDictionaryCreateMutable");
    p_dict_count = cf_sym(SUPPORT_LIB, "CFDictionaryGetCount");
    p_dict_keys = cf_sym(SUPPORT_LIB, "CFDictionaryGetKeysAndValues");
    p_dict_get = cf_sym(SUPPORT_LIB, "CFDictionaryGetValue");
    p_dict_set = cf_sym(SUPPORT_LIB, "CFDictionarySetValue");
    p_dict_get_i64 = cf_sym(SUPPORT_LIB, "CFDictionaryGetInt64");
    p_dict_set_i64 = cf_sym(SUPPORT_LIB, "CFDictionarySetInt64");
    p_dict_remove = cf_sym(SUPPORT_LIB, "CFDictionaryRemoveValue");
    p_array_new = cf_sym(SUPPORT_LIB, "CFArrayCreateMutable");
    p_array_copy = cf_sym(SUPPORT_LIB, "CFArrayCreateMutableCopy");
    p_array_count = cf_sym(SUPPORT_LIB, "CFArrayGetCount");
    p_array_get = cf_sym(SUPPORT_LIB, "CFArrayGetValueAtIndex");
    p_array_append = cf_sym(SUPPORT_LIB, "CFArrayAppendValue");
    p_key_callbacks = cf_sym(SUPPORT_LIB, "kCFLDictionaryKeyCallBacksCFLTypes");
    p_value_callbacks = cf_sym(SUPPORT_LIB, "kCFLDictionaryValueCallBacksCFLTypes");
    p_array_callbacks = cf_sym(SUPPORT_LIB, "kCFLArrayCallBacksCFLTypes");
    p_false = cf_sym(SUPPORT_LIB, "kCFLBooleanFalse");
    p_true = cf_sym(SUPPORT_LIB, "kCFLBooleanTrue");
    ready = p_str && p_release && p_dict_new && p_dict_count && p_dict_keys && p_dict_get &&
            p_dict_set && p_dict_get_i64 && p_dict_set_i64 && p_dict_remove && p_array_new &&
            p_array_copy && p_array_count && p_array_get && p_array_append && p_key_callbacks &&
            p_value_callbacks && p_array_callbacks && p_false && p_true;
    return ready;
}

CFStringRef cf_str(const char *s)
{
    return p_str(NULL, s, CF_UTF8);
}

void cf_release(CFTypeRef o)
{
    if (o) {
        p_release(o);
    }
}

CFMutableDictionaryRef cf_dict_new(void)
{
    return p_dict_new(NULL, 0, p_key_callbacks, p_value_callbacks);
}

CFMutableDictionaryRef cf_dict_clone(CFDictionaryRef src)
{
    CFMutableDictionaryRef d = cf_dict_new();
    CFIndex i, n = src ? p_dict_count(src) : 0;
    const void **keys, **values;

    if (!d || n <= 0) {
        return d;
    }
    keys = malloc(sizeof(void *) * (size_t)n * 2);
    if (!keys) {
        cf_release(d);
        return NULL;
    }
    values = keys + n;
    p_dict_keys(src, keys, values);
    for (i = 0; i < n; i++) {
        p_dict_set(d, keys[i], values[i]);
    }
    free(keys);
    return d;
}

CFTypeRef cf_dict_get(CFDictionaryRef d, const char *key)
{
    CFStringRef k = cf_str(key);
    CFTypeRef v = d && k ? p_dict_get(d, k) : NULL;

    cf_release(k);
    return v;
}

int cf_dict_get_i64(CFDictionaryRef d, const char *key, int64_t *out)
{
    OSStatus err = -1;
    CFStringRef k = cf_str(key);

    if (d && k) {
        *out = p_dict_get_i64(d, k, &err);
    }
    cf_release(k);
    return err == 0;
}

void cf_dict_set(CFMutableDictionaryRef d, const char *key, CFTypeRef value)
{
    CFStringRef k = cf_str(key);

    if (d && k && value) {
        p_dict_set(d, k, value);
    }
    cf_release(k);
}

void cf_dict_set_i64(CFMutableDictionaryRef d, const char *key, int64_t value)
{
    CFStringRef k = cf_str(key);

    if (d && k) {
        p_dict_set_i64(d, k, value);
    }
    cf_release(k);
}

void cf_dict_set_str(CFMutableDictionaryRef d, const char *key, const char *value)
{
    CFStringRef v = cf_str(value);

    cf_dict_set(d, key, v);
    cf_release(v);
}

void cf_dict_set_bool(CFMutableDictionaryRef d, const char *key, int value)
{
    cf_dict_set(d, key, value ? *p_true : *p_false);
}

void cf_dict_remove(CFMutableDictionaryRef d, const char *key)
{
    CFStringRef k = cf_str(key);

    if (d && k) {
        p_dict_remove(d, k);
    }
    cf_release(k);
}

CFMutableArrayRef cf_array_new(void)
{
    return p_array_new(NULL, 0, p_array_callbacks);
}

CFMutableArrayRef cf_array_copy(CFArrayRef src)
{
    return src ? p_array_copy(NULL, 0, src) : cf_array_new();
}

CFIndex cf_array_count(CFArrayRef a)
{
    return a ? p_array_count(a) : 0;
}

CFTypeRef cf_array_get(CFArrayRef a, CFIndex i)
{
    return p_array_get(a, i);
}

void cf_array_append(CFMutableArrayRef a, CFTypeRef value)
{
    if (a && value) {
        p_array_append(a, value);
    }
}

void cf_array_append_str(CFMutableArrayRef a, const char *value)
{
    CFStringRef v = cf_str(value);

    cf_array_append(a, v);
    cf_release(v);
}
