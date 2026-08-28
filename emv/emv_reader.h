#pragma once

#include "emv_types.h"

#include <nfc/nfc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EmvReader EmvReader;

/** Called once the read attempt has concluded (success or failure), from the
 * Nfc worker thread. `data` is only valid for the duration of the callback.
 */
typedef void (*EmvReaderDoneCallback)(EmvReadResult result, const EmvCardData* data, void* context);

EmvReader* emv_reader_alloc(void);
void emv_reader_free(EmvReader* reader);

/** Starts polling for an ISO14443-4A (contactless EMV) card on `nfc`.
 * Waits indefinitely for a card to be presented; call emv_reader_stop() to cancel.
 */
void emv_reader_start(EmvReader* reader, Nfc* nfc, EmvReaderDoneCallback callback, void* context);

void emv_reader_stop(EmvReader* reader);

#ifdef __cplusplus
}
#endif
