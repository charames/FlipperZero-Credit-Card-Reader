#include "emv_tlv.h"

/* Parses one tag+length header starting at *pos (which is advanced past the
 * header, to the start of the value). Returns false if the buffer does not
 * contain a well-formed header or the encoded length overruns `end`.
 */
static bool emv_tlv_parse_header(
    const uint8_t** pos,
    const uint8_t* end,
    uint16_t* tag_out,
    bool* constructed_out,
    size_t* length_out) {
    const uint8_t* p = *pos;
    if(p >= end) return false;

    uint8_t first = *p++;
    uint16_t tag = first;
    *constructed_out = (first & 0x20) != 0;

    if((first & 0x1F) == 0x1F) {
        if(p >= end) return false;
        uint8_t second = *p++;
        tag = ((uint16_t)first << 8) | second;
        /* EMV tags are at most 2 bytes; skip any further continuation bytes
         * we cannot represent rather than misparsing them as data. */
        while((second & 0x80) && p < end) {
            second = *p++;
        }
    }

    if(p >= end) return false;
    uint8_t len_byte = *p++;
    size_t length;
    if((len_byte & 0x80) == 0) {
        length = len_byte;
    } else {
        uint8_t num_len_bytes = len_byte & 0x7F;
        if(num_len_bytes == 0 || num_len_bytes > 2) return false;
        if((size_t)(end - p) < num_len_bytes) return false;
        length = 0;
        for(uint8_t i = 0; i < num_len_bytes; i++) {
            length = (length << 8) | *p++;
        }
    }

    if((size_t)(end - p) < length) return false;

    *pos = p;
    *tag_out = tag;
    *length_out = length;
    return true;
}

/* Returns false if the walk was aborted early by the visitor. */
static bool emv_tlv_walk_internal(const uint8_t* data, size_t len, EmvTlvVisitor visitor, void* context) {
    const uint8_t* pos = data;
    const uint8_t* end = data + len;

    while(pos < end) {
        uint16_t tag;
        bool constructed;
        size_t value_len;
        if(!emv_tlv_parse_header(&pos, end, &tag, &constructed, &value_len)) break;

        const uint8_t* value = pos;

        EmvTlv node = {
            .tag = tag,
            .constructed = constructed,
            .value = value,
            .length = value_len,
        };
        if(!visitor(&node, context)) return false;

        if(constructed) {
            if(!emv_tlv_walk_internal(value, value_len, visitor, context)) return false;
        }

        pos = value + value_len;
    }

    return true;
}

void emv_tlv_walk(const uint8_t* data, size_t len, EmvTlvVisitor visitor, void* context) {
    emv_tlv_walk_internal(data, len, visitor, context);
}

typedef struct {
    uint16_t tag;
    EmvTlv result;
    bool found;
} EmvTlvFindCtx;

static bool emv_tlv_find_visitor(const EmvTlv* tlv, void* context) {
    EmvTlvFindCtx* ctx = context;
    if(tlv->tag == ctx->tag) {
        ctx->result = *tlv;
        ctx->found = true;
        return false;
    }
    return true;
}

bool emv_tlv_find(const uint8_t* data, size_t len, uint16_t tag, EmvTlv* out_tlv) {
    EmvTlvFindCtx ctx = {.tag = tag, .found = false};
    emv_tlv_walk(data, len, emv_tlv_find_visitor, &ctx);
    if(ctx.found) *out_tlv = ctx.result;
    return ctx.found;
}

size_t emv_dol_value_length(const uint8_t* dol, size_t dol_len) {
    const uint8_t* pos = dol;
    const uint8_t* end = dol + dol_len;
    size_t total = 0;

    while(pos < end) {
        uint8_t first = *pos++;
        if((first & 0x1F) == 0x1F) {
            if(pos >= end) break;
            uint8_t second = *pos++;
            while((second & 0x80) && pos < end) {
                second = *pos++;
            }
        }
        if(pos >= end) break;
        total += *pos++;
    }

    return total;
}
