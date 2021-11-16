#include <sol/sol.hpp>
#include <element/scripting.hpp>

namespace element {

class Scripting::Impl {
public:
    Impl (Scripting& s) : owner(s) {
        
    }

private:
    Scripting& owner;
    sol::state rootstate;
};

Scripting::Scripting()
{
    impl.reset(new Impl(*this));
}
Scripting::~Scripting() {}

}
