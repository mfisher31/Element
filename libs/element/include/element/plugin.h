#ifndef EL_PLUGIN_H
#define EL_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define EL_PREFIX "el."

typedef void* elHandle;

typedef struct elFeature {
    const char* ID;
    void* data;
} elFeature;

/** NULL terminated array of elFeature pointers */
typedef const elFeature* const* elFeatures;
#define EL_FEATURES_FOREACH(features, f) \
    for (const elFeature* f = *features; f != NULL; f++)

/** Descriptor for an Element plugin */
typedef struct elDescriptor {
    const char* ID;
    elHandle (*create)(elFeatures features);
    void (*destroy)(elHandle plugin);
    const void* (*feature)(const char* ID);
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
    #define EL_PLUGIN_EXPORT                                                 \
        EL_PLUGIN_EXTERN __attribute__((visibility("default")))
#endif

typedef const elDescriptor* (*ELDescriptorFunction)();

EL_PLUGIN_EXPORT
const elDescriptor* eds_descriptor();

#ifdef __cplusplus
}
#endif

#endif
