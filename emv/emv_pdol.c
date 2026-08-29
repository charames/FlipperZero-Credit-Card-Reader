#include "emv_pdol.h"
#include "emv_tlv.h"

#include <string.h>

typedef struct {
    uint16_t tag;
    uint8_t len;
    const uint8_t data[6];
} EmvPdolDefault;

/* Default terminal-context values for commonly-requested PDOL tags. Contactless
 * cards use these to decide whether to proceed past GET PROCESSING OPTIONS; an
 * all-zero response is frequently rejected (SW 6985), particularly by Visa, since
 * a zeroed Terminal Transaction Qualifiers (9F66) declares no supported reading
 * method at all. */
static const EmvPdolDefault kDefaults[] = {
    {0x9F66, 4, {0x79, 0x00, 0x40, 0x80}}, /* Terminal Transaction Qualifiers */
    {0x9F40, 4, {0x79, 0x00, 0x40, 0x80}}, /* Additional Terminal Capabilities */
    {0x9F02, 6, {0x00, 0x00, 0x00, 0x00, 0x10, 0x00}}, /* Amount, Authorised */
    {0x9F03, 6, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, /* Amount, Other */
    {0x9F1A, 2, {0x01, 0x24}}, /* Terminal Country Code */
    {0x5F2A, 2, {0x01, 0x24}}, /* Transaction Currency Code */
    {0x95, 5, {0x00, 0x00, 0x00, 0x00, 0x00}}, /* Terminal Verification Results */
    {0x9A, 3, {0x19, 0x01, 0x01}}, /* Transaction Date */
    {0x9C, 1, {0x00}}, /* Transaction Type */
    {0x9F37, 4, {0x82, 0x3D, 0xDE, 0x7A}}, /* Unpredictable Number */
    {0x9F58, 1, {0x01}}, /* Merchant Type Indicator */
    {0x9F59, 3, {0xC8, 0x80, 0x00}}, /* Terminal Transaction Information */
    {0x9F5A, 1, {0x00}}, /* Terminal Transaction Type */
};

typedef struct {
    uint8_t* out;
    size_t cap;
    size_t pos;
    bool overflowed;
} EmvPdolBuildCtx;

static bool emv_pdol_build_visitor(uint16_t tag, uint8_t length, void* context) {
    EmvPdolBuildCtx* ctx = context;

    if(ctx->pos + length > ctx->cap) {
        ctx->overflowed = true;
        return false;
    }

    const EmvPdolDefault* match = NULL;
    for(size_t i = 0; i < sizeof(kDefaults) / sizeof(kDefaults[0]); i++) {
        if(kDefaults[i].tag == tag) {
            match = &kDefaults[i];
            break;
        }
    }

    if(match && match->len == length) {
        memcpy(ctx->out + ctx->pos, match->data, length);
    } else {
        memset(ctx->out + ctx->pos, 0, length);
    }
    ctx->pos += length;

    return true;
}

size_t emv_pdol_build_value(const uint8_t* pdol, size_t pdol_len, uint8_t* out, size_t out_cap) {
    EmvPdolBuildCtx ctx = {.out = out, .cap = out_cap, .pos = 0, .overflowed = false};
    emv_dol_walk(pdol, pdol_len, emv_pdol_build_visitor, &ctx);
    return ctx.overflowed ? 0 : ctx.pos;
}
