#pragma once

#include <stdint.h>
#include <stddef.h>

/** Max APDU size we ever build or expect back (records/FCI templates are small). */
#define EMV_APDU_MAX_LEN (255)

/** Builds "SELECT 2PAY.SYS.DDF01" (PPSE), the entry point for contactless EMV apps.
 * @return number of bytes written to buf.
 */
size_t emv_apdu_build_select_ppse(uint8_t* buf);

/** Builds "SELECT <AID>".
 * @return number of bytes written to buf, or 0 if aid_len is out of range.
 */
size_t emv_apdu_build_select_aid(uint8_t* buf, const uint8_t* aid, uint8_t aid_len);

/** Builds "GET PROCESSING OPTIONS" with a zero-filled PDOL value of the given length
 * (pdol_value_len may be 0 if the card presented no PDOL).
 * @return number of bytes written to buf, or 0 if pdol_value_len is out of range.
 */
size_t emv_apdu_build_gpo(uint8_t* buf, size_t pdol_value_len);

/** Builds "READ RECORD <record_num> from SFI <sfi>". */
size_t emv_apdu_build_read_record(uint8_t* buf, uint8_t record_num, uint8_t sfi);

/** Builds "GET RESPONSE" with the given expected length (from a 61xx status word). */
size_t emv_apdu_build_get_response(uint8_t* buf, uint8_t le);
