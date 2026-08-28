#include "emv_reader.h"
#include "emv_apdu.h"
#include "emv_tlv.h"
#include "emv_aid_db.h"

#include <nfc/protocols/iso14443_4a/iso14443_4a.h>
#include <nfc/protocols/iso14443_4a/iso14443_4a_poller.h>
#include <nfc/nfc_poller.h>

#include <furi.h>
#include <string.h>

#define EMV_MAX_CANDIDATE_AIDS (12)
#define EMV_MAX_RECORD_READS (40)

typedef struct {
    uint8_t aid[EMV_AID_MAX_LEN];
    uint8_t len;
} EmvAidCandidate;

/* Well-known contactless AIDs to fall back to when a card either has no PPSE
 * directory or PPSE selection fails outright (older/non-standard cards).
 * Values verified against applications/main/nfc/resources/nfc/assets/aid.nfc
 * in the official flipperzero-firmware repo. */
static const EmvAidCandidate kFallbackAids[] = {
    {{0xA0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10}, 7}, /* Visa Debit/Credit (Classic) */
    {{0xA0, 0x00, 0x00, 0x00, 0x04, 0x10, 0x10}, 7}, /* MasterCard Global */
    {{0xA0, 0x00, 0x00, 0x00, 0x25, 0x01}, 6}, /* American Express */
    {{0xA0, 0x00, 0x00, 0x01, 0x52, 0x40, 0x10}, 7}, /* Discover */
    {{0xA0, 0x00, 0x00, 0x00, 0x65, 0x10}, 6}, /* JCB */
    {{0xA0, 0x00, 0x00, 0x03, 0x33, 0x01, 0x01, 0x02}, 8}, /* UnionPay Credit */
    {{0xA0, 0x00, 0x00, 0x02, 0x77, 0x10, 0x10}, 7}, /* INTERAC */
};

struct EmvReader {
    Nfc* nfc;
    NfcPoller* poller;
    BitBuffer* tx;
    BitBuffer* rx;
    EmvReaderDoneCallback callback;
    void* callback_context;
};

static uint8_t emv_bcd_byte_to_dec(uint8_t b) {
    return (uint8_t)((b >> 4) * 10 + (b & 0x0F));
}

static uint16_t emv_bcd2_to_number(const uint8_t* v) {
    return (uint16_t)(
        (v[0] >> 4) * 1000 + (v[0] & 0x0F) * 100 + (v[1] >> 4) * 10 + (v[1] & 0x0F));
}

static void emv_bcd_pan_decode(const uint8_t* value, size_t len, char* out, size_t out_cap) {
    size_t di = 0;
    for(size_t i = 0; i < len && di + 1 < out_cap; i++) {
        uint8_t hi = value[i] >> 4;
        uint8_t lo = value[i] & 0x0F;
        if(hi == 0x0F) break;
        out[di++] = (char)('0' + hi);
        if(lo == 0x0F) break;
        if(di + 1 >= out_cap) break;
        out[di++] = (char)('0' + lo);
    }
    out[di] = '\0';
}

static void emv_copy_trimmed_ascii(const uint8_t* value, size_t len, char* out, size_t out_cap) {
    size_t n = len < out_cap - 1 ? len : out_cap - 1;
    memcpy(out, value, n);
    while(n > 0 && (out[n - 1] == ' ' || out[n - 1] == '\0')) n--;
    out[n] = '\0';
}

static bool emv_isdigit(char c) {
    return c >= '0' && c <= '9';
}

