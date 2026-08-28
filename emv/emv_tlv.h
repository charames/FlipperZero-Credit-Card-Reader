#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** A single decoded BER-TLV node. */
typedef struct {
    uint16_t tag; /**< EMV tags are at most 2 bytes; that is all this parser supports. */
    bool constructed; /**< True if the tag's constructed bit was set. */
    const uint8_t* value;
    size_t length;
} EmvTlv;

/** Visitor callback invoked for every TLV node found while walking a blob,
 * including constructed (nested) nodes themselves as well as their children.
 * Return false to abort the walk early.
 */
typedef bool (*EmvTlvVisitor)(const EmvTlv* tlv, void* context);

/** Recursively walks all TLV nodes in [data, data+len), descending into
 * constructed tags, invoking visitor for each node encountered.
 */
void emv_tlv_walk(const uint8_t* data, size_t len, EmvTlvVisitor visitor, void* context);

/** Finds the first occurrence of `tag` anywhere in the TLV tree (including nested
 * constructed tags).
 * @return true if found, with out_tlv filled in.
 */
bool emv_tlv_find(const uint8_t* data, size_t len, uint16_t tag, EmvTlv* out_tlv);

/** Parses a Data Object List (DOL, e.g. a PDOL) and returns the total byte length
 * of the values that would need to be supplied for it (a DOL only lists tag+length
 * pairs, no values).
 */
size_t emv_dol_value_length(const uint8_t* dol, size_t dol_len);
