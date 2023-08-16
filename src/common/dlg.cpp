#include "dlg.h"

DialogLibrary dialog_library{};

bool IsWhitespace(char c)
{
    switch (c) {
        case ' ': case '\t': case '\r': case '\n': {
            return true;
        }
    }
    return false;
}

void SkipWhitespace(char *&buf, bool skipNewlines = false)
{
    while (*buf) {
        if (IsWhitespace(*buf)) {
            if (*buf == '\n' && !skipNewlines) {
                break;
            }
        } else {
            break;
        }
        buf++;
    }
}

bool Expect(char *&buf, char c)
{
    if (*buf && *buf == c) {
        buf++;
        return true;
    }
    return false;
}

std::string_view ReadLine(char *&buf)
{
    const char *str = buf;
    while (*buf && *buf != '\n') {
        buf++;
    }
    return { str, (size_t)(buf - str) };
}

std::string_view ReadUntilChar(std::string_view view, char c)
{
    const char *buf = view.data();
    const char *start = buf;
    size_t len = 0;

    while (*buf && *buf != c && (buf - start) < view.size()) {
        len++;
        buf++;
    }

    return { start, len };
}

std::string_view ReadUntilLineStartsWith(char *&buf, char c)
{
    char *start = buf;
    char *lastChar = 0;

    bool done = false;
    while (*buf && !done) {
        switch (*buf) {
            case '\n': {
                if (*(buf + 1) == c) {
                    done = true;
                }
                break;
            }
        }

        if (!IsWhitespace(*buf)) {
            lastChar = buf;
        }
        buf++;
    }

    if (lastChar) {
        return { start, (size_t)((lastChar + 1) - start) };
    } else {
        return { start, 0 };
    }
}

Err ParseTag(std::string_view str, DialogTag &tag)
{
    Err err = RN_SUCCESS;
    char *buf = (char *)str.data();

    // Read dialog tag text
    if (!Expect(buf, '[')) {
        assert(!"expected tag text to open with '['");
        return RN_BAD_FILE_READ;
    }

    std::string_view tagText = ReadUntilChar({ buf, str.size() - (buf - str.data()) }, ']');
    if (!tagText.size()) {
        assert(!"expected tag text to be at least 1 character");
        return RN_BAD_FILE_READ;
    }
    buf += tagText.size();

    if (!Expect(buf, ']')) {
        assert(!"expected tag text to close with ']'");
        return RN_BAD_FILE_READ;
    }

    // Read dialog tag key
    if (!Expect(buf, '(')) {
        assert(!"expected tag data to open with '('");
        return RN_BAD_FILE_READ;
    }
    if (!Expect(buf, '#')) {
        assert(!"expected tag data to start with '#'");
        return RN_BAD_FILE_READ;
    }

    std::string_view tagData = ReadUntilChar({ buf, str.size() - (buf - str.data()) }, ')');
    if (!tagData.size()) {
        assert(!"expected tag data to be at least 1 character");
        return RN_BAD_FILE_READ;
    }
    buf += tagData.size();

    if (!Expect(buf, ')')) {
        assert(!"expected tag data to close with ')'");
        return RN_BAD_FILE_READ;
    }

    std::string_view tagView{ str.data(), (size_t)(buf - str.data()) };

    if (tagData[0] == ' ') {
        buf = (char *)&tagData[1];
        if (!Expect(buf, '"')) {
            assert(!"expected hover tip tag data to open with '\"'");
            return RN_BAD_FILE_READ;
        }

        std::string_view hoverTip = ReadUntilChar({ buf, str.size() - (buf - str.data()) }, '"');
        if (!hoverTip.size()) {
            assert(!"expected hover tip tag data to be at least 1 character");
            return RN_BAD_FILE_READ;
        }
        buf += hoverTip.size();

        if (!Expect(buf, '"')) {
            assert(!"expected hover tip tag data to close with '\"'");
            return RN_BAD_FILE_READ;
        }
        tag.type = DIALOG_TAG_HOVER_TIP;
        tag.view = tagView;
        tag.text = tagText;
        tag.data = hoverTip;
    } else {
        tag.type = DIALOG_TAG_LINK;
        tag.view = tagView;
        tag.text = tagText;
        tag.data = tagData;
    }

    return err;
}

Err ParseOptions(Dialog &dialog)
{
    Err err = RN_SUCCESS;
    const char *buf = dialog.msg.data();
    int tag_idx = 0;

    while (*buf && (buf - dialog.msg.data()) < dialog.msg.size() && tag_idx < SV_MAX_DIALOG_TAGS && !err) {
        switch (*buf) {
            case '[': {
                size_t subLen = dialog.msg.size() - (buf - dialog.msg.data());
                err = ParseTag({ buf, subLen }, dialog.tags[tag_idx]);
                if (!err) {
                    tag_idx++;
                }
                break;
            }
        }
        buf++;
    }

    return err;
}

Err ParseDialog(char *&buf)
{
    if (!Expect(buf, '#')) {
        assert(!"expected dialog key starting with '#'");
        return RN_BAD_FILE_READ;
    }
    if (!Expect(buf, ' ')) {
        assert(!"expected ' ' after '#' in dialog key");
        return RN_BAD_FILE_READ;
    }

    SkipWhitespace(buf);

    std::string_view key = ReadLine(buf);
    if (!key.size()) {
        assert(!"expected dialog key to have length > 0");
        return RN_BAD_FILE_READ;
    }

    if (dialog_library.dialog_idx_by_key.contains(key)) {
        assert(!"duplicate dialog key");
        return RN_BAD_FILE_READ;
    }

    if (!Expect(buf, '\n')) {
        assert(!"expected newline after dialog key");
        return RN_BAD_FILE_READ;
    }

    SkipWhitespace(buf);

    std::string_view msg = ReadUntilLineStartsWith(buf, '#');
    // NOTE(dlb): DIALOG_CANCEL has no message, so perhaps don't need this check?
    //if (!msg.size()) {
    //    assert(!"expected message after dialog key");
    //    return RN_BAD_FILE_READ;
    //}

    Dialog &dialog = dialog_library.Alloc(key);
    dialog.msg = msg;

    ParseOptions(dialog);

    return RN_SUCCESS;
}

Err ParseDialogFile(char *buf)
{
    Err err = RN_SUCCESS;

    SkipWhitespace(buf);

    while (*buf) {
        if (*buf == '#') {
            ParseDialog(buf);
        } else {
            break;
        }
    }

    if (*buf) {
        assert(!"unexpected character");
        return RN_BAD_FILE_READ;
    }

    return err;
}

Err LoadDialogFile(std::string path)
{
    Err err = RN_SUCCESS;

    char *buf = LoadFileText(path.c_str());
    if (buf) {
        err = ParseDialogFile(buf);
        dialog_library.dialog_files.push_back(buf);
    } else {
        assert(!"failed to read file");
        err = RN_BAD_FILE_READ;
    }

    return err;
}
