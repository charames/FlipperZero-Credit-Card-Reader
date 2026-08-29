#pragma once

#include <stdint.h>
#include <stddef.h>

/** Max PDOL value buffer we'll ever build (matches EMV_APDU_MAX_LEN headroom). */
#define EMV_PDOL_MAX_VALUE_LEN (250)

/** Builds the value bytes to send back for a card's PDOL (Processing Options Data
 * Object List), for tag 83 of the GET PROCESSING OPTIONS command.
 *
 * Many contactless cards (notably Visa) reject GPO with SW 6985 "conditions not
 * satisfied" if given an all-zero PDOL response, because fields like the Terminal
 * Transaction Qualifiers (9F66) declare terminal capabilities the card checks
 * before proceeding. For every PDOL tag we recognize, we supply the same
 * plausible terminal-context default values a generic contactless reader would
 * use; unrecognized tags are zero-filled, which is safe for informational ones
 * (like Terminal Verification Results).
 *
 * @param pdol raw PDOL bytes from the card (tag 9F38 in the SELECT AID response)
 * @param pdol_len length of pdol
 * @param out buffer to write the value bytes into, at least EMV_PDOL_MAX_VALUE_LEN long
 * @param out_cap capacity of out
 * @return number of bytes written to out, or 0 if the PDOL was malformed or too long
 */
size_t emv_pdol_build_value(const uint8_t* pdol, size_t pdol_len, uint8_t* out, size_t out_cap);
