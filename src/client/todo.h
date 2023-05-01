#pragma once
#include "../common/shared_lib.h"

struct TodoItem {
    int textLen;
    const char *text;
    bool done;
};

struct TodoList {
    char *fileBuffer;
    int itemCount;
    TodoItem *items;
    bool dirty;

    ~TodoList(void);
    Err Save(const char *filename);
    Err Load(const char *filename);
    void Draw(Vector2 uiPosition);

private:
    bool TrySwapItems(int i, int j);
};