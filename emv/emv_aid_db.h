#pragma once

#include <stdint.h>
#include <stdbool.h>

/** Look up a human-readable scheme name for an EMV Application ID.
 *
 * Tries an exact length+prefix match first, then falls back to the
 * longest known AID whose first 5+ bytes match (RID-level match), which
 * covers scheme variants not individually listed in the table.
 *
 * @param aid raw AID bytes
 * @param aid_len number of AID bytes (5-16)
 * @param name_out set to a pointer to a static string on success
 * @return true if a name was found
 */
bool emv_aid_db_lookup(const uint8_t* aid, uint8_t aid_len, const char** name_out);
