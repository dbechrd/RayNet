#include "spritesheet.h"

struct Token {
    int line;
    std::string key;
    std::string value;

    Token(int line, std::string key, std::string value)
        : line(line), key(key), value(value) {};
};

typedef std::vector<Token> TokenList;
struct TokenStream {
    std::string path;
    TokenList tokens;
    int next = 0;

    bool Eof(void) {
        return next >= tokens.size();
    }

    Token Peek(void) {
        return tokens[next];
    }

    Token Next(void) {
        return tokens[next++];
    }

    //bool Expect(std::string token) {
    //
    //}
};

#define REPORT_ERROR(error, tip, ...) \
{ \
    int column = lineStream.tellg(); \
    if (column < 0) column = lineBuf.size(); \
    printf("\n[%s:%d:%d] " error "\n", path.c_str(), line + 1, column + 1, __VA_ARGS__); \
    printf("%s\n", lineBuf.c_str()); \
    if (tip) { \
        printf("%*c", column, ' '); \
        printf("^ %s\n", tip); \
    } \
    putchar('\n'); \
}

Err ReadKVFile(std::string path, TokenStream &tokenStream)
{
    Err err = RN_SUCCESS;

    tokenStream.path = path;
    std::ifstream file{ path, std::ios::binary };

    int line = 0;
    std::string lineBuf{};
    while (std::getline(file, lineBuf)) {
        // Remove dumb Windows shit
        lineBuf.erase(std::remove(lineBuf.begin(), lineBuf.end(), '\r'), lineBuf.end());

        std::istringstream lineStream(lineBuf);

        std::string key;
        std::string value;

        lineStream >> key;
        lineStream >> std::ws;

        if (lineStream.peek() == '"') {
            lineStream.ignore(1);
            std::stringbuf buf{};
            lineStream.get(buf, '"');
            value = buf.str();
            int peek2 = lineStream.peek();
            if (peek2 == '"') {
                lineStream.ignore(1024, '\n');
            } else {
                REPORT_ERROR("Unmatched quotation mark", "Expected closing '\"'");
                err = RN_BAD_FILE_READ;
            }
        } else {
            lineStream >> value;
        }

        if (value == "[" || value == "{") {
            std::swap(key, value);
        }

        if (lineStream.good()) {
            // there's still stuff left after value.. we should not have stuff left
            REPORT_ERROR("Ignored extra characters in line", "After this");
            err = RN_BAD_FILE_READ;
        }

#if 0
        if (kv.find(key) == kv.end()) {
            kv[key] = value;
        } else {
            lineStream.seekg(lineStream.beg);
            REPORT_ERROR(
                "DUPLICATE_KEY: Key '%s' already exists with value '%s'",
                "Ignoring line",
                key.c_str(),
                value.c_str()
            );
        }
#endif
        tokenStream.tokens.push_back(Token{ line, key, value });
        line++;
    }

    return err;
}

Err ParseBoolean(TokenStream &tokenStream, bool &value)
{
    Err err = RN_SUCCESS;

    if (tokenStream.Eof()) {
        return RN_BAD_FILE_READ;
    }

    const Token &token = tokenStream.Next();
    if (token.value == "true") {
        value = true;
    } else if (token.value == "false") {
        value = false;
    } else {
        printf("[%s:%d] Unexpected value '%s = %s' in boolean field.\n",
            tokenStream.path.c_str(),
            token.line,
            token.key.c_str(),
            token.value.c_str()
        );
        err = RN_BAD_FILE_READ;
    }
    return err;
}

Err ParseInt(TokenStream &tokenStream, int &value)
{
    Err err = RN_SUCCESS;

    if (tokenStream.Eof()) {
        return RN_BAD_FILE_READ;
    }

    const Token &token = tokenStream.Next();
    try {
        value = std::stoi(token.value);
    } catch (std::exception& ex) {
        printf("[%s:%d] Unexpected value '%s = %s' in integer field. %s\n",
            tokenStream.path.c_str(),
            token.line,
            token.key.c_str(),
            token.value.c_str(),
            ex.what()
        );
        err = RN_BAD_FILE_READ;
    }
    return err;
}

Err ParseFloat(TokenStream &tokenStream, float &value)
{
    Err err = RN_SUCCESS;

    if (tokenStream.Eof()) {
        return RN_BAD_FILE_READ;
    }

    const Token &token = tokenStream.Next();
    try {
        value = std::stof(token.value);
    } catch (std::exception& ex) {
        printf("[%s:%d] Unexpected value '%s = %s' in float field. %s\n",
            tokenStream.path.c_str(),
            token.line,
            token.key.c_str(),
            token.value.c_str(),
            ex.what()
        );
        err = RN_BAD_FILE_READ;
    }
    return err;
}

Err ParseString(TokenStream &tokenStream, std::string &value)
{
    Err err = RN_SUCCESS;
    if (tokenStream.Eof()) {
        return RN_BAD_FILE_READ;
    }

    const Token &token = tokenStream.Next();
    if (token.value.size() > 0) {
        value = token.value;
    } else {
        printf("[%s:%d] Unexpected value '%s = %s' in string field.\n",
            tokenStream.path.c_str(),
            token.line,
            token.key.c_str(),
            token.value.c_str()
        );
        err = RN_BAD_FILE_READ;
    }
    return err;
}

