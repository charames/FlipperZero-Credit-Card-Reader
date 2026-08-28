#include "scenes.h"

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const credit_card_reader_app_on_enter_handlers[])(void*) = {
#include "scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const credit_card_reader_app_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const credit_card_reader_app_on_exit_handlers[])(void* context) = {
#include "scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers credit_card_reader_app_scene_handlers = {
    .on_enter_handlers = credit_card_reader_app_on_enter_handlers,
    .on_event_handlers = credit_card_reader_app_on_event_handlers,
    .on_exit_handlers = credit_card_reader_app_on_exit_handlers,
    .scene_num = CreditCardReaderAppSceneNum,
};
