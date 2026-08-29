#pragma once

#include <stdint.h>
#include <stdbool.h>

#define EMV_AID_MAX_LEN (16)
#define EMV_PAN_MAX_DIGITS (19)
#define EMV_NAME_MAX_LEN (26)

#define EMV_RAW_FIELD_MAX (48)
#define EMV_RAW_VALUE_MAX (24)

/** One raw BER-TLV data element as returned by the card, tag + value bytes,
 * kept for display alongside the curated fields above. Constructed (template)
 * tags are not recorded, only their leaf contents. */
typedef struct {
    uint16_t tag;
    uint8_t length; /**< Original length; may exceed EMV_RAW_VALUE_MAX if truncated. */
    uint8_t value[EMV_RAW_VALUE_MAX];
} EmvRawField;

typedef enum {
    EmvReadResultSuccess, /**< PAN (and usually more) was read successfully. */
    EmvReadResultNoPan, /**< An EMV application was found and started, but no PAN could be read. */
    EmvReadResultProtocolError, /**< Could not find/start any EMV application on the card. */
} EmvReadResult;

typedef struct {
    uint8_t aid[EMV_AID_MAX_LEN];
    uint8_t aid_len;
    bool aid_found;
    const char* aid_name; /**< Points into the static AID table, or NULL if unknown. */

    char pan[EMV_PAN_MAX_DIGITS + 1];
    bool pan_found;

    uint8_t exp_month; /**< 1-12 */
    uint8_t exp_year; /**< 2-digit, add 2000 */
    bool exp_found;

    char name[EMV_NAME_MAX_LEN + 1];
    bool name_found;

    uint16_t country_code;
    bool country_found;

    uint16_t currency_code;
    bool currency_found;

    EmvRawField raw_fields[EMV_RAW_FIELD_MAX];
    uint8_t raw_field_count;
} EmvCardData;
