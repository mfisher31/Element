
#include <element/plugin.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct JackPlugin {
} JackPlugin;

static elHandle el_jack_create(const elFeature* const* params)
{
    (void)params;
    JackPlugin* jack = (JackPlugin*)malloc(sizeof(JackPlugin));
    return jack;
}

static void el_jack_destroy(elHandle handle)
{
    JackPlugin* jack = (JackPlugin*)handle;
    free(jack);
    printf("JACK destroyed\n");
}

const elDescriptor* element_descriptor()
{
    static const struct elDescriptor D = {
        .create = el_jack_create,
        .destroy = el_jack_destroy };
    return &D;
}
