#ifndef ELEMENT_H_INCLUDED
#define ELEMENT_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
    #define EL_EXTERN extern "C"
#else
    #define EL_EXTERN
#endif

#ifdef _WIN32
    // windows exports
    #ifdef EL_DLLEXPORT
        #define EL_API __declspec(dllexport)
    #else
        #define EL_API __declspec(dllimport)
    #endif
    #define EL_EXPORT EL_EXTERN EL_API
#else
    // *nix exports
    #define EL_API __attribute__ ((visibility ("default")))
    #define EL_EXPORT EL_EXTERN EL_API
#endif

// export macro: includes extern C if needed followed by visibility attribute;
#ifndef EL_EXPORT
    #define EL_EXPORT
#endif

#define EL_MT_AUDIO_BUFFER_64              "el.AudioBuffer64"
#define EL_MT_AUDIO_BUFFER_32              "el.AudioBuffer32"
#define EL_MT_BYTE_ARRAY                   "el.ByteArray"
#define EL_MT_MIDI_MESSAGE                 "el.MidiMessage"
#define EL_MT_MIDI_BUFFER                  "el.MidiBuffer"
#define EL_MT_MIDI_PIPE                    "el.MidiPipe"
#define EL_MT_VECTOR                       "el.Vector"


#ifdef __cplusplus

#define EL_DISABLE_COPY(ClassName)        \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete;
#define EL_DISABLE_MOVE(ClassName)         \
    ClassName(const ClassName&&) = delete; \
    ClassName& operator=(const ClassName&&) = delete;

} // extern "C"
#endif

#endif
