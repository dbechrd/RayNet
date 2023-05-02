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
    dirty = false;
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

    dirty = false;
    return RN_SUCCESS;
}

bool TodoList::TrySwapItems(int i, int j)
{
    if (i == j || i < 0 || j < 0 || i >= itemCount || j >= itemCount) return false;

    TodoItem tmp = items[i];
    items[i] = items[j];
    items[j] = tmp;
    dirty = true;
    return true;
}

void TodoList::Draw(Vector2 position)
{
    static int textDraggingIndex = -1;

    UIStyle uiStyle{};
    uiStyle.margin = { 4, 0, 0, 4 };
    UI ui{ position, uiStyle };

    UIState loadButton = ui.Button("Load");
    if (loadButton.clicked) {
        Load(TODO_LIST_PATH);
    }

    UIState saveButton;
    if (dirty) {
        saveButton = ui.Button("Save*", ColorBrightness(ORANGE, -0.2f));
    } else {
        saveButton = ui.Button("Save");
    }
    if (saveButton.clicked) {
        Save(TODO_LIST_PATH);
    }

    ui.Newline();

    //DrawTextShadowEx(fntHackBold20, TextFormat("dragIdx: %d", textDraggingIndex), Vector2Add(uiPosition, uiCursor), fntHackBold20.baseSize, WHITE);
    //uiCursor.y += fntHackBold20.baseSize + 4;

    const Vector2 margin { 8, 6 };
    for (int i = 0; i < itemCount; i++) {
        if (items[i].done) {
            UIState doneButton = ui.Button("Done", DARKGREEN);
            if (doneButton.clicked) {
                items[i].done = false;
                dirty = true;
            }
        } else {
            UIState todoButton = ui.Button("Todo", MAROON);
            if (todoButton.clicked) {
                items[i].done = true;
                dirty = true;
            }
        }

        UIState dragButton = ui.Button(" ", i == textDraggingIndex ? ORANGE : DARKGRAY);
        if (dragButton.pressed) {
            textDraggingIndex = i;
        } else if (textDraggingIndex == i && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            textDraggingIndex = -1;
        }

        bool mouseAboveMe = GetMouseY() < ui.CursorScreen().y;
        if (textDraggingIndex >= i && mouseAboveMe) {
            if (TrySwapItems(textDraggingIndex, textDraggingIndex - 1)) {
                textDraggingIndex--;
            }
        } else if (textDraggingIndex < i && !mouseAboveMe) {
            if (TrySwapItems(textDraggingIndex, textDraggingIndex + 1)) {
                textDraggingIndex++;
            }
        }
#if 0
        // Ugly up and down buttons, we have drag now wooo
        if (i > 0) {
            UIState moveUpButton = UIButton(fntHackBold20, BLUE, "^", uiPosition, uiCursor);
            if (moveUpButton.clicked) {
                TrySwapItems(i, i - 1);
            }
        } else {
            UIState moveUpButtonFake = UIButton(fntHackBold20, DARKGRAY, " ", uiPosition, uiCursor);
        }
        uiCursor.x += margin.x;

        if (i < itemCount - 1) {
            UIState moveDownButton = UIButton(fntHackBold20, BLUE, "v", uiPosition, uiCursor);
            if (moveDownButton.clicked) {
                TrySwapItems(i, i + 1);
            }
        } else {
            UIState moveUpButtonFake = UIButton(fntHackBold20, DARKGRAY, " ", uiPosition, uiCursor);
        }
        uiCursor.x += margin.x;
#endif

        Vector2 textPos = Vector2Add(ui.CursorScreen(), uiStyle.margin.TopLeft());
        DrawTextShadowEx(fntHackBold20, items[i].text, textPos, WHITE);
        ui.Newline();
    }
}