struct KVWriter {
    int indent = 0;
    std::ofstream file;

    KVWriter(std::string path)
        : file{ path } //, std::ifstream::binary }
    {
    }

    void WriteIndent(void) {
        for (int i = 0; i < indent; i++) {
            file << "    ";
        }
    }

    void ObjStart(std::string key) {
        WriteIndent();
        file << key << " {\n";
        ++indent;
    }
    void ObjEnd(void) {
        --indent;
        WriteIndent();
        file << "}\n";
    }

    void ArrayStart(std::string key) {
        WriteIndent();
        file << key << " [\n";
        ++indent;
    }
    void ArrayEnd(void) {
        --indent;
        WriteIndent();
        file << "]\n";
    }

    template <typename T>
    void WriteValue(T value) {
        constexpr bool str_type =
            std::is_same_v<T, const char *> ||
            std::is_same_v<T, std::string>;
        if (str_type) file << "\"";
        file << value;
        if (str_type) file << "\"";
    }

    template <typename T>
    void Write(std::string key, T value) {
        WriteIndent();
        file << key << std::string(std::max(1, 16 - (int)key.size()), ' ');
        WriteValue(value);
        file << "\n";
    }

    template <typename T>
    void WriteArray(std::string key, std::vector<T> values, std::string delim = ",", int rowWidth = 1) {
        ArrayStart(key);
        WriteIndent();
        int valuesLen = (int)values.size();
        for (int i = 0; i < valuesLen; i++) {
            WriteValue(values[i]);
            file << delim;
            const bool last = i == valuesLen - 1;
            if (last || (i + 1) % rowWidth == 0) {
                file << '\n';
                if (!last) {
                    WriteIndent();
                }
            }
        }
        ArrayEnd();
    }

    void WriteArrayHex(std::string key, std::vector<uint8_t> values, int rowWidth = 32) {
        ArrayStart(key);
        WriteIndent();
        int valuesLen = (int)values.size();
        const char *hexLut = "0123456789ABCDEF";
        for (int i = 0; i < valuesLen; i++) {
            int high = values[i] / 16;
            int low = values[i] % 16;
            file << hexLut[high] << hexLut[low];
            const bool last = i == valuesLen - 1;
            if (last || (i + 1) % rowWidth == 0) {
                file << '\n';
                if (!last) {
                    WriteIndent();
                }
            }
        }
        ArrayEnd();
    }
};

Err Tilemap::SaveKV(std::string path)
{
    Err err = RN_SUCCESS;

    if (FileExists(path.c_str())) {
        err = MakeBackup(path.c_str());
        if (err) return err;
    }

    do {
        // this isn't technically necessary, but it seems really bizarre to save
        // a tilemap that points to the placeholder texture. the user should
        // really pick a valid tileset image before saving the map
        if (!textureId) {
            err = RN_INVALID_PATH; break;
        }
        if (!width || !height) {
            err = RN_INVALID_SIZE; break;
        }
        if (tiles.size() != (size_t)width * height) {
            err = RN_INVALID_SIZE; break;
        }

        KVWriter writer{ path };
        if (writer.file.fail()) {
            err = RN_BAD_FILE_WRITE; break;
        }

        writer.ObjStart("map");
            writer.Write("magic", MAGIC);
            writer.Write("version", VERSION);
            writer.ObjStart("tileset");
                const std::string texturePath = rnStringCatalog.GetString(textureId);
                writer.Write("texture_path_len", texturePath.size());
                writer.Write("texture_path", texturePath);
                writer.Write("tile_def_count", tileDefs.size());
                writer.ArrayStart("tile_defs");
                for (const TileDef &tileDef : tileDefs) {
                    writer.ObjStart("tile_def");
                    writer.Write("x", tileDef.x);
                    writer.Write("y", tileDef.y);
                    writer.Write("material_id", tileDef.materialId);
                    writer.Write("collide", tileDef.collide);
                    writer.ObjEnd();
                }
                writer.ArrayEnd();
            writer.ObjEnd();

            writer.Write("width", width);
            writer.Write("height", height);
            writer.WriteArrayHex("tiles", tiles, width);

            writer.Write("path_node_count", pathNodes.size());
            writer.ArrayStart("path_nodes");
            for (const AiPathNode &aiPathNode : pathNodes) {
                writer.ObjStart("path_node");
                writer.Write("x", aiPathNode.pos.x);
                writer.Write("y", aiPathNode.pos.y);
                writer.Write("wait_for", aiPathNode.waitFor);
                writer.ObjEnd();
            }
            writer.ArrayEnd();

            writer.Write("path_node_index_count", pathNodeIndices.size());
            writer.WriteArray("path_node_indices", pathNodeIndices, ", ", 32);

            writer.Write("path_count", paths.size());
            writer.ArrayStart("paths");
            for (const AiPath &aiPath : paths) {
                writer.ObjStart("path");
                writer.Write("offset", aiPath.pathNodeIndexOffset);
                writer.Write("length", aiPath.pathNodeIndexCount);
                writer.ObjEnd();
            }
            writer.ArrayEnd();

            writer.Write("warp_count", warps.size());
            writer.ArrayStart("warps");
            for (const Warp &warp : warps) {
                writer.ObjStart("warp");
                    writer.ObjStart("collider");
                        writer.Write("x", warp.collider.x);
                        writer.Write("y", warp.collider.y);
                        writer.Write("w", warp.collider.width);
                        writer.Write("h", warp.collider.height);
                    writer.ObjEnd();

                    writer.ObjStart("dest");
                        writer.Write("x", warp.destPos.x);
                        writer.Write("y", warp.destPos.y);
                    writer.ObjEnd();

                    writer.Write("dest_map_len", warp.destMap.size());
                    writer.Write("dest_map", warp.destMap);

                    writer.Write("template_map_len", warp.templateMap.size());
                    writer.Write("template_map", warp.templateMap);

                    writer.Write("template_tileset_len", warp.templateTileset.size());
                    writer.Write("template_tileset", warp.templateTileset);
                writer.ObjEnd();
            }
            writer.ArrayEnd();
        writer.ObjEnd();
    } while (0);

    if (!err) {
        //filename = path;
    }
    return RN_SUCCESS;
}