static void emv_track2_decode(const uint8_t* value, size_t len, EmvCardData* data) {
    char digits[41];
    size_t di = 0;
    for(size_t i = 0; i < len && di < sizeof(digits) - 1; i++) {
        uint8_t hi = value[i] >> 4;
        uint8_t lo = value[i] & 0x0F;
        if(hi <= 9)
            digits[di++] = (char)('0' + hi);
        else if(hi == 0x0D)
            digits[di++] = 'D';
        else
            break;
        if(di >= sizeof(digits) - 1) break;
        if(lo <= 9)
            digits[di++] = (char)('0' + lo);
        else if(lo == 0x0D)
            digits[di++] = 'D';
        else
            break;
    }
    digits[di] = '\0';

    char* sep = strchr(digits, 'D');
    if(!sep) return;

    size_t pan_len = (size_t)(sep - digits);
    if(!data->pan_found && pan_len > 0 && pan_len < sizeof(data->pan)) {
        memcpy(data->pan, digits, pan_len);
        data->pan[pan_len] = '\0';
        data->pan_found = true;
    }

    if(!data->exp_found) {
        const char* p = sep + 1;
        if(strlen(p) >= 4 && emv_isdigit(p[0]) && emv_isdigit(p[1]) && emv_isdigit(p[2]) &&
           emv_isdigit(p[3])) {
            data->exp_year = (uint8_t)((p[0] - '0') * 10 + (p[1] - '0'));
            data->exp_month = (uint8_t)((p[2] - '0') * 10 + (p[3] - '0'));
            data->exp_found = true;
        }
    }
}

typedef struct {
    EmvCardData* data;
    uint8_t track2[19];
    size_t track2_len;
} EmvFieldCollector;

static bool emv_field_visitor(const EmvTlv* tlv, void* context) {
    EmvFieldCollector* fc = context;
    EmvCardData* d = fc->data;

    switch(tlv->tag) {
    case 0x5A: /* PAN */
        if(!d->pan_found && tlv->length > 0) {
            emv_bcd_pan_decode(tlv->value, tlv->length, d->pan, sizeof(d->pan));
            d->pan_found = d->pan[0] != '\0';
        }
        break;
    case 0x5F24: /* Expiration date YYMMDD */
        if(!d->exp_found && tlv->length >= 2) {
            d->exp_year = emv_bcd_byte_to_dec(tlv->value[0]);
            d->exp_month = emv_bcd_byte_to_dec(tlv->value[1]);
            d->exp_found = true;
        }
        break;
    case 0x5F20: /* Cardholder name */
        if(!d->name_found && tlv->length > 0) {
            emv_copy_trimmed_ascii(tlv->value, tlv->length, d->name, sizeof(d->name));
            d->name_found = d->name[0] != '\0';
        }
        break;
    case 0x57: /* Track 2 equivalent data */
        if(fc->track2_len == 0 && tlv->length > 0 && tlv->length <= sizeof(fc->track2)) {
            memcpy(fc->track2, tlv->value, tlv->length);
            fc->track2_len = tlv->length;
        }
        break;
    case 0x5F28: /* Issuer country code */
        if(!d->country_found && tlv->length >= 2) {
            d->country_code = emv_bcd2_to_number(tlv->value);
            d->country_found = true;
        }
        break;
    case 0x9F42: /* Application currency code */
        if(!d->currency_found && tlv->length >= 2) {
            d->currency_code = emv_bcd2_to_number(tlv->value);
            d->currency_found = true;
        }
        break;
    default:
        break;
    }
    return true;
}

/* Sends one APDU and follows a 61xx status word with GET RESPONSE as needed.
 * On success, out_data and out_len are set to the response body (SW1SW2 stripped);
 * these remain valid only until the next call to emv_transact. */
static bool emv_transact(
    EmvReader* reader,
    Iso14443_4aPoller* poller,
    const uint8_t* apdu,
    size_t apdu_len,
    const uint8_t** out_data,
    size_t* out_len) {
    bit_buffer_reset(reader->tx);
    bit_buffer_copy_bytes(reader->tx, apdu, apdu_len);
    bit_buffer_reset(reader->rx);

    if(iso14443_4a_poller_send_block(poller, reader->tx, reader->rx) != Iso14443_4aErrorNone) {
        return false;
    }

    size_t rx_len = bit_buffer_get_size_bytes(reader->rx);
    if(rx_len < 2) return false;
    const uint8_t* rx_data = bit_buffer_get_data(reader->rx);
    uint8_t sw1 = rx_data[rx_len - 2];
    uint8_t sw2 = rx_data[rx_len - 1];

    if(sw1 == 0x61) {
        uint8_t gr[EMV_APDU_MAX_LEN];
        size_t gr_len = emv_apdu_build_get_response(gr, sw2);
        bit_buffer_reset(reader->tx);
        bit_buffer_copy_bytes(reader->tx, gr, gr_len);
        bit_buffer_reset(reader->rx);
        if(iso14443_4a_poller_send_block(poller, reader->tx, reader->rx) != Iso14443_4aErrorNone) {
            return false;
        }
        rx_len = bit_buffer_get_size_bytes(reader->rx);
        if(rx_len < 2) return false;
        rx_data = bit_buffer_get_data(reader->rx);
        sw1 = rx_data[rx_len - 2];
        sw2 = rx_data[rx_len - 1];
    }

    if(sw1 != 0x90 || sw2 != 0x00) return false;

    *out_data = rx_data;
    *out_len = rx_len - 2;
    return true;
}

