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

std::string_view ReadUntilChar(char *&buf, char c)
{
    const char *start = buf;
    while (*buf) {
        if (*buf == c) break;
        buf++;
    }
    return { start, (size_t)(buf - start) };
}

std::string_view ParseTextNode(char *&buf)
{
    char *start = buf;
    char *lastChar = 0;

    bool done = false;
    bool discardTrailingWhitespace = false;
    while (*buf && !done) {
        switch (*buf) {
            case '\n': {
                char next = *(buf + 1);
                if (!next || next == '#') {
                    buf++;  // eat the newline
                    done = true;
                    discardTrailingWhitespace = true;
                }
                break;
            }
            case '[': {
                done = true;
                break;
            }
        }
        if (done) break;

        if (!IsWhitespace(*buf)) {
            lastChar = buf;
        }
        buf++;
    }

    if (lastChar) {
        if (discardTrailingWhitespace) {
            return { start, (size_t)((lastChar + 1) - start) };
        } else {
            return { start, (size_t)(buf - start) };
        }
    } else {
        return { start, 0 };
    }
}

Err ParseTagNode(char *&buf, DialogNode &node)
{
    Err err = RN_SUCCESS;
    char *start = buf;

    // Read dialog tag text
    if (!Expect(buf, '[')) {
        assert(!"expected tag text to open with '['");
        return RN_PARSE_ERROR;
    }

    std::string_view tagText = ReadUntilChar(buf, ']');
    if (!tagText.size()) {
        assert(!"expected tag text to be at least 1 character");
        return RN_PARSE_ERROR;
    }

    if (!Expect(buf, ']')) {
        assert(!"expected tag text to close with ']'");
        return RN_PARSE_ERROR;
    }

    // Read dialog tag key
    if (!Expect(buf, '(')) {
        assert(!"expected tag data to open with '('");
        return RN_PARSE_ERROR;
    }
    if (!Expect(buf, '#')) {
        assert(!"expected tag data to start with '#'");
        return RN_PARSE_ERROR;
    }

    std::string_view tagData = ReadUntilChar(buf, ')');
    if (!tagData.size()) {
        assert(!"expected tag data to be at least 1 character");
        return RN_PARSE_ERROR;
    }

    if (!Expect(buf, ')')) {
        assert(!"expected tag data to close with ')'");
        return RN_PARSE_ERROR;
    }

    if (tagData[0] == ' ') {
        char *hoverBuf = (char *)&tagData[1];
        if (!Expect(hoverBuf, '"')) {
            assert(!"expected hover tip tag data to open with '\"'");
            return RN_PARSE_ERROR;
        }

        std::string_view hoverTip = ReadUntilChar(hoverBuf, '"');
        if (!hoverTip.size()) {
            assert(!"expected hover tip tag data to be at least 1 character");
            return RN_PARSE_ERROR;
        }

        if (!Expect(hoverBuf, '"')) {
            assert(!"expected hover tip tag data to close with '\"'");
            return RN_PARSE_ERROR;
        }
        node.type = DIALOG_NODE_HOVER_TIP;
        node.text = tagText;
        node.data = hoverTip;
    } else {
        node.type = DIALOG_NODE_LINK;
        node.text = tagText;
        node.data = tagData;
    }

    return err;
}

Err ParseMessage(char *&buf, DialogNodeList &nodes)
{
    Err err = RN_SUCCESS;
    bool done = false;

    while (*buf && !done && !err) {
        switch (*buf) {
            case '#': {
                if (*(buf - 1) == '\n') {
                    done = true;
                }
                break;
            }
            case '[': {
                DialogNode node{};
                err = ParseTagNode(buf, node);
                if (!err) {
                    nodes.push_back(node);
                }
                break;
            }
            default: {
                std::string_view text = ParseTextNode(buf);
                if (text.size()) {
                    DialogNode &node = nodes.emplace_back();
                    node.type = DIALOG_NODE_TEXT;
                    node.text = text;
                }
                break;
            }
        }
    }

    return err;
}

Err ParseDialog(char *&buf)
{
    Err err = RN_SUCCESS;

    if (!Expect(buf, '#')) {
        assert(!"expected dialog key starting with '#'");
        return RN_PARSE_ERROR;
    }
    if (!Expect(buf, ' ')) {
        assert(!"expected ' ' after '#' in dialog key");
        return RN_PARSE_ERROR;
    }

    SkipWhitespace(buf);

    std::string_view key = ReadUntilChar(buf, '\n');
    if (!key.size()) {
        assert(!"expected dialog key to have length > 0");
        return RN_PARSE_ERROR;
    }

    if (dialog_library.dialog_idx_by_key.contains(key)) {
        assert(!"duplicate dialog key");
        return RN_PARSE_ERROR;
    }

    if (!Expect(buf, '\n')) {
        assert(!"expected newline after dialog key");
        return RN_PARSE_ERROR;
    }

    SkipWhitespace(buf);

    char *msgStart = buf;
    Dialog &dialog = dialog_library.Alloc(key);
    err = ParseMessage(buf, dialog.nodes);
    dialog.msg = { msgStart, (size_t)(buf - msgStart) };

    return err;
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
        return RN_PARSE_ERROR;
    }

    return err;
}

Err LoadDialogFile(const std::string &path)
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
