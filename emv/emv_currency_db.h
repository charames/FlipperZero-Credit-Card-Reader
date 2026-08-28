#pragma once

#include <stdint.h>
#include <stdbool.h>

/** Look up the ISO 4217 alpha-3 currency code for an EMV numeric currency code.
 * @param code numeric currency code (e.g. 840 for USD), as read from tag 9F42
 * @param name_out set to a pointer to a static 3-letter code string on success
 * @return true if found
 */
bool emv_currency_db_lookup(uint16_t code, const char** name_out);
