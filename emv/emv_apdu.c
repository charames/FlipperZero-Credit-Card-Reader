#include "emv_apdu.h"

#include <string.h>

static const uint8_t kPpseName[] = {
    0x32, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31}; /* "2PAY.SYS.DDF01" */

size_t emv_apdu_build_select_ppse(uint8_t* buf) {
    size_t pos = 0;
    buf[pos++] = 0x00; /* CLA */
    buf[pos++] = 0xA4; /* INS: SELECT */
    buf[pos++] = 0x04; /* P1: select by name */
    buf[pos++] = 0x00; /* P2 */
    buf[pos++] = sizeof(kPpseName); /* Lc */
    memcpy(buf + pos, kPpseName, sizeof(kPpseName));
    pos += sizeof(kPpseName);
    buf[pos++] = 0x00; /* Le */
    return pos;
}

size_t emv_apdu_build_select_aid(uint8_t* buf, const uint8_t* aid, uint8_t aid_len) {
    if(aid_len == 0 || aid_len > 16) return 0;

    size_t pos = 0;
    buf[pos++] = 0x00;
    buf[pos++] = 0xA4;
    buf[pos++] = 0x04;
    buf[pos++] = 0x00;
    buf[pos++] = aid_len;
    memcpy(buf + pos, aid, aid_len);
    pos += aid_len;
    buf[pos++] = 0x00;
    return pos;
}

size_t emv_apdu_build_gpo(uint8_t* buf, const uint8_t* pdol_value, size_t pdol_value_len) {
    if(pdol_value_len > 250) return 0;

    size_t pos = 0;
    buf[pos++] = 0x80; /* CLA */
    buf[pos++] = 0xA8; /* INS: GET PROCESSING OPTIONS */
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = (uint8_t)(pdol_value_len + 2); /* Lc: template tag+len+value */
    buf[pos++] = 0x83; /* Command Template tag */
    buf[pos++] = (uint8_t)pdol_value_len;
    if(pdol_value_len > 0) memcpy(buf + pos, pdol_value, pdol_value_len);
    pos += pdol_value_len;
    buf[pos++] = 0x00; /* Le */
    return pos;
}

size_t emv_apdu_build_read_record(uint8_t* buf, uint8_t record_num, uint8_t sfi) {
    size_t pos = 0;
    buf[pos++] = 0x00;
    buf[pos++] = 0xB2; /* INS: READ RECORD */
    buf[pos++] = record_num;
    buf[pos++] = (uint8_t)((sfi << 3) | 0x04); /* P2: SFI, "read record P1" mode */
    buf[pos++] = 0x00; /* Le */
    return pos;
}

size_t emv_apdu_build_get_response(uint8_t* buf, uint8_t le) {
    size_t pos = 0;
    buf[pos++] = 0x00;
    buf[pos++] = 0xC0; /* INS: GET RESPONSE */
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = le;
    return pos;
}
