
#include <element/juce.hpp>
#include <element/plugin.h>

extern "C" {
/** JUCE audio plugin host support.
 * 
 *  Provides an interface for dealing with juce objects directly.
 */
typedef struct elJuceRegistrar {
    /** Opaque handle to host data. */
    void* handle;
    bool (*register_audio_plugin_format)(void* handle, void* format);
} elJuceRegistrar;
}

#define EL_FEATURE__JuceRegistrar EL_PREFIX "JuceRegistrar"

namespace element {

struct JuceRegistrar {
    JuceRegistrar() = default;
    ~JuceRegistrar() = default;
    constexpr static size_t data_size() noexcept
    {
        return sizeof(elJuceRegistrar);
    }
    JuceRegistrar(const elFeature& e) { memcpy(&ext, e.data, data_size()); }
    JuceRegistrar(const elJuceRegistrar& j) { memcpy(&ext, &j, data_size()); }

    inline bool register_format(juce::AudioPluginFormat* format)
    {
        return valid() ? ext.register_audio_plugin_format(ext.handle, (void*)format)
                       : false;
    }

    constexpr bool valid() const noexcept
    {
        return ext.handle != nullptr && ext.register_audio_plugin_format != nullptr;
    }
    inline operator bool() const noexcept { return valid(); }

private:
    elJuceRegistrar ext;
};

template<class APF, typename... Args>
static bool register_juce_audio_plugin_format(elFeatures features,
                                              Args&&... args)
{
    if (NULL == features)
        return false;

    for (auto* e = *features; e != nullptr; e++) {
        if (strcmp(e->ID, EL_FEATURE__JuceRegistrar) == 0) {
            std::unique_ptr<APF> ptr;
            JuceRegistrar je (*e);
            if (sizeof...(args) > 0)
                ptr.reset(new APF(std::forward<Args>(args)...));
            else
                ptr.reset(new APF());

            if (auto* fmt = dynamic_cast<juce::AudioPluginFormat*>(ptr.get()))
                if (je.register_format(fmt)) {
                    ptr.release();
                    return true;
                }

            break;
        }
    }

    return false;
}

}
