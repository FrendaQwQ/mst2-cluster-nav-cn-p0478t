/*
 * mst2-cluster-nav - phone navigation on the MQB instrument cluster.
 * Copyright (c) 2026 Anton Perk. CC BY-NC-SA 4.0, see LICENSE and NOTICE.
 *
 * The CoreFoundation-like API ("CFLite") the CarPlay plug-in uses for its dictionaries,
 * resolved from libext.apple.carplay.support.so at first use.
 */
#ifndef MCN_CF_H
#define MCN_CF_H

#include "mcn_qnx.h"

typedef const void *CFTypeRef;
typedef const void *CFStringRef;
typedef const void *CFDictionaryRef;
typedef void *CFMutableDictionaryRef;
typedef const void *CFArrayRef;
typedef void *CFMutableArrayRef;
typedef int32_t OSStatus;
typedef unsigned char Boolean;
typedef long CFIndex;

#define SUPPORT_LIB "libext.apple.carplay.support.so"
#define AIRPLAY_LIB "libext.apple.carplay.airplay.so"

int cf_ready(void);
void *cf_sym(const char *lib, const char *name);

CFStringRef cf_str(const char *s);
void cf_release(CFTypeRef o);
CFMutableDictionaryRef cf_dict_new(void);
CFMutableDictionaryRef cf_dict_clone(CFDictionaryRef src);
CFTypeRef cf_dict_get(CFDictionaryRef d, const char *key);
int cf_dict_get_i64(CFDictionaryRef d, const char *key, int64_t *out);
void cf_dict_set(CFMutableDictionaryRef d, const char *key, CFTypeRef value);
void cf_dict_set_i64(CFMutableDictionaryRef d, const char *key, int64_t value);
void cf_dict_set_str(CFMutableDictionaryRef d, const char *key, const char *value);
void cf_dict_set_bool(CFMutableDictionaryRef d, const char *key, int value);
void cf_dict_remove(CFMutableDictionaryRef d, const char *key);
CFMutableArrayRef cf_array_new(void);
CFMutableArrayRef cf_array_copy(CFArrayRef src);
CFIndex cf_array_count(CFArrayRef a);
CFTypeRef cf_array_get(CFArrayRef a, CFIndex i);
void cf_array_append(CFMutableArrayRef a, CFTypeRef value);
void cf_array_append_str(CFMutableArrayRef a, const char *value);

#endif
