#ifndef EL_ELEMENT_H_INCLUDED
#define EL_ELEMENT_H_INCLUDED

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

#ifndef EL_EXPORT
    #define EL_EXPORT
#endif

#define LKV_MT_AUDIO_BUFFER_64              "el.AudioBuffer64"
#define LKV_MT_AUDIO_BUFFER_32              "el.AudioBuffer32"
#define LKV_MT_BYTE_ARRAY                   "el.ByteArray"
#define LKV_MT_MIDI_MESSAGE                 "el.MidiMessage"
#define LKV_MT_MIDI_BUFFER                  "el.MidiBuffer"
#define LKV_MT_MIDI_PIPE                    "el.MidiPipe"
#define LKV_MT_VECTOR                       "el.Vector"

typedef struct ElementContext ElementContext;

uint64_t eds_time_ns();

#ifdef __cplusplus
}
#endif

#endif
