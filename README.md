# FlipperZero Credit Card Reader

A standalone Flipper Zero app that reads a contactless EMV bank card and displays
its PAN (card number), expiry date, cardholder name, and issuer/currency info.

> **Use responsibly.** Only read cards you own or have explicit authorization to
> test. Reading a card's exposed EMV data does not let you clone it or make
> payments with it — no CVV, no cryptographic keys, and no way to authorize a
> transaction are ever exposed by this data.

## Background

The official [`flipperzero-firmware`](https://github.com/flipperdevices/flipperzero-firmware)
shipped this as part of the built-in NFC app until April 2023, when it was removed in
[commit `7ac7b708`](https://github.com/flipperdevices/flipperzero-firmware/commit/7ac7b708840d29daf8f358f629119f61b859aaa0)
("[FL-3241] NFC disable EMV support") following media coverage that mischaracterized
the feature. Technically nothing changed: an EMV contactless card broadcasts its PAN,
expiry, and (often) cardholder name **unencrypted** to any reader that asks — exactly
what a payment terminal does at every tap-to-pay transaction. Flipper Devices pulled
the feature to sidestep the PR fallout rather than for any technical reason, and the
NFC stack has since been rewritten around a plugin-based protocol architecture that
has no path left for it.

This app rebuilds that functionality from scratch as an external FAP (Flipper
Application Package), talking directly to the stable ISO14443-4A poller API that
ships with the current firmware SDK, so it doesn't require patching or rebuilding
the firmware itself.

## What it reads

- Card number (PAN)
- Expiry date
- Cardholder name (when present on the card)
- Card scheme (Visa / Mastercard / Amex / etc., via AID)
- Issuer country
- Currency

The AID, country-code, and currency-code lookup tables are generated from the same
resource files the original built-in reader used
(`applications/main/nfc/resources/nfc/assets/{aid,country_code,currency_code}.nfc`
in the official firmware repo), baked into flash instead of loaded from SD card so
the app is self-contained. Regenerate them with:

```sh
python scripts/gen_emv_tables.py /path/to/flipperzero-firmware
```

## How it works

1. `SELECT 2PAY.SYS.DDF01` (PPSE) to discover the card's AID(s); falls back to a
   short list of well-known scheme AIDs if PPSE isn't supported.
2. `SELECT AID` on the first candidate that works.
3. `GET PROCESSING OPTIONS` (with a zero-filled PDOL if the card requires one) to
   get the Application File Locator (AFL).
4. `READ RECORD` on every record the AFL points at, pulling PAN (tag `5A`), expiry
   (`5F24`), cardholder name (`5F20`), issuer country (`5F28`), currency (`9F42`),
   and Track 2 equivalent data (`57`) as a fallback source for PAN/expiry.

See `emv/emv_reader.c` for the full state machine and `emv/emv_tlv.c` for the
BER-TLV parser it's built on.

## Building & installing

Requires [`ufbt`](https://github.com/flipperdevices/flipperzero-ufbt):

```sh
pip install ufbt
ufbt              # build dist/credit_card_reader.fap
ufbt launch       # build, install to a connected Flipper over USB, and launch it
```

No Flipper connected? Build with `ufbt`, then copy `dist/credit_card_reader.fap`
onto the SD card's `apps/NFC/` folder (over USB mass storage, qFlipper, or the
mobile app) and launch it from the on-device app menu instead.

## Project layout

```
application.fam           - FAP manifest
credit_card_reader.c       - app entry point / view dispatcher wiring
credit_card_reader_app.h   - app struct, view/event enums
scenes/                    - scan screen + result screen
emv/                       - EMV protocol logic (TLV parser, APDU builder, reader
                             state machine, AID/country/currency lookup tables)
scripts/                   - table/icon generators
```

## Acknowledgments

The AID, ISO 3166 country-code, and ISO 4217 currency-code tables in `emv/`, the
default PDOL terminal-context values in `emv/emv_pdol.c` (needed because several
issuers, notably Visa, reject GET PROCESSING OPTIONS with an all-zero PDOL
response), and several low-level NFC API usage patterns, are derived from
[flipperdevices/flipperzero-firmware](https://github.com/flipperdevices/flipperzero-firmware).

## License

GPL-3.0-or-later, matching the upstream firmware this project builds on. See
[`LICENSE`](LICENSE) for the full text.
