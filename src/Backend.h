
#pragma once

#include <element/juce.hpp>
#include <juce_events/juce_events.h>
#include <element/context.hpp>

namespace Element {
class AppController;
class Globals;
class Startup;

class JuceBackend : public element::Backend {
public:
    JuceBackend (juce::ActionListener* listener);
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
