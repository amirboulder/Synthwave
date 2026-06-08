#pragma once

enum class AssetType {
    Unknown,
    Mesh,
    Model,
    Texture2D,
    Audio,
    Shader,
    Material,
    Scene,
};

struct AssetMetadata {
    uint64_t                id = 0;
    std::string             name;
    std::string             cookedPath;
    std::string             sourcePath;
    uint64_t                contentHash = 0;
    uint64_t                sourceHash = 0; // contentHash of the source file
    uint64_t                importedAt = 0;
    std::vector<uint64_t>   dependencies;
    AssetType               type = AssetType::Unknown;
};


inline AssetType AssetTypeFromString(const std::string& str) {
    if (str == "Mesh")      return AssetType::Mesh;
    if (str == "Model")      return AssetType::Model;
    if (str == "Texture2D") return AssetType::Texture2D;
    if (str == "Audio")     return AssetType::Audio;
    if (str == "Shader")    return AssetType::Shader;
    if (str == "Material")  return AssetType::Material;
    if (str == "Scene")     return AssetType::Scene;
    return AssetType::Unknown;
}

inline std::string AssetTypeToString(AssetType type) {
    switch (type) {
    case AssetType::Mesh:      return "Mesh";
    case AssetType::Model:      return "Model";
    case AssetType::Texture2D: return "Texture2D";
    case AssetType::Audio:     return "Audio";
    case AssetType::Shader:    return "Shader";
    case AssetType::Material:  return "Material";
    case AssetType::Scene:     return "Scene";
    default:                   return "Unknown";
    }
}

namespace rj = rapidjson;

class Manifest {

private:

    fs::path path;
    uint32_t version = 1;
    std::unordered_map<uint64_t, AssetMetadata> assetsMap;
    mutable bool isDirty = false;

public:

   
    Manifest() = default;

    explicit Manifest(const fs::path & filePath)
        : path(filePath) {
    }

    bool Load() { return Load(path); }

    bool Load(const fs::path& filePath) {
        path = filePath;
        assetsMap.clear();
        isDirty = false;

        std::ifstream f(path);
        if (!f.is_open())
            return false;

        std::ostringstream ss;
        ss << f.rdbuf();
        std::string raw = ss.str();

        rj::Document doc;
        doc.Parse(raw.c_str());

        if (doc.HasParseError() || !doc.IsObject())
            return false;

        if (doc.HasMember("version") && doc["version"].IsInt())
            version = doc["version"].GetInt();

        const auto& assets = doc["assets"];
        if (!assets.IsObject())
            return true; // empty manifest is valid

        for (auto it = assets.MemberBegin(); it != assets.MemberEnd(); ++it) {
            const auto& v = it->value;
            const auto& key = it->name;
            AssetMetadata meta;

            meta.id = std::stoull(key.GetString());
            meta.name = SafeStr(v, "name");
            meta.cookedPath = SafeStr(v, "cookedPath");
            meta.sourcePath = SafeStr(v, "sourcePath");
            meta.contentHash = v.HasMember("contentHash") && v["contentHash"].IsUint64();
            meta.sourceHash =  v.HasMember("contentHash") && v["contentHash"].IsUint64();
            meta.type = AssetTypeFromString(SafeStr(v, "type"));
            meta.importedAt = v.HasMember("importedAt") && v["importedAt"].IsUint64()
                ? v["importedAt"].GetUint64() : 0;

            if (v.HasMember("dependencies") && v["dependencies"].IsArray()) {
                for (const auto& dep : v["dependencies"].GetArray())
                    if (dep.IsInt64())
                        meta.dependencies.push_back(dep.GetUint64());
            }

            assetsMap.emplace(meta.id, std::move(meta));
        }

        return true;
    }

