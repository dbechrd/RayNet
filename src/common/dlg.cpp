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

std::string_view ReadUntilLineStartsWith(char *&buf, char c)
{
    char *msgStart = buf;
    char *msgEnd = 0;

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
            msgEnd = buf;
        }
        buf++;
    }

    if (msgEnd) {
        assert(msgEnd >= msgStart);
        return { msgStart, (size_t)(msgEnd - msgStart) };
    } else {
        return { msgStart, 0 };
    }
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

    return RN_SUCCESS;
}

Err ParseDialogFile(char *buf)
{
    Err err = RN_SUCCESS;

    while (*buf) {
        if (*buf == '#') {
            ParseDialog(buf);
        } else {
            ReadUntilLineStartsWith(buf, '#');
        }
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