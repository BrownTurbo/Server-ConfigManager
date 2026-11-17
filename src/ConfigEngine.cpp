#include "ConfigEngine.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

const std::unordered_map<std::string, std::string> kLegacyToOmpMap = {
    {"hostname",          "name"},
    {"maxplayers",        "max_players"},
    {"port",              "network.port"},
    {"bind",              "network.bind"},
    {"password",          "password"},
    {"rcon_password",     "rcon.password"},
    {"rcon",              "rcon.allow"},
    {"lanmode",           "network.lan_mode"},
    {"announce",          "announce"},
    {"query",             "enable_query"},
    {"weburl",            "website"},
    {"gamemodetext",      "game.mode"},
    {"mapname",           "game.map"},
    {"language",          "language"},
    {"maxnpc",            "max_bots"},
    {"onfoot_rate",       "network.on_foot_sync_rate"},
    {"incar_rate",        "network.in_vehicle_sync_rate"},
    {"weapon_rate",       "network.aiming_sync_rate"},
    {"stream_distance",   "network.stream_radius"},
    {"stream_rate",       "network.stream_rate"},
    {"sleep",             "sleep"},
    {"mtu",               "network.mtu"},
    {"useartwork",        "artwork.enable"},
    {"artwork_path",      "artwork.models_path"},
    {"logtimeformat",     "logging.timestamp_format"},
    {"logqueries",        "logging.log_queries"},
    {"chatlogging",       "logging.log_chat"},
    {"db_logging",        "logging.log_sqlite"},
    {"db_log_queries",    "logging.log_sqlite_queries"},
    {"conncookies",       "network.use_connection_cookies"},
    {"cookielogging",     "logging.log_connection_cookies"},
    {"connseedtime",      "network.cookie_reseed_time"},
    {"minconnectiontime", "network.minimum_connection_time"},
    {"messageslimit",     "network.messages_limit"},
    {"acksheight",        "network.acks_limit"},
    {"messageholelimit",  "network.message_hole_limit"},
    {"playertimeout",     "network.player_timeout"},
    {"plugins",           "pawn.legacy_plugins"},
    {"filterscripts",     "pawn.filterscripts"},
    {"gamemodes",         "pawn.main_scripts"}
};

const std::unordered_map<std::string, std::string> kOmpToLegacyMap = []() {
    std::unordered_map<std::string, std::string> rev;
    for (const auto& [legacy, omp] : kLegacyToOmpMap) {
        rev[omp] = legacy;
    }
    return rev;
}();

std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> SplitPath(const std::string& str) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, '.')) {
        if (!item.empty()) {
            tokens.push_back(item);
        }
    }
    return tokens;
}

} // anonymous namespace

// ============================================================================
// JsonConfigDocument Implementation
// ============================================================================

JsonConfigDocument::JsonConfigDocument(std::string filepath)
    : filePath_(std::move(filepath)) {
}

std::string JsonConfigDocument::ResolveCanonicalPath(const std::string& key) const {
    auto it = kLegacyToOmpMap.find(key);
    return (it != kLegacyToOmpMap.end()) ? it->second : key;
}

