#include <element/juce/plugin.hpp>
#include <element/context.hpp>

#define JLV2_PLUGINHOST_LV2 1
#include "../../libs/jlv2/modules/jlv2_host/jlv2_host.cpp"

namespace element {

template<typename T>
class NullTerminated final {
public:
    explicit NullTerminated (T v)
        : vec (v)
    {
        for (auto e = *v; e != NULL; e++)
            ++_size;
    }
    ~NullTerminated() = default;

    constexpr size_t size() const noexcept { return _size; }
    auto begin() const noexcept { return &vec [0]; }
    auto end()   const noexcept { return &vec [_size - 1]; }

private:
    T vec {nullptr};
    size_t _size {0};
};

inline static bool contains (elFeatures features, const char* key)
{
    EL_FEATURES_FOREACH (features, f) {
        if (strcmp (f->ID, key) == 0)
            return true;
    }
    return false;
}

}

struct JLV2 final {
    JLV2(elFeatures features)
    {
        std::clog << "JLV2 created..." << std::endl;
        element::register_juce_audio_plugin_format<jlv2::LV2PluginFormat>(
                features);
    }

    ~JLV2() {
        std::clog << "JLV2 destoyed..." << std::endl;
    }
};

static elHandle jlv2_create(const elFeature* const* fa)
{
    if (auto data = element::contains (fa, EL_FEATURE__JuceRegistrar))
        return new JLV2 (fa);
    return nullptr;
}

static void jlv2_destroy(elHandle handle)
{
    delete static_cast<JLV2*>(handle);
}

const elDescriptor* eds_descriptor()
{
    const static elDescriptor M = {.ID = "org.lvtk.element.JLV2",
                                   .create = jlv2_create,
                                   .destroy = jlv2_destroy};
    return &M;
}
