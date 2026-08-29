#pragma once

#include <stdint.h>

/** Looks up the standard EMV Book 3/4 name for a BER-TLV tag (e.g. 0x5A -> "PAN").
 * @return the name, or NULL if the tag isn't in the known-tag table.
 */
const char* emv_tag_name(uint16_t tag);