const nlohmann::json* JsonConfigDocument::ResolveNode(const std::string& key) const {
    if (rootJson_.contains(key)) {
        return &rootJson_[key];
    }

    std::string effectiveKey = ResolveCanonicalPath(key);
    auto parts = SplitPath(effectiveKey);
    const nlohmann::json* cursor = &rootJson_;

    for (const auto& part : parts) {
        if (cursor->is_object() && cursor->contains(part)) {
            cursor = &((*cursor)[part]);
        } else if (cursor->is_array()) {
            try {
                size_t idx = std::stoul(part);
                if (idx < cursor->size()) {
                    cursor = &((*cursor)[idx]);
                } else {
                    return nullptr;
                }
            } catch (...) {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }
    return cursor;
}

nlohmann::json* JsonConfigDocument::ResolveOrCreateNode(const std::string& key) {
    std::string effectiveKey = ResolveCanonicalPath(key);
    auto parts = SplitPath(effectiveKey);
    if (parts.empty()) return nullptr;

    nlohmann::json* cursor = &rootJson_;
    for (size_t i = 0; i < parts.size(); ++i) {
        const std::string& part = parts[i];
        if (i + 1 == parts.size()) {
            if (!cursor->is_object()) {
                *cursor = nlohmann::json::object();
            }
            return &((*cursor)[part]);
        } else {
            if (!cursor->is_object()) {
                *cursor = nlohmann::json::object();
            }
            if (!cursor->contains(part) || !(*cursor)[part].is_object()) {
                (*cursor)[part] = nlohmann::json::object();
            }
            cursor = &((*cursor)[part]);
        }
    }
    return cursor;
}

bool JsonConfigDocument::Load(const std::string& filepath) {
    filePath_ = filepath;
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    try {
        rootJson_ = nlohmann::json::parse(file, nullptr, true, true);
        dirty_ = false;
        return rootJson_.is_object();
    } catch (...) {
        return false;
    }
}

bool JsonConfigDocument::Save(const std::string& filepath) {
    const std::string& target = !filepath.empty() ? filepath : filePath_;
    try {
        std::string serialized = rootJson_.dump(4) + "\n";
        if (ConfigManager::AtomicWriteFile(target, serialized, nullptr)) {
            dirty_ = false;
            filePath_ = target;
            return true;
        }
    } catch (...) {
    }
    return false;
}

bool JsonConfigDocument::HasKey(const std::string& key) const {
    return ResolveNode(key) != nullptr;
}

bool JsonConfigDocument::RemoveKey(const std::string& key) {
    bool removed = false;
    if (rootJson_.contains(key)) {
        rootJson_.erase(key);
        removed = true;
    }

    std::string canonical = ResolveCanonicalPath(key);
    auto parts = SplitPath(canonical);
    if (!parts.empty()) {
        nlohmann::json* cursor = &rootJson_;
        for (size_t i = 0; i + 1 < parts.size(); ++i) {
            if (cursor->is_object() && cursor->contains(parts[i])) {
                cursor = &((*cursor)[parts[i]]);
            } else {
                cursor = nullptr;
                break;
            }
        }
        if (cursor && cursor->is_object() && cursor->contains(parts.back())) {
            cursor->erase(parts.back());
            removed = true;
        }
    }

    if (removed) dirty_ = true;
    return removed;
}

std::optional<std::string> JsonConfigDocument::GetString(const std::string& key) const {
    const nlohmann::json* node = ResolveNode(key);
    if (!node || node->is_null()) return std::nullopt;

    if (node->is_string()) return node->get<std::string>();
    if (node->is_number_integer()) return std::to_string(node->get<int64_t>());
    if (node->is_number_float()) return std::to_string(node->get<double>());
    if (node->is_boolean()) return node->get<bool>() ? "true" : "false";
    return node->dump();
}

std::optional<int> JsonConfigDocument::GetInt(const std::string& key) const {
    const nlohmann::json* node = ResolveNode(key);
    if (!node || node->is_null()) return std::nullopt;

    if (node->is_number_integer()) return node->get<int>();
    if (node->is_number_float()) return static_cast<int>(node->get<double>());
    if (node->is_boolean()) return node->get<bool>() ? 1 : 0;
    if (node->is_string()) {
        try {
            return std::stoi(node->get<std::string>());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<int64_t> JsonConfigDocument::GetInt64(const std::string& key) const {
    const nlohmann::json* node = ResolveNode(key);
    if (!node || node->is_null()) return std::nullopt;

    if (node->is_number_integer()) return node->get<int64_t>();
    if (node->is_number_float()) return static_cast<int64_t>(node->get<double>());
    if (node->is_boolean()) return node->get<bool>() ? 1 : 0;
    if (node->is_string()) {
        try {
            return std::stoll(node->get<std::string>());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<float> JsonConfigDocument::GetFloat(const std::string& key) const {
    const nlohmann::json* node = ResolveNode(key);
    if (!node || node->is_null()) return std::nullopt;

    if (node->is_number()) return node->get<float>();
    if (node->is_string()) {
        try {
            return std::stof(node->get<std::string>());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<bool> JsonConfigDocument::GetBool(const std::string& key) const {
    const nlohmann::json* node = ResolveNode(key);
    if (!node || node->is_null()) return std::nullopt;

    if (node->is_boolean()) return node->get<bool>();
    if (node->is_number_integer()) return node->get<int>() != 0;
    if (node->is_string()) {
        std::string s = node->get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (s == "1" || s == "true" || s == "yes" || s == "on") return true;
        if (s == "0" || s == "false" || s == "no" || s == "off") return false;
    }
    return std::nullopt;
}

bool JsonConfigDocument::SetString(const std::string& key, const std::string& value) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node) return false;
    *node = value;
    dirty_ = true;
    return true;
}

bool JsonConfigDocument::SetInt(const std::string& key, int value) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node) return false;
    *node = value;
    dirty_ = true;
    return true;
}

bool JsonConfigDocument::SetInt64(const std::string& key, int64_t value) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node) return false;
    *node = value;
    dirty_ = true;
    return true;
}

bool JsonConfigDocument::SetFloat(const std::string& key, float value) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node) return false;
    *node = value;
    dirty_ = true;
    return true;
}

bool JsonConfigDocument::SetBool(const std::string& key, bool value) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node) return false;
    *node = value;
    dirty_ = true;
    return true;
}

int JsonConfigDocument::GetArrayLength(const std::string& key) const {
    const nlohmann::json* node = ResolveNode(key);
    if (!node || !node->is_array()) return -1;
    return static_cast<int>(node->size());
}

std::optional<std::string> JsonConfigDocument::GetArrayString(const std::string& key, size_t index) const {
    const nlohmann::json* node = ResolveNode(key);
    if (!node || !node->is_array() || index >= node->size()) return std::nullopt;

    const auto& elem = (*node)[index];
    if (elem.is_string()) return elem.get<std::string>();
    return elem.dump();
}

bool JsonConfigDocument::SetArrayString(const std::string& key, size_t index, const std::string& value) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node || !node->is_array() || index >= node->size()) return false;
    (*node)[index] = value;
    dirty_ = true;
    return true;
}