    bool Save() const {

        rj::StringBuffer sb;
        rj::PrettyWriter<rj::StringBuffer> writer(sb);

        writer.StartObject();

        writer.Key("version"); writer.Int(version);
        writer.Key("assets");  writer.StartObject();

        for (const auto& [id, meta] : assetsMap) {
            writer.Key(std::to_string(id).c_str());
            writer.StartObject();

            writer.Key("cookedPath");  writer.String(meta.cookedPath.c_str());
            writer.Key("name");  writer.String(meta.name.c_str());
            writer.Key("sourcePath");  writer.String(meta.sourcePath.c_str());
            writer.Key("contentHash"); writer.Uint64(meta.contentHash);
            writer.Key("sourceHash");  writer.Uint64(meta.sourceHash);
            writer.Key("type");        writer.String(AssetTypeToString(meta.type).c_str());
            writer.Key("importedAt");  writer.Uint64(meta.importedAt);

            writer.Key("dependencies");
            writer.StartArray();
            for (const auto& dep : meta.dependencies)
                writer.Uint64(dep);
            writer.EndArray();

            writer.EndObject();
        }

        writer.EndObject(); // assets
        writer.EndObject(); // root

        // Ensure the directory exists before trying to write the file
        if (path.has_parent_path()) {
            !fs::create_directories(path.parent_path());
        }

        std::ofstream f(path);
        if (!f.is_open())
            return false;

        f << sb.GetString();
        isDirty = false;
        return true;
    }

    // ── Lookup ────────────────────────────────────────────────────────────────

    const AssetMetadata* Find(const uint64_t& id) const {
        auto it = assetsMap.find(id);
        return it != assetsMap.end() ? &it->second : nullptr;
    }

    AssetMetadata* Find(const uint64_t& id) {
        auto it = assetsMap.find(id);
        return it != assetsMap.end() ? &it->second : nullptr;
    }

    bool Contains(const uint64_t& id) const {
        return assetsMap.count(id) > 0;
    }

    uint64_t FindByName( const std::string& name) const {

        auto it = std::find_if(assetsMap.begin(), assetsMap.end(),
            [&name](const auto& pair) {
            const AssetMetadata& asset = pair.second;
            return asset.name == name;
        });

        return it != assetsMap.end() ? it->second.id : 0;
    }

    // ── Mutation ──────────────────────────────────────────────────────────────

    void Insert(const AssetMetadata& meta) {
        assetsMap.insert_or_assign(meta.id, meta);
        isDirty = true;
    }

    void Remove(const uint64_t& id) {
        assetsMap.erase(id);
        isDirty = true;
    }

    void Clear() {
        assetsMap.clear();
        isDirty = true;
    }

    // ── Iteration ─────────────────────────────────────────────────────────────

    void ForEach(const std::function<void(const AssetMetadata&)>& fn) const {
        for (const auto& [id, meta] : assetsMap)
            fn(meta);
    }

    // ── Queries ───────────────────────────────────────────────────────────────

    std::vector<const AssetMetadata*> FindByType(AssetType type) const {
        std::vector<const AssetMetadata*> result;
        for (const auto& [id, meta] : assetsMap)
            if (meta.type == type)
                result.push_back(&meta);
        return result;
    }

    // Groups IDs by contentHash — any group with size > 1 is a duplicate set
    std::unordered_map<uint64_t, std::vector<uint64_t>> GetDuplicates() const {
        std::unordered_map<uint64_t, std::vector<uint64_t>> byHash;
        for (const auto& [id, meta] : assetsMap)
            byHash[meta.contentHash].push_back(id);

        // strip entries with only one asset — not duplicates
        for (auto it = byHash.begin(); it != byHash.end(); ) {
            it = (it->second.size() < 2) ? byHash.erase(it) : ++it;
        }
        return byHash;
    }

    // Returns IDs whose sourceHash differs from the provided current hashes.
    // Caller computes: id → hash of the source file on disk right now.
    // Any ID in the manifest not present in currentHashes is also considered stale.
    std::vector<uint64_t> FindStale(
        const std::unordered_map<uint64_t, uint64_t>& currentHashes) const
    {
        std::vector<uint64_t> stale;
        for (const auto& [id, meta] : assetsMap) {
            auto it = currentHashes.find(id);
            if (it == currentHashes.end() || it->second != meta.sourceHash)
                stale.push_back(id);
        }
        return stale;
    }

    // ── Metadata ──────────────────────────────────────────────────────────────

    int                Version()  const { return version; }
    size_t             Count()    const { return assetsMap.size(); }
    bool               IsDirty()  const { return isDirty; }
    const fs::path Path()     const { return path; }

private:

    static const char* SafeStr(const rj::Value& v, const char* key) {
        auto it = v.FindMember(key);
        return (it != v.MemberEnd() && it->value.IsString())
            ? it->value.GetString() : "";
    }

   
};