#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <notification/notification_messages.h>

#include <nfc/nfc.h>

#include "emv/emv_reader.h"
#include "scenes/scenes.h"

typedef struct {
    Gui* gui;
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;
    NotificationApp* notifications;

    Popup* popup;
    Widget* widget;

    Nfc* nfc;
    EmvReader* emv_reader;

    EmvCardData card_data;
    EmvReadResult read_result;
} CreditCardReaderApp;

typedef enum {
    CreditCardReaderAppViewPopup,
    CreditCardReaderAppViewWidget,
} CreditCardReaderAppView;

typedef enum {
    CreditCardReaderAppCustomEventReadDone,
} CreditCardReaderAppCustomEvent;