bool JsonConfigDocument::AppendArrayString(const std::string& key, const std::string& value) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node) return false;
    if (!node->is_array()) *node = nlohmann::json::array();
    node->push_back(value);
    dirty_ = true;
    return true;
}

bool JsonConfigDocument::RemoveArrayElement(const std::string& key, size_t index) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node || !node->is_array() || index >= node->size()) return false;
    node->erase(node->begin() + static_cast<ptrdiff_t>(index));
    dirty_ = true;
    return true;
}

bool JsonConfigDocument::ClearArray(const std::string& key) {
    nlohmann::json* node = ResolveOrCreateNode(key);
    if (!node) return false;
    *node = nlohmann::json::array();
    dirty_ = true;
    return true;
}

// ============================================================================
// CfgConfigDocument Implementation (server.cfg Engine)
// ============================================================================

CfgConfigDocument::CfgConfigDocument(std::string filepath)
    : filePath_(std::move(filepath)) {
}

size_t CfgConfigDocument::FindCommentStart(const std::string& line) {
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (inQuote) continue;

        if (c == '#') {
            if (i == 0 || std::isspace(static_cast<unsigned char>(line[i - 1]))) {
                return i;
            }
        }
        if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            // Guard against URL schemes such as "https://open.mp"
            if (i == 0 || std::isspace(static_cast<unsigned char>(line[i - 1]))) {
                return i;
            }
        }
    }
    return std::string::npos;
}

std::string CfgConfigDocument::ResolveLegacyKey(const std::string& key) const {
    auto it = kOmpToLegacyMap.find(key);
    return (it != kOmpToLegacyMap.end()) ? it->second : key;
}

