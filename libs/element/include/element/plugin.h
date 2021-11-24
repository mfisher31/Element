#ifndef EL_PLUGIN_H
#define EL_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define EL_PREFIX "el."
#define EL_EXTENSION__Main EL_PREFIX "Main"
#define EL_EXTENSION__LuaPackages EL_PREFIX "LuaPackages"

typedef void* elHandle;

typedef struct {
    elHandle (*create)();
    void (*start) (elHandle timer, int32_t ms);
} elTimerDescriptor;

typedef struct elContext elContext;

typedef struct elFeature {
    const char* ID;
    void* data;
} elFeature;

typedef struct {
    int (*main) (elHandle handle, int argc, const char* argv[]);
} elMain;

typedef elFeature elExtension;

/** NULL terminated array of elFeature pointers */
typedef const elFeature* const* elFeatures;
#define EL_FEATURES_FOREACH(features, f) \
    for (const elFeature* f = *features; f != NULL; f = *(++features))

/** Descriptor for an Element module */
typedef struct elDescriptor {
    const char* ID;
    elHandle (*create)();
    const void* (*extension) (elHandle handle, const char* name);
    void (*load) (elHandle handle, elFeatures features);
    void (*unload) (elHandle handle);
    void (*destroy) (elHandle handle);
} elDescriptor;

#ifdef __cplusplus
#define EL_PLUGIN_EXTERN extern "C"
#else
#define EL_PLUGIN_EXTERN
#endif

#ifdef _WIN32
#define EL_PLUGIN_EXPORT EL_PLUGIN_EXTERN __declspec(dllexport)
#else
// *nix exports
#define EL_PLUGIN_EXPORT \
    EL_PLUGIN_EXTERN __attribute__ ((visibility ("default")))
#endif

typedef const elDescriptor* (*elDescriptorFunction)();

EL_PLUGIN_EXPORT
const elDescriptor* element_descriptor();

#ifdef __cplusplus
}
#endif

#endif