Err ParseAnimation(TokenStream &tokenStream, Animation &animation)
{
    Err err = RN_SUCCESS;

    const Token &root = tokenStream.Peek();
    if (root.key != "{" || root.value != "animation") {
        // TODO: More info about line # etc
        printf("Expected an animation object\n");
        return RN_BAD_FILE_READ;
    }
    tokenStream.Next();

    while (!tokenStream.Eof()) {
        const Token &token = tokenStream.Peek();
        if (token.key == "name") {
            err = ParseString(tokenStream, animation.name);
        } else if (token.key == "frame_start") {
            err = ParseInt(tokenStream, animation.frameStart);
        } else if (token.key == "frame_count") {
            err = ParseInt(tokenStream, animation.frameCount);
        } else if (token.key == "frame_duration") {
            err = ParseFloat(tokenStream, animation.frameDuration);
        } else if (token.key == "loop") {
            err = ParseBoolean(tokenStream, animation.loop);
        } else if (token.key == "}") {
            tokenStream.Next();
            break;
        } else {
            printf("[%s:%d] Unexpected field '%s = %s' in animation object.\n",
                tokenStream.path.c_str(),
                token.line,
                token.key.c_str(),
                token.value.c_str()
            );
            err = RN_BAD_FILE_READ;
        }
        if (err) break;
    }
    return err;
}

Err ParseSpritesheet(TokenStream &tokenStream, Spritesheet &spritesheet)
{
    Err err = RN_SUCCESS;

    const Token &root = tokenStream.Peek();
    if (root.key != "{" || root.value != "spritesheet") {
        // TODO: More info about line # etc
        printf("Expected a spritesheet object\n");
        return RN_BAD_FILE_READ;
    }
    tokenStream.Next();

    while (!tokenStream.Eof()) {
        const Token &token = tokenStream.Peek();
        if (token.key == "version") {
            err = ParseInt(tokenStream, spritesheet.version);
        } else if (token.key == "texture_path") {
            std::string texturePath{};
            err = ParseString(tokenStream, texturePath);
            if (!err) {
                spritesheet.textureId = rnTextureCatalog.FindOrLoad(texturePath);
            }
        } else if (token.key == "frame_width") {
            err = ParseInt(tokenStream, spritesheet.frameWidth);
        } else if (token.key == "frame_height") {
            err = ParseInt(tokenStream, spritesheet.frameHeight);
        } else if (token.key == "[") {
            if (token.value == "animations") {
                tokenStream.Next();
                while (tokenStream.Peek().key != "]") {
                    Animation animation{};
                    err = ParseAnimation(tokenStream, animation);
                    if (err) break;
                    spritesheet.animations.push_back(animation);
                }
                if (!err) {
                    tokenStream.Next();
                }
            } else {
                printf("[%s:%d] Unexpected array '%s' in spritesheet object.\n",
                    tokenStream.path.c_str(),
                    token.line,
                    token.key.c_str()
                );
                err = RN_BAD_FILE_READ;
            }
        } else if (token.key == "}") {
            tokenStream.Next();
            break;
        } else {
            printf("[%s:%d] Unexpected field '%s = %s' in spritesheet object.\n",
                tokenStream.path.c_str(),
                token.line,
                token.key.c_str(),
                token.value.c_str()
            );
            err = RN_BAD_FILE_READ;
        }
        if (err) break;
    }
    return err;
}

Err Spritesheet::Load(std::string path)
{
    Err err = RN_SUCCESS;
    printf("Loading spritesheet %s...\n", path.c_str());

    TokenStream tokenStream{};
    err = ReadKVFile(path, tokenStream);
    if (err) return err;

    err = ParseSpritesheet(tokenStream, *this);
    if (!err) {
        filename = path;
    }
    return err;
}

////////////////////////////////////////////////////////////////////////////////

SpritesheetCatalog rnSpritesheetCatalog;

void SpritesheetCatalog::Init(void)
{
    size_t spritesheetId = entries.size();
    Spritesheet spritesheet{};
    spritesheet.textureId = rnTextureCatalog.FindOrLoad("PLACEHOLDER");
    spritesheet.version = 1;
    spritesheet.frameWidth = 32;
    spritesheet.frameHeight = 32;
    Animation animation{};
    animation.name = "PLACEHOLDER";
    animation.frameStart = 0;
    animation.frameCount = 1;
    animation.frameDuration = 1;
    animation.loop = true;
    spritesheet.animations.push_back(animation);
    entries.push_back(spritesheet);
    entriesByPath["PLACEHOLDER"] = spritesheetId;
}

void SpritesheetCatalog::Free(void)
{
    //for (auto &entry: entries) {
    //    nothin to do here
    //}
}

SpritesheetId SpritesheetCatalog::FindOrLoad(std::string path)
{
    SpritesheetId id = 0;
    const auto &entry = entriesByPath.find(path);
    if (entry != entriesByPath.end()) {
        id = entry->second;
    } else {
        Spritesheet spritesheet{};
        if (spritesheet.Load(path) == RN_SUCCESS) {
            id = entries.size();
            entries.emplace_back(spritesheet);
            entriesByPath[path] = id;
        }
    }
    return id;
}

const Spritesheet &SpritesheetCatalog::GetSpritesheet(SpritesheetId id)
{
    if (id >= 0 && id < entries.size()) {
        return entries[id];
    }
    return entries[0];
}

void SpritesheetCatalog::Unload(SpritesheetId id)
{
    // TODO: unload it eventually (ref count?)
}