bool CfgConfigDocument::Load(const std::string& filepath) {
    filePath_ = filepath;
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    rawLines_.clear();
    flatValues_.clear();
    gamemodes_.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        rawLines_.push_back(line);

        std::string clean = line;
        size_t commentPos = FindCommentStart(clean);
        if (commentPos != std::string::npos) {
            clean.erase(commentPos);
        }

        clean = Trim(clean);
        if (clean.empty()) continue;

        size_t spacePos = clean.find_first_of(" \t");
        if (spacePos == std::string::npos) {
            flatValues_[clean] = "";
            continue;
        }

        std::string key = Trim(clean.substr(0, spacePos));
        std::string val = Trim(clean.substr(spacePos + 1));

        // Strip enclosing quotes if present (e.g. discord_token "token_here")
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }

        if (key.rfind("gamemode", 0) == 0 && key != "gamemodetext") {
            gamemodes_.push_back(val);
            flatValues_[key] = val;
        } else {
            flatValues_[key] = val;
        }
    }

    dirty_ = false;
    return true;
}

bool CfgConfigDocument::Save(const std::string& filepath) {
    const std::string& target = !filepath.empty() ? filepath : filePath_;

    std::unordered_map<std::string, bool> writtenKeys;
    std::vector<std::string> outputLines;
    bool gamemodesWritten = false;

    for (const auto& origLine : rawLines_) {
        std::string clean = origLine;
        size_t commentPos = FindCommentStart(clean);
        std::string commentSuffix;
        if (commentPos != std::string::npos) {
            commentSuffix = " " + Trim(clean.substr(commentPos));
            clean.erase(commentPos);
        }

        clean = Trim(clean);
        if (clean.empty()) {
            outputLines.push_back(origLine);
            continue;
        }

        size_t spacePos = clean.find_first_of(" \t");
        std::string key = (spacePos == std::string::npos) ? clean : Trim(clean.substr(0, spacePos));

        // Group gamemode lines together
        if (key.rfind("gamemode", 0) == 0 && key != "gamemodetext") {
            if (!gamemodesWritten) {
                for (size_t i = 0; i < gamemodes_.size(); ++i) {
                    outputLines.push_back("gamemode" + std::to_string(i) + " " + gamemodes_[i]);
                }
                gamemodesWritten = true;
            }
            continue;
        }

        auto it = flatValues_.find(key);
        if (it == flatValues_.end()) {
            // Directive removed
            continue;
        }

        if (!writtenKeys[key]) {
            if (it->second.empty()) {
                outputLines.push_back(key + commentSuffix);
            } else {
                outputLines.push_back(key + " " + it->second + commentSuffix);
            }
            writtenKeys[key] = true;
        }
    }

    // Append newly created keys
    for (const auto& [key, val] : flatValues_) {
        if (!writtenKeys[key]) {
            if (val.empty()) {
                outputLines.push_back(key);
            } else {
                outputLines.push_back(key + " " + val);
            }
        }
    }

    if (!gamemodesWritten && !gamemodes_.empty()) {
        for (size_t i = 0; i < gamemodes_.size(); ++i) {
            outputLines.push_back("gamemode" + std::to_string(i) + " " + gamemodes_[i]);
        }
    }

    rawLines_ = outputLines;

    std::ostringstream finalStream;
    for (const auto& l : outputLines) {
        finalStream << l << '\n';
    }

    if (ConfigManager::AtomicWriteFile(target, finalStream.str(), nullptr)) {
        dirty_ = false;
        filePath_ = target;
        return true;
    }
    return false;
}

bool CfgConfigDocument::HasKey(const std::string& key) const {
    if (flatValues_.find(key) != flatValues_.end()) return true;
    std::string legacy = ResolveLegacyKey(key);
    return flatValues_.find(legacy) != flatValues_.end();
}

bool CfgConfigDocument::RemoveKey(const std::string& key) {
    std::string legacy = ResolveLegacyKey(key);
    bool removed = false;
    if (flatValues_.erase(key) > 0) removed = true;
    if (flatValues_.erase(legacy) > 0) removed = true;
    if (removed) dirty_ = true;
    return removed;
}

