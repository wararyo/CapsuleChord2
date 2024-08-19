#include "AppMetronome.h"

void AppMetronome::onCreate()
{
    isActive = false;
}

void AppMetronome::onActivate()
{
    isActive = true;
    Tempo.addListener(&tempoCallbacks);
}

void AppMetronome::onDeactivate()
{
    isActive = false;
    Tempo.removeListener(&tempoCallbacks);
}

void AppMetronome::onShowGui(lv_obj_t *container)
{
    button = lv_btn_create(container);
    lv_obj_center(button);
    lv_obj_add_event_cb(button, [](lv_event_t *e) {
        auto *self = (AppMetronome *)lv_event_get_user_data(e);
        if (self->isActive)
        {
            Tempo.removeListener(&self->tempoCallbacks);
        }
        else
        {
            Tempo.addListener(&self->tempoCallbacks);
        }
        self->isActive = !self->isActive;
    }, LV_EVENT_CLICKED, (void *)this);
    label = lv_label_create(button);
    lv_label_set_text(label, "Helloメトロノーム");
}

void AppMetronome::onHideGui()
{
    lv_obj_del(label);
}

void AppMetronome::onDestroy()
{
}