typedef struct {
    EmvAidCandidate* candidates;
    uint8_t count;
    uint8_t max;
} EmvAidCollector;

static bool emv_aid_collect_visitor(const EmvTlv* tlv, void* context) {
    EmvAidCollector* col = context;
    if(tlv->tag == 0x4F && !tlv->constructed && tlv->length > 0 &&
       tlv->length <= EMV_AID_MAX_LEN && col->count < col->max) {
        EmvAidCandidate* c = &col->candidates[col->count++];
        memcpy(c->aid, tlv->value, tlv->length);
        c->len = (uint8_t)tlv->length;
    }
    return true;
}

static EmvReadResult
    emv_reader_perform_read(EmvReader* reader, Iso14443_4aPoller* poller, EmvCardData* data) {
    memset(data, 0, sizeof(*data));

    EmvAidCandidate candidates[EMV_MAX_CANDIDATE_AIDS];
    uint8_t candidate_count = 0;

    /* Step 1: discover AID(s) via PPSE. */
    {
        uint8_t apdu[EMV_APDU_MAX_LEN];
        size_t apdu_len = emv_apdu_build_select_ppse(apdu);
        const uint8_t* resp;
        size_t resp_len;
        if(emv_transact(reader, poller, apdu, apdu_len, &resp, &resp_len)) {
            EmvAidCollector col = {
                .candidates = candidates,
                .count = 0,
                .max = EMV_MAX_CANDIDATE_AIDS - COUNT_OF(kFallbackAids),
            };
            emv_tlv_walk(resp, resp_len, emv_aid_collect_visitor, &col);
            candidate_count = col.count;
        }
    }

    /* Step 2: append well-known fallback AIDs. */
    for(size_t i = 0; i < COUNT_OF(kFallbackAids) && candidate_count < EMV_MAX_CANDIDATE_AIDS;
        i++) {
        candidates[candidate_count++] = kFallbackAids[i];
    }

    /* Step 3: try each candidate AID until one starts an EMV application. */
    bool app_started = false;
    const uint8_t* afl_data = NULL;
    size_t afl_len = 0;

    for(uint8_t i = 0; i < candidate_count && !app_started; i++) {
        const EmvAidCandidate* cand = &candidates[i];

        uint8_t apdu[EMV_APDU_MAX_LEN];
        size_t apdu_len = emv_apdu_build_select_aid(apdu, cand->aid, cand->len);
        const uint8_t* select_resp;
        size_t select_resp_len;
        if(!emv_transact(reader, poller, apdu, apdu_len, &select_resp, &select_resp_len)) {
            continue;
        }

        size_t pdol_value_len = 0;
        EmvTlv pdol_tlv;
        if(emv_tlv_find(select_resp, select_resp_len, 0x9F38, &pdol_tlv)) {
            pdol_value_len = emv_dol_value_length(pdol_tlv.value, pdol_tlv.length);
            if(pdol_value_len > 250) pdol_value_len = 0;
        }

        apdu_len = emv_apdu_build_gpo(apdu, pdol_value_len);
        if(apdu_len == 0) continue;

        const uint8_t* gpo_resp;
        size_t gpo_resp_len;
        if(!emv_transact(reader, poller, apdu, apdu_len, &gpo_resp, &gpo_resp_len)) {
            continue;
        }

        memcpy(data->aid, cand->aid, cand->len);
        data->aid_len = cand->len;
        data->aid_found = true;
        emv_aid_db_lookup(data->aid, data->aid_len, &data->aid_name);
        app_started = true;

        EmvTlv fmt1;
        if(emv_tlv_find(gpo_resp, gpo_resp_len, 0x80, &fmt1) && fmt1.length > 2) {
            afl_data = fmt1.value + 2;
            afl_len = fmt1.length - 2;
        } else {
            EmvTlv fmt2;
            if(emv_tlv_find(gpo_resp, gpo_resp_len, 0x94, &fmt2)) {
                afl_data = fmt2.value;
                afl_len = fmt2.length;
            }
        }
    }

    if(!app_started) return EmvReadResultProtocolError;

    /* Step 4: read every record listed in the AFL and pull out the fields we want. */
    EmvFieldCollector fc = {.data = data, .track2_len = 0};
    uint16_t reads_done = 0;

    if(afl_data && afl_len % 4 == 0) {
        for(size_t g = 0; g + 4 <= afl_len && reads_done < EMV_MAX_RECORD_READS; g += 4) {
            uint8_t sfi = afl_data[g] >> 3;
            uint8_t first_rec = afl_data[g + 1];
            uint8_t last_rec = afl_data[g + 2];
            if(sfi == 0 || first_rec == 0 || last_rec < first_rec) continue;

            for(uint8_t rec = first_rec; rec <= last_rec && reads_done < EMV_MAX_RECORD_READS;
                rec++) {
                reads_done++;
                uint8_t apdu[EMV_APDU_MAX_LEN];
                size_t apdu_len = emv_apdu_build_read_record(apdu, rec, sfi);
                const uint8_t* rec_resp;
                size_t rec_resp_len;
                if(!emv_transact(reader, poller, apdu, apdu_len, &rec_resp, &rec_resp_len)) {
                    continue;
                }
                emv_tlv_walk(rec_resp, rec_resp_len, emv_field_visitor, &fc);
            }
        }
    }

    if(!data->pan_found && fc.track2_len > 0) {
        emv_track2_decode(fc.track2, fc.track2_len, data);
    }

    return data->pan_found ? EmvReadResultSuccess : EmvReadResultNoPan;
}