std::optional<std::string> CfgConfigDocument::GetString(const std::string& key) const {
    auto it = flatValues_.find(key);
    if (it != flatValues_.end()) return it->second;

    std::string legacy = ResolveLegacyKey(key);
    it = flatValues_.find(legacy);
    if (it != flatValues_.end()) return it->second;

    return std::nullopt;
}

std::optional<int> CfgConfigDocument::GetInt(const std::string& key) const {
    auto val = GetString(key);
    if (!val) return std::nullopt;
    try {
        return std::stoi(*val);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int64_t> CfgConfigDocument::GetInt64(const std::string& key) const {
    auto val = GetString(key);
    if (!val) return std::nullopt;
    try {
        return std::stoll(*val);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<float> CfgConfigDocument::GetFloat(const std::string& key) const {
    auto val = GetString(key);
    if (!val) return std::nullopt;
    try {
        return std::stof(*val);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> CfgConfigDocument::GetBool(const std::string& key) const {
    auto val = GetString(key);
    if (!val) return std::nullopt;
    std::string s = *val;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (s == "1" || s == "true" || s == "yes" || s == "on") return true;
    if (s == "0" || s == "false" || s == "no" || s == "off") return false;
    return std::nullopt;
}

bool CfgConfigDocument::SetString(const std::string& key, const std::string& value) {
    std::string legacy = ResolveLegacyKey(key);
    flatValues_[legacy] = value;
    dirty_ = true;
    return true;
}

bool CfgConfigDocument::SetInt(const std::string& key, int value) {
    return SetString(key, std::to_string(value));
}

bool CfgConfigDocument::SetInt64(const std::string& key, int64_t value) {
    return SetString(key, std::to_string(value));
}

bool CfgConfigDocument::SetFloat(const std::string& key, float value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << value;
    std::string s = oss.str();
    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
    if (s.back() == '.') s.push_back('0');
    return SetString(key, s);
}

bool CfgConfigDocument::SetBool(const std::string& key, bool value) {
    return SetString(key, value ? "1" : "0");
}

int CfgConfigDocument::GetArrayLength(const std::string& key) const {
    std::string legacy = ResolveLegacyKey(key);
    if (legacy == "gamemodes") {
        return static_cast<int>(gamemodes_.size());
    }
    return GetCfgTokenCount(legacy);
}

std::optional<std::string> CfgConfigDocument::GetArrayString(const std::string& key, size_t index) const {
    std::string legacy = ResolveLegacyKey(key);
    if (legacy == "gamemodes") {
        return GetCfgGamemode(index);
    }
    return GetCfgToken(legacy, index);
}

bool CfgConfigDocument::SetArrayString(const std::string&, size_t, const std::string&) {
    // server.cfg does not have structured JSON arrays; mutation is disabled to prevent corruption
    return false;
}

bool CfgConfigDocument::AppendArrayString(const std::string&, const std::string&) {
    // server.cfg does not have structured JSON arrays; mutation is disabled to prevent corruption
    return false;
}

bool CfgConfigDocument::RemoveArrayElement(const std::string&, size_t) {
    // server.cfg does not have structured JSON arrays; mutation is disabled to prevent corruption
    return false;
}

bool CfgConfigDocument::ClearArray(const std::string&) {
    // server.cfg does not have structured JSON arrays; mutation is disabled to prevent corruption
    return false;
}

int CfgConfigDocument::GetCfgTokenCount(const std::string& key) const {
    auto val = GetString(key);
    if (!val) return -1;

    std::istringstream iss(*val);
    std::string item;
    int count = 0;
    while (iss >> item) ++count;
    return count;
}

std::optional<std::string> CfgConfigDocument::GetCfgToken(const std::string& key, size_t index) const {
    auto val = GetString(key);
    if (!val) return std::nullopt;

    std::istringstream iss(*val);
    std::string item;
    size_t cur = 0;
    while (iss >> item) {
        if (cur == index) return item;
        ++cur;
    }
    return std::nullopt;
}

int CfgConfigDocument::GetCfgGamemodeCount() const {
    return static_cast<int>(gamemodes_.size());
}

std::optional<std::string> CfgConfigDocument::GetCfgGamemode(size_t index) const {
    if (index >= gamemodes_.size()) return std::nullopt;
    const std::string& entry = gamemodes_[index];
    size_t space = entry.find_first_of(" \t");
    return (space == std::string::npos) ? entry : entry.substr(0, space);
}

// ============================================================================
// ConfigManager Implementation
// ============================================================================

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::SetLogger(LoggerFunc logger) {
    std::unique_lock lock(mutex_);
    logger_ = std::move(logger);
}

void ConfigManager::Log(const std::string& message) const {
    if (logger_) {
        logger_(message);
    }
}

ConfigFormat ConfigManager::DetectFormat(const std::string& filepath) {
    if (filepath.ends_with(".json") || filepath.ends_with(".JSON")) {
        return ConfigFormat::Json;
    }
    if (filepath.ends_with(".cfg") || filepath.ends_with(".CFG")) {
        return ConfigFormat::Cfg;
    }

    // Inspect first non-whitespace character
    std::ifstream file(filepath);
    if (file.is_open()) {
        char ch;
        while (file.get(ch)) {
            if (!std::isspace(static_cast<unsigned char>(ch))) {
                if (ch == '{' || ch == '[') return ConfigFormat::Json;
                break;
            }
        }
    }
    return ConfigFormat::Cfg;
}

void ConfigManager::InitializeDefaultFiles() {
    std::unique_lock lock(mutex_);

    bool loadedAny = false;

    // config.json -> ID 1 (CONFIG_FILE_JSON)
    if (fs::exists("config.json") && fs::is_regular_file("config.json")) {
        auto doc = std::make_shared<JsonConfigDocument>("config.json");
        if (doc->Load("config.json")) {
            documentsById_[CONFIG_FILE_JSON] = doc;
            pathToId_["config.json"] = CONFIG_FILE_JSON;
            activeFileId_ = CONFIG_FILE_JSON;
            loadedAny = true;
            Log("[ConfigManager] Loaded open.mp config.json (Identifier: CONFIG_FILE_JSON [1]).");
        }
    }

    // server.cfg -> ID 2 (CONFIG_FILE_CFG)
    if (fs::exists("server.cfg") && fs::is_regular_file("server.cfg")) {
        auto doc = std::make_shared<CfgConfigDocument>("server.cfg");
        if (doc->Load("server.cfg")) {
            documentsById_[CONFIG_FILE_CFG] = doc;
            pathToId_["server.cfg"] = CONFIG_FILE_CFG;
            if (activeFileId_ == 0) {
                activeFileId_ = CONFIG_FILE_CFG;
            }
            loadedAny = true;
            Log("[ConfigManager] Loaded server.cfg (Identifier: CONFIG_FILE_CFG [2]).");
        }
    }

    if (!loadedAny) {
        // Fallback default: prepare empty config.json at ID 1
        activeFileId_ = CONFIG_FILE_JSON;
        documentsById_[CONFIG_FILE_JSON] = std::make_shared<JsonConfigDocument>("config.json");
        pathToId_["config.json"] = CONFIG_FILE_JSON;
        Log("[ConfigManager] No existing configuration files found; initialized empty config.json (ID 1).");
    }
}

int ConfigManager::LoadFile(const std::string& filepath, ConfigFormat format) {
    std::unique_lock lock(mutex_);

    if (filepath == "config.json" && documentsById_.count(CONFIG_FILE_JSON)) {
        activeFileId_ = CONFIG_FILE_JSON;
        return CONFIG_FILE_JSON;
    }
    if (filepath == "server.cfg" && documentsById_.count(CONFIG_FILE_CFG)) {
        activeFileId_ = CONFIG_FILE_CFG;
        return CONFIG_FILE_CFG;
    }

    auto existing = pathToId_.find(filepath);
    if (existing != pathToId_.end()) {
        activeFileId_ = existing->second;
        return existing->second;
    }

    ConfigFormat effectiveFormat = format;
    if (effectiveFormat == ConfigFormat::Auto) {
        effectiveFormat = DetectFormat(filepath);
    }

    std::shared_ptr<IConfigDocument> doc;
    if (effectiveFormat == ConfigFormat::Json) {
        doc = std::make_shared<JsonConfigDocument>(filepath);
    } else {
        doc = std::make_shared<CfgConfigDocument>(filepath);
    }

    if (!doc->Load(filepath)) {
        Log("[ConfigManager] Failed to load config file: " + filepath);
        return CONFIG_FILE_INVALID;
    }

    int fileId = nextFileId_++;
    documentsById_[fileId] = doc;
    pathToId_[filepath] = fileId;
    activeFileId_ = fileId;
    Log("[ConfigManager] Loaded configuration file: " + filepath + " with Identifier: " + std::to_string(fileId));
    return fileId;
}

bool ConfigManager::SaveFile(int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    if (!doc) {
        Log("[ConfigManager] Save failed: file identifier not loaded: " + std::to_string(fileId));
        return false;
    }

    bool ok = doc->Save(doc->GetFilePath());
    if (ok) {
        Log("[ConfigManager] Saved config file: " + doc->GetFilePath());
    } else {
        Log("[ConfigManager] Failed saving config file: " + doc->GetFilePath());
    }
    return ok;
}

bool ConfigManager::SaveAll() {
    std::unique_lock lock(mutex_);
    bool allOk = true;
    for (auto& [id, doc] : documentsById_) {
        if (doc && doc->IsDirty()) {
            if (!doc->Save(doc->GetFilePath())) {
                allOk = false;
                Log("[ConfigManager] Failed saving: " + doc->GetFilePath());
            } else {
                Log("[ConfigManager] Saved: " + doc->GetFilePath());
            }
        }
    }
    return allOk;
}

bool ConfigManager::ReloadFile(int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    if (!doc) return false;
    return doc->Load(doc->GetFilePath());
}

bool ConfigManager::CloseFile(int fileId) {
    std::unique_lock lock(mutex_);
    int effectiveId = (fileId == 0) ? activeFileId_ : fileId;
    auto it = documentsById_.find(effectiveId);
    if (it == documentsById_.end()) return false;

    if (it->second->IsDirty()) {
        it->second->Save(it->second->GetFilePath());
    }
    pathToId_.erase(it->second->GetFilePath());
    documentsById_.erase(it);

    if (activeFileId_ == effectiveId) {
        activeFileId_ = documentsById_.empty() ? 0 : documentsById_.begin()->first;
    }
    return true;
}

void ConfigManager::CloseAll() {
    std::unique_lock lock(mutex_);
    for (auto& [id, doc] : documentsById_) {
        if (doc && doc->IsDirty()) doc->Save(doc->GetFilePath());
    }
    documentsById_.clear();
    pathToId_.clear();
    activeFileId_ = 0;
}

bool ConfigManager::HasFile(int fileId) const {
    std::shared_lock lock(mutex_);
    return ResolveDocumentLocked(fileId) != nullptr;
}

bool ConfigManager::SetActiveFile(int fileId) {
    std::unique_lock lock(mutex_);
    if (ResolveDocumentLocked(fileId) != nullptr) {
        activeFileId_ = (fileId == 0) ? activeFileId_ : fileId;
        return true;
    }
    return false;
}

int ConfigManager::GetActiveFile() const {
    std::shared_lock lock(mutex_);
    return activeFileId_;
}

std::string ConfigManager::GetFilePath(int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetFilePath() : "";
}

bool ConfigManager::IsDirty(int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->IsDirty() : false;
}

std::shared_ptr<IConfigDocument> ConfigManager::GetDocument(int fileId) const {
    std::shared_lock lock(mutex_);
    return ResolveDocumentLocked(fileId);
}

std::shared_ptr<IConfigDocument> ConfigManager::ResolveDocumentLocked(int fileId) const {
    int effectiveId = (fileId == 0) ? activeFileId_ : fileId;
    auto it = documentsById_.find(effectiveId);
    if (it != documentsById_.end()) return it->second;
    return nullptr;
}

// Forwarding Operations

bool ConfigManager::HasKey(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->HasKey(key) : false;
}

bool ConfigManager::RemoveKey(const std::string& key, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->RemoveKey(key) : false;
}

std::optional<std::string> ConfigManager::GetString(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetString(key) : std::nullopt;
}

std::optional<int> ConfigManager::GetInt(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetInt(key) : std::nullopt;
}

std::optional<int64_t> ConfigManager::GetInt64(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetInt64(key) : std::nullopt;
}

std::optional<float> ConfigManager::GetFloat(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetFloat(key) : std::nullopt;
}

std::optional<bool> ConfigManager::GetBool(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetBool(key) : std::nullopt;
}

bool ConfigManager::SetString(const std::string& key, const std::string& value, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->SetString(key, value) : false;
}

bool ConfigManager::SetInt(const std::string& key, int value, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->SetInt(key, value) : false;
}

bool ConfigManager::SetInt64(const std::string& key, int64_t value, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->SetInt64(key, value) : false;
}

bool ConfigManager::SetFloat(const std::string& key, float value, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->SetFloat(key, value) : false;
}

bool ConfigManager::SetBool(const std::string& key, bool value, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->SetBool(key, value) : false;
}

int ConfigManager::GetArrayLength(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetArrayLength(key) : -1;
}

std::optional<std::string> ConfigManager::GetArrayString(const std::string& key, size_t index, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetArrayString(key, index) : std::nullopt;
}

bool ConfigManager::SetArrayString(const std::string& key, size_t index, const std::string& value, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->SetArrayString(key, index, value) : false;
}

bool ConfigManager::AppendArrayString(const std::string& key, const std::string& value, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->AppendArrayString(key, value) : false;
}

bool ConfigManager::RemoveArrayElement(const std::string& key, size_t index, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->RemoveArrayElement(key, index) : false;
}

bool ConfigManager::ClearArray(const std::string& key, int fileId) {
    std::unique_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->ClearArray(key) : false;
}

ConfigFormat ConfigManager::GetFormat(int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetFormat() : ConfigFormat::Unknown;
}

int ConfigManager::GetCfgTokenCount(const std::string& key, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetCfgTokenCount(key) : -1;
}

std::optional<std::string> ConfigManager::GetCfgToken(const std::string& key, size_t index, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetCfgToken(key, index) : std::nullopt;
}

int ConfigManager::GetCfgGamemodeCount(int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetCfgGamemodeCount() : -1;
}

std::optional<std::string> ConfigManager::GetCfgGamemode(size_t index, int fileId) const {
    std::shared_lock lock(mutex_);
    auto doc = ResolveDocumentLocked(fileId);
    return doc ? doc->GetCfgGamemode(index) : std::nullopt;
}

bool ConfigManager::AtomicWriteFile(const std::string& filepath, const std::string& content, LoggerFunc logger) {
    std::string tempPath = filepath + ".tmp";
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            if (logger) logger("[ConfigManager] ERROR: Cannot open temp file for writing: " + tempPath);
            return false;
        }
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.flush();
        if (!out.good()) {
            if (logger) logger("[ConfigManager] ERROR: Buffer write failed for: " + tempPath);
            return false;
        }
    }

#if defined(_WIN32)
    std::wstring wTemp(tempPath.begin(), tempPath.end());
    std::wstring wTarget(filepath.begin(), filepath.end());

    if (!MoveFileExW(wTemp.c_str(), wTarget.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(wTarget.c_str());
        if (!MoveFileExW(wTemp.c_str(), wTarget.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            if (logger) logger("[ConfigManager] ERROR: MoveFileEx failed for " + filepath);
            return false;
        }
    }
    return true;
#else
    std::error_code ec;
    fs::rename(tempPath, filepath, ec);
    if (ec) {
        fs::remove(filepath, ec);
        ec.clear();
        fs::rename(tempPath, filepath, ec);
        if (ec) {
            if (logger) logger("[ConfigManager] ERROR: Atomic rename failed: " + ec.message());
            return false;
        }
    }
    return true;
#endif
}
