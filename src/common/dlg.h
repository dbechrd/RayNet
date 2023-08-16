#pragma once
#include "common.h"

enum DialogTagType {
    DIALOG_TAG_NONE,
    DIALOG_TAG_HOVER_TIP,
    DIALOG_TAG_LINK,
};

struct DialogTag {
    DialogTagType type;

    // the full tag (useful for skipping while rendering client-side)
    std::string_view view{};

    // the part that's always visible and does something when you hover/click
    std::string_view text{};

    // HOVER_TIP: the text to display in the pop-up
    // LINK:      dialog_key of the dialog to go to
    std::string_view data{};
};

struct Dialog {
    uint32_t         id   {};
    std::string_view key  {};
    std::string_view msg  {};
    DialogTag        tags [SV_MAX_DIALOG_TAGS]{};
};

struct DialogLibrary {
    uint32_t nextId = 1;

    std::vector<const char *> dialog_files{};
    std::vector<Dialog> dialogs{};
    std::unordered_map<uint32_t, size_t> dialog_idx_by_id{};
    std::unordered_map<std::string_view, size_t> dialog_idx_by_key{};

    Dialog &Alloc(std::string_view key) {
        Dialog &dialog = dialogs.emplace_back();
        dialog.id = nextId++;
        dialog.key = key;

        size_t dialogIdx = dialogs.size() - 1;
        dialog_idx_by_id[dialog.id] = dialogIdx;
        dialog_idx_by_key[dialog.key] = dialogIdx;

        return dialog;
    }

    Dialog *FindById(uint32_t id) {
        const auto &kv = dialog_idx_by_id.find(id);
        if (kv != dialog_idx_by_id.end()) {
            return &dialogs[kv->second];
        }
        return 0;
    }

    Dialog *FindByKey(std::string_view key) {
        const auto &kv = dialog_idx_by_key.find(key);
        if (kv != dialog_idx_by_key.end()) {
            return &dialogs[kv->second];
        }
        return 0;
    }
};

extern DialogLibrary dialog_library;

Err LoadDialogFile(std::string path);