static NfcCommand emv_reader_poller_callback(NfcGenericEvent event, void* context) {
    EmvReader* reader = context;
    furi_assert(event.protocol == NfcProtocolIso14443_4a);
    const Iso14443_4aPollerEvent* iso_event = event.event_data;

    if(iso_event->type != Iso14443_4aPollerEventTypeReady) {
        return NfcCommandContinue;
    }

    EmvCardData data;
    EmvReadResult result = emv_reader_perform_read(reader, event.instance, &data);

    if(reader->callback) {
        reader->callback(result, &data, reader->callback_context);
    }

    return NfcCommandStop;
}

EmvReader* emv_reader_alloc(void) {
    EmvReader* reader = malloc(sizeof(EmvReader));
    reader->nfc = NULL;
    reader->poller = NULL;
    reader->tx = bit_buffer_alloc(EMV_APDU_MAX_LEN);
    reader->rx = bit_buffer_alloc(EMV_APDU_MAX_LEN);
    reader->callback = NULL;
    reader->callback_context = NULL;
    return reader;
}

void emv_reader_free(EmvReader* reader) {
    furi_assert(reader);
    furi_assert(reader->poller == NULL);
    bit_buffer_free(reader->tx);
    bit_buffer_free(reader->rx);
    free(reader);
}

void emv_reader_start(EmvReader* reader, Nfc* nfc, EmvReaderDoneCallback callback, void* context) {
    furi_assert(reader);
    furi_assert(reader->poller == NULL);

    reader->nfc = nfc;
    reader->callback = callback;
    reader->callback_context = context;
    reader->poller = nfc_poller_alloc(nfc, NfcProtocolIso14443_4a);
    nfc_poller_start(reader->poller, emv_reader_poller_callback, reader);
}

void emv_reader_stop(EmvReader* reader) {
    furi_assert(reader);
    if(reader->poller) {
        nfc_poller_stop(reader->poller);
        nfc_poller_free(reader->poller);
        reader->poller = NULL;
    }
}
