#include "../credit_card_reader_app.h"
#include "../emv/emv_country_db.h"
#include "../emv/emv_currency_db.h"

static void credit_card_reader_scene_result_build_text(CreditCardReaderApp* app, FuriString* out) {
    const EmvCardData* data = &app->card_data;

    if(app->read_result == EmvReadResultProtocolError) {
        furi_string_cat_printf(
            out,
            "No EMV application\nfound on this card.\n\nThis may not be a\ncontactless bank card,\nor it uses an\nunsupported AID.");
        return;
    }

    furi_string_cat_printf(out, "\e#Card\n");
    if(data->aid_found) {
        furi_string_cat_printf(out, "%s\n", data->aid_name ? data->aid_name : "Unknown AID");
    }

    furi_string_cat_printf(out, "\nPAN:\n");
    if(data->pan_found) {
        furi_string_cat_printf(out, "%s\n", data->pan);
    } else {
        furi_string_cat_printf(out, "-- not found --\n");
    }

    if(data->exp_found) {
        furi_string_cat_printf(out, "\nExpires: %02u/%02u\n", data->exp_month, data->exp_year);
    }

    if(data->name_found) {
        furi_string_cat_printf(out, "\nName:\n%s\n", data->name);
    }

    if(data->country_found) {
        const char* country_name = NULL;
        if(emv_country_db_lookup(data->country_code, &country_name)) {
            furi_string_cat_printf(out, "\nCountry: %s\n", country_name);
        } else {
            furi_string_cat_printf(out, "\nCountry code: %u\n", data->country_code);
        }
    }

    if(data->currency_found) {
        const char* currency_name = NULL;
        if(emv_currency_db_lookup(data->currency_code, &currency_name)) {
            furi_string_cat_printf(out, "\nCurrency: %s\n", currency_name);
        } else {
            furi_string_cat_printf(out, "\nCurrency code: %u\n", data->currency_code);
        }
    }

    if(!data->pan_found) {
        furi_string_cat_printf(
            out, "\nAn EMV app started but\nno PAN record could\nbe read from it.");
    }
}

void credit_card_reader_app_scene_result_on_enter(void* context) {
    CreditCardReaderApp* app = context;

    widget_reset(app->widget);

    FuriString* text = furi_string_alloc();
    credit_card_reader_scene_result_build_text(app, text);
    widget_add_text_scroll_element(app->widget, 0, 0, 128, 64, furi_string_get_cstr(text));
    furi_string_free(text);

    view_dispatcher_switch_to_view(app->view_dispatcher, CreditCardReaderAppViewWidget);
}

bool credit_card_reader_app_scene_result_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void credit_card_reader_app_scene_result_on_exit(void* context) {
    CreditCardReaderApp* app = context;
    widget_reset(app->widget);
}
