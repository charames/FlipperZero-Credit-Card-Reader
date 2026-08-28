#pragma once

#include <stdint.h>
#include <stdbool.h>

/** Look up the ISO 3166-1 alpha-3 country code for an EMV numeric country code.
 * @param code numeric country code (e.g. 840 for the USA), as read from tag 5F28
 * @param name_out set to a pointer to a static 3-letter code string on success
 * @return true if found
 */
bool emv_country_db_lookup(uint16_t code, const char** name_out);
