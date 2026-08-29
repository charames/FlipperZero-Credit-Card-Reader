#include "emv_tag_names.h"

#include <stddef.h>

/* Standard EMV Book 3/4 tag names (publicly documented in the EMV Contactless
 * and Contact specifications), covering the tags typically seen while reading
 * a contactless bank card. */
typedef struct {
    uint16_t tag;
    const char* name;
} EmvTagNameEntry;

static const EmvTagNameEntry kTagNames[] = {
    {0x42, "Issuer Identification Number"},
    {0x4F, "Application ID (AID)"},
    {0x50, "Application Label"},
    {0x56, "Track 1 Data"},
    {0x57, "Track 2 Equivalent Data"},
    {0x5A, "PAN"},
    {0x5F20, "Cardholder Name"},
    {0x5F24, "Application Expiration Date"},
    {0x5F25, "Application Effective Date"},
    {0x5F28, "Issuer Country Code"},
    {0x5F2A, "Transaction Currency Code"},
    {0x5F2D, "Language Preference"},
    {0x5F30, "Service Code"},
    {0x5F34, "PAN Sequence Number"},
    {0x61, "Application Template"},
    {0x6F, "FCI Template"},
    {0x70, "Record Template"},
    {0x77, "Response Message Template Format 2"},
    {0x80, "Response Message Template Format 1"},
    {0x82, "Application Interchange Profile"},
    {0x83, "Command Template"},
    {0x84, "Dedicated File Name"},
    {0x87, "Application Priority Indicator"},
    {0x88, "Short File Identifier"},
    {0x8A, "Authorisation Response Code"},
    {0x8C, "CDOL1"},
    {0x8D, "CDOL2"},
    {0x8E, "CVM List"},
    {0x8F, "CA Public Key Index"},
    {0x90, "Issuer Public Key Certificate"},
    {0x92, "Issuer Public Key Remainder"},
    {0x93, "Signed Static App Data"},
    {0x94, "Application File Locator"},
    {0x95, "Terminal Verification Results"},
    {0x97, "Transaction Certificate DOL"},
    {0x98, "Transaction Certificate Hash Value"},
    {0x9A, "Transaction Date"},
    {0x9C, "Transaction Type"},
    {0x9F02, "Amount, Authorised"},
    {0x9F03, "Amount, Other"},
    {0x9F07, "Application Usage Control"},
    {0x9F08, "Application Version Number"},
    {0x9F0D, "Issuer Action Code - Default"},
    {0x9F0E, "Issuer Action Code - Denial"},
    {0x9F0F, "Issuer Action Code - Online"},
    {0x9F10, "Issuer Application Data"},
    {0x9F12, "Application Preferred Name"},
    {0x9F1A, "Terminal Country Code"},
    {0x9F1F, "Track 1 Discretionary Data"},
    {0x9F26, "Application Cryptogram"},
    {0x9F27, "Cryptogram Information Data"},
    {0x9F32, "Issuer Public Key Exponent"},
    {0x9F33, "Terminal Capabilities"},
    {0x9F34, "CVM Results"},
    {0x9F35, "Terminal Type"},
    {0x9F36, "Application Transaction Counter"},
    {0x9F37, "Unpredictable Number"},
    {0x9F38, "Processing Options DOL (PDOL)"},
    {0x9F40, "Additional Terminal Capabilities"},
    {0x9F42, "Application Currency Code"},
    {0x9F44, "Application Currency Exponent"},
    {0x9F45, "Data Authentication Code"},
    {0x9F46, "ICC Public Key Certificate"},
    {0x9F47, "ICC Public Key Exponent"},
    {0x9F48, "ICC Public Key Remainder"},
    {0x9F49, "Dynamic DOL (DDOL)"},
    {0x9F4A, "Static Data Auth Tag List"},
    {0x9F4B, "Signed Dynamic Application Data"},
    {0x9F4C, "ICC Dynamic Number"},
    {0x9F4D, "Log Entry"},
    {0x9F4F, "Log Format"},
    {0x9F53, "Consumer Device CVM Results"},
    {0x9F58, "Merchant Type Indicator"},
    {0x9F59, "Terminal Transaction Information"},
    {0x9F5A, "Terminal Transaction Type"},
    {0x9F66, "Terminal Transaction Qualifiers"},
    {0xA5, "FCI Proprietary Template"},
    {0xBF0C, "FCI Issuer Discretionary Data"},
};

const char* emv_tag_name(uint16_t tag) {
    for(size_t i = 0; i < sizeof(kTagNames) / sizeof(kTagNames[0]); i++) {
        if(kTagNames[i].tag == tag) return kTagNames[i].name;
    }
    return NULL;
}
