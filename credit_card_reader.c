#include "credit_card_reader_app.h"

static bool credit_card_reader_app_custom_event_callback(void* context, uint32_t event) {
    CreditCardReaderApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool credit_card_reader_app_back_event_callback(void* context) {
    CreditCardReaderApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static CreditCardReaderApp* credit_card_reader_app_alloc(void) {
    CreditCardReaderApp* app = malloc(sizeof(CreditCardReaderApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    app->scene_manager = scene_manager_alloc(&credit_card_reader_app_scene_handlers, app);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, credit_card_reader_app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, credit_card_reader_app_back_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->popup = popup_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, CreditCardReaderAppViewPopup, popup_get_view(app->popup));

    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, CreditCardReaderAppViewWidget, widget_get_view(app->widget));

    app->nfc = NULL;
    app->emv_reader = NULL;

    return app;
}

static void credit_card_reader_app_free(CreditCardReaderApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, CreditCardReaderAppViewPopup);
    view_dispatcher_remove_view(app->view_dispatcher, CreditCardReaderAppViewWidget);
    popup_free(app->popup);
    widget_free(app->widget);

    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t credit_card_reader_app(void* args) {
    UNUSED(args);

    CreditCardReaderApp* app = credit_card_reader_app_alloc();

    scene_manager_next_scene(app->scene_manager, CreditCardReaderAppSceneRead);

    view_dispatcher_run(app->view_dispatcher);

    credit_card_reader_app_free(app);
    return 0;
}
