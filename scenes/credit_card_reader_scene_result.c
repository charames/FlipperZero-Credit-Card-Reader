#include "../credit_card_reader_app.h"
#include "../emv/emv_country_db.h"
#include "../emv/emv_currency_db.h"
#include "../emv/emv_tag_names.h"

#include <string.h>

/* Groups PAN digits the way they're printed on the physical card: 4-6-5 for
 * 15-digit Amex numbers (which start with 34 or 37), groups of 4 otherwise. */
static void credit_card_reader_format_pan(const char* pan, FuriString* out) {
    size_t len = strlen(pan);
    bool is_amex = len == 15 && pan[0] == '3' && (pan[1] == '4' || pan[1] == '7');

    for(size_t i = 0; i < len; i++) {
        bool group_break = is_amex ? (i == 4 || i == 10) : (i > 0 && i % 4 == 0);
        if(group_break) furi_string_cat_printf(out, "  ");
        furi_string_push_back(out, pan[i]);
    }
}

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
        credit_card_reader_format_pan(data->pan, out);
        furi_string_cat_printf(out, "\n");
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

    if(data->raw_field_count > 0) {
        furi_string_cat_printf(out, "\n\e#All returned fields\n");
        for(uint8_t i = 0; i < data->raw_field_count; i++) {
            const EmvRawField* field = &data->raw_fields[i];
            const char* name = emv_tag_name(field->tag);

            if(field->tag > 0xFF) {
                furi_string_cat_printf(out, "\n%04X ", field->tag);
            } else {
                furi_string_cat_printf(out, "\n%02X ", field->tag);
            }
            furi_string_cat_printf(out, "%s\n", name ? name : "(unknown tag)");

            size_t shown = field->length < EMV_RAW_VALUE_MAX ? field->length : EMV_RAW_VALUE_MAX;
            for(size_t b = 0; b < shown; b++) {
                furi_string_cat_printf(out, "%02X ", field->value[b]);
            }
            if(field->length > shown) furi_string_cat_printf(out, "...");
            furi_string_cat_printf(out, "\n");
        }
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
