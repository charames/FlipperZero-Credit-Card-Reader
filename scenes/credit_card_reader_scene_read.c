#include "../credit_card_reader_app.h"

#include <string.h>

static void credit_card_reader_scene_read_emv_done_callback(
    EmvReadResult result,
    const EmvCardData* data,
    void* context) {
    CreditCardReaderApp* app = context;
    app->read_result = result;
    memcpy(&app->card_data, data, sizeof(EmvCardData));
    view_dispatcher_send_custom_event(
        app->view_dispatcher, CreditCardReaderAppCustomEventReadDone);
}

void credit_card_reader_app_scene_read_on_enter(void* context) {
    CreditCardReaderApp* app = context;

    popup_reset(app->popup);
    popup_set_header(app->popup, "Scanning for card", 64, 10, AlignCenter, AlignTop);
    popup_set_text(
        app->popup,
        "Hold a contactless\nbank card to the back\nof your Flipper",
        64,
        30,
        AlignCenter,
        AlignTop);
    view_dispatcher_switch_to_view(app->view_dispatcher, CreditCardReaderAppViewPopup);

    app->nfc = nfc_alloc();
    app->emv_reader = emv_reader_alloc();
    emv_reader_start(
        app->emv_reader, app->nfc, credit_card_reader_scene_read_emv_done_callback, app);
}

bool credit_card_reader_app_scene_read_on_event(void* context, SceneManagerEvent event) {
    CreditCardReaderApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == CreditCardReaderAppCustomEventReadDone) {
            if(app->read_result == EmvReadResultSuccess) {
                notification_message(app->notifications, &sequence_success);
            }
            scene_manager_next_scene(app->scene_manager, CreditCardReaderAppSceneResult);
            consumed = true;
        }
    }

    return consumed;
}

void credit_card_reader_app_scene_read_on_exit(void* context) {
    CreditCardReaderApp* app = context;

    emv_reader_stop(app->emv_reader);
    emv_reader_free(app->emv_reader);
    app->emv_reader = NULL;

    nfc_free(app->nfc);
    app->nfc = NULL;

    popup_reset(app->popup);
}
