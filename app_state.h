#pragma once

#include "logic.h"
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <gui/view_dispatcher.h>

#define SCAN_DIR "/ext/subghz"

typedef enum {
    SubDupFinderViewSubmenu,
    SubDupFinderViewGroups,
    SubDupFinderViewFiles,
    SubDupFinderViewConfirm,
    SubDupFinderViewCredits,
    SubDupFinderViewPopup,
    SubDupFinderViewSummary,
} SubDupFinderView;

typedef enum {
    SubDupFinderSubmenuIndexScan,
    SubDupFinderSubmenuIndexCredits,
} SubDupFinderSubmenuIndex;

typedef struct {
    ViewDispatcher *view_dispatcher;
    Submenu *main_submenu;
    Submenu *groups_submenu;
    Submenu *files_in_group_submenu;
    DialogEx *confirm_dialog;
    DialogEx *summary_dialog;
    Widget *credits_widget;
    Popup *popup;
    HashDatabase db;
    ScanStats scan_stats;
    char scan_summary[96];
    char scanned_dir[FULL_PATH_LEN];
    char selected_path[FULL_PATH_LEN];
    char selected_name[APP_MAX_PATH_LEN];
    size_t selected_group_index;
} SubDupFinderApp;
