
#pragma once

#include <element/juce.hpp>
#include <juce_events/juce_events.h>
#include <element/context.hpp>

namespace Element {

class AppController;
class Globals;
class Startup;

class JuceBackend {
public:
    JuceBackend() = delete;
    explicit JuceBackend (juce::ActionListener* listener = nullptr);
    ~JuceBackend();
    void initialize();
    Globals& globals();
    AppController* finish_and_return_app();
    bool has_launched() const { return launched; }

private:
    friend class Context;
    friend class Application;
    std::unique_ptr<Globals> world;
    std::unique_ptr<Startup> startup;
    juce::ActionListener* app;
    bool launched = false;
};

}
