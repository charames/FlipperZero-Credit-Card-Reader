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

/** Visitor callback invoked for every (tag, length) entry in a Data Object List. */
typedef bool (*EmvDolVisitor)(uint16_t tag, uint8_t length, void* context);

/** Walks a Data Object List, invoking visitor for each tag+length entry in order.
 * Unlike TLV records, a DOL entry's length is always a single byte (EMV Book 3),
 * and there is no value present in the list itself.
 */
void emv_dol_walk(const uint8_t* dol, size_t dol_len, EmvDolVisitor visitor, void* context);
