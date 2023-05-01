#include "todo.h"
#include "../common/ui/ui.h"

TodoList::~TodoList(void)
{
    free(items);
    UnloadFileText(fileBuffer);
}

Err TodoList::Save(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (!file) {
        return RN_BAD_FILE_WRITE;
    }

    for (int i = 0; i < itemCount; i++) {
        if (items[i].done) {
            fwrite("[x] ", 1, 4, file);
        } else {
            fwrite("[ ] ", 1, 4, file);
        }
        fwrite(items[i].text, 1, items[i].textLen, file);
        fputc('\n', file);
    }

    fclose(file);
    return RN_SUCCESS;
}

int CountLines(char *fileBuffer)
{
    int lineCount = 0;
    char *c = fileBuffer;
    bool lineHasText = false;
    while (*c) {
        switch (*c) {
            case '\n': {
                if (lineHasText) {
                    lineCount++;
                    lineHasText = false;
                }
                break;
            }
            default: {
                lineHasText = !isspace(*c);
                break;
            }
        }
        c++;
    }

    // Add 1 more if no trailing newline at end of file
    lineCount += *(c - 1) != '\n';

    return lineCount;
}

Err TodoList::Load(const char *filename)
{
    fileBuffer = LoadFileText(filename);
    if (!fileBuffer) {
        return RN_BAD_FILE_READ;
    }

    itemCount = CountLines(fileBuffer);
    if (!itemCount) {
        return RN_SUCCESS;
    }

    char *buf = fileBuffer;
    items = (TodoItem *)calloc(itemCount, sizeof(*items));
    for (int i = 0; i < itemCount;) {
        // Look for '[x] ' at beginning of line
        char *check = buf;
        if (!strncmp(check, "[x] ", 4) && *(check + 4)) {
            items[i].done = true;
            buf += 4;
        } else if (!strncmp(check, "[ ] ", 4) && *(check + 4)) {
            buf += 4;
        }

        // Start a new item
        if (!isspace(*buf)) {
            items[i].text = buf;
            // Eat all chars until newline
            while (*buf && *buf != '\r' && *buf != '\n') {
                items[i].textLen++;
                buf++;
            }
            i++;
        }

        // Malformed line caused premature EOF, time to leave
        if (!*buf) {
            break;
        }

        // Replace newline with nil terminator
        *buf = 0;
        buf++;

        // Eat all newlines/whitespace between items
        while (*buf && isspace(*buf)) {
            buf++;
        }
    }

    return RN_SUCCESS;
}

void TodoList::Draw(Vector2 uiPosition, Vector2 &uiCursor)
{
    for (int i = 0; i < itemCount; i++) {
        uiCursor.x = 0;
        if (items[i].done) {
            UIState restartButton = UIButton(fntHackBold20, DARKGREEN, "Done", uiPosition, uiCursor);
            if (restartButton.clicked) {
                items[i].done = false;
            }
        } else {
            UIState doneButton = UIButton(fntHackBold20, MAROON, "Todo", uiPosition, uiCursor);
            if (doneButton.clicked) {
                items[i].done = true;
            }
        }
        uiCursor.x += 8;
        Vector2 textPos = Vector2Add(uiPosition, uiCursor);
        DrawTextShadowEx(fntHackBold20, items[i].text, textPos, fntHackBold20.baseSize, WHITE);
        uiCursor.y += fntHackBold20.baseSize + 4;
    }
}