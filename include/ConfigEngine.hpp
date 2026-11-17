#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

enum class ConfigFormat {
    Unknown = 0,
    Json    = 1,
    Cfg     = 2,
    Auto    = 3
};

/**
 * Interface representing an individual configuration file / document.
 * Each document implements its own format engine (JSON or CFG).
 */
class IConfigDocument {
public:
    virtual ~IConfigDocument() = default;

    virtual ConfigFormat GetFormat() const noexcept = 0;
    virtual const std::string& GetFilePath() const noexcept = 0;
    virtual bool Load(const std::string& filepath) = 0;
    virtual bool Save(const std::string& filepath = "") = 0;
    virtual bool IsDirty() const noexcept = 0;

    virtual bool HasKey(const std::string& key) const = 0;
    virtual bool RemoveKey(const std::string& key) = 0;

    virtual std::optional<std::string> GetString(const std::string& key) const = 0;
    virtual std::optional<int> GetInt(const std::string& key) const = 0;
    virtual std::optional<int64_t> GetInt64(const std::string& key) const = 0;
    virtual std::optional<float> GetFloat(const std::string& key) const = 0;
    virtual std::optional<bool> GetBool(const std::string& key) const = 0;

    virtual bool SetString(const std::string& key, const std::string& value) = 0;
    virtual bool SetInt(const std::string& key, int value) = 0;
    virtual bool SetInt64(const std::string& key, int64_t value) = 0;
    virtual bool SetFloat(const std::string& key, float value) = 0;
    virtual bool SetBool(const std::string& key, bool value) = 0;

    // Structured JSON array operations (return -1 / false on non-array or formats without native arrays)
    virtual int GetArrayLength(const std::string& key) const = 0;
    virtual std::optional<std::string> GetArrayString(const std::string& key, size_t index) const = 0;
    virtual bool SetArrayString(const std::string& key, size_t index, const std::string& value) = 0;
    virtual bool AppendArrayString(const std::string& key, const std::string& value) = 0;
    virtual bool RemoveArrayElement(const std::string& key, size_t index) = 0;
    virtual bool ClearArray(const std::string& key) = 0;

    // Line-oriented / server.cfg specific token and gamemode helpers
    virtual int GetCfgTokenCount(const std::string& key) const { (void)key; return -1; }
    virtual std::optional<std::string> GetCfgToken(const std::string& key, size_t index) const { (void)key; (void)index; return std::nullopt; }
    virtual int GetCfgGamemodeCount() const { return -1; }
    virtual std::optional<std::string> GetCfgGamemode(size_t index) const { (void)index; return std::nullopt; }
};

/**
 * Dedicated engine for JSON configuration files (e.g. config.json).
 * Supports nested dot-paths, comments (// and /* * /), arrays, and typed values.
 */
class JsonConfigDocument final : public IConfigDocument {
public:
    JsonConfigDocument() = default;
    explicit JsonConfigDocument(std::string filepath);

    ConfigFormat GetFormat() const noexcept override { return ConfigFormat::Json; }
    const std::string& GetFilePath() const noexcept override { return filePath_; }
    bool Load(const std::string& filepath) override;
    bool Save(const std::string& filepath = "") override;
    bool IsDirty() const noexcept override { return dirty_; }

    bool HasKey(const std::string& key) const override;
    bool RemoveKey(const std::string& key) override;

    std::optional<std::string> GetString(const std::string& key) const override;
    std::optional<int> GetInt(const std::string& key) const override;
    std::optional<int64_t> GetInt64(const std::string& key) const override;
    std::optional<float> GetFloat(const std::string& key) const override;
    std::optional<bool> GetBool(const std::string& key) const override;

    bool SetString(const std::string& key, const std::string& value) override;
    bool SetInt(const std::string& key, int value) override;
    bool SetInt64(const std::string& key, int64_t value) override;
    bool SetFloat(const std::string& key, float value) override;
    bool SetBool(const std::string& key, bool value) override;

    int GetArrayLength(const std::string& key) const override;
    std::optional<std::string> GetArrayString(const std::string& key, size_t index) const override;
    bool SetArrayString(const std::string& key, size_t index, const std::string& value) override;
    bool AppendArrayString(const std::string& key, const std::string& value) override;
    bool RemoveArrayElement(const std::string& key, size_t index) override;
    bool ClearArray(const std::string& key) override;

    const nlohmann::json& GetRootJson() const noexcept { return rootJson_; }

private:
    std::string ResolveCanonicalPath(const std::string& key) const;
    const nlohmann::json* ResolveNode(const std::string& key) const;
    nlohmann::json* ResolveOrCreateNode(const std::string& key);

    std::string filePath_;
    bool dirty_ = false;
    nlohmann::json rootJson_ = nlohmann::json::object();
};

/**
 * Dedicated engine for line-oriented SA-MP configuration files (e.g. server.cfg).
 * Preserves custom comments, whitespace, blank lines, and directive order on writes.
 */
class CfgConfigDocument final : public IConfigDocument {
public:
    CfgConfigDocument() = default;
    explicit CfgConfigDocument(std::string filepath);

    ConfigFormat GetFormat() const noexcept override { return ConfigFormat::Cfg; }
    const std::string& GetFilePath() const noexcept override { return filePath_; }
    bool Load(const std::string& filepath) override;
    bool Save(const std::string& filepath = "") override;
    bool IsDirty() const noexcept override { return dirty_; }

    bool HasKey(const std::string& key) const override;
    bool RemoveKey(const std::string& key) override;

    std::optional<std::string> GetString(const std::string& key) const override;
    std::optional<int> GetInt(const std::string& key) const override;
    std::optional<int64_t> GetInt64(const std::string& key) const override;
    std::optional<float> GetFloat(const std::string& key) const override;
    std::optional<bool> GetBool(const std::string& key) const override;

    bool SetString(const std::string& key, const std::string& value) override;
    bool SetInt(const std::string& key, int value) override;
    bool SetInt64(const std::string& key, int64_t value) override;
    bool SetFloat(const std::string& key, float value) override;
    bool SetBool(const std::string& key, bool value) override;

    int GetArrayLength(const std::string& key) const override;
    std::optional<std::string> GetArrayString(const std::string& key, size_t index) const override;
    bool SetArrayString(const std::string& key, size_t index, const std::string& value) override;
    bool AppendArrayString(const std::string& key, const std::string& value) override;
    bool RemoveArrayElement(const std::string& key, size_t index) override;
    bool ClearArray(const std::string& key) override;

    // server.cfg specific token & gamemode overrides
    int GetCfgTokenCount(const std::string& key) const override;
    std::optional<std::string> GetCfgToken(const std::string& key, size_t index) const override;
    int GetCfgGamemodeCount() const override;
    std::optional<std::string> GetCfgGamemode(size_t index) const override;

private:
    std::string ResolveLegacyKey(const std::string& key) const;
    static size_t FindCommentStart(const std::string& line);

    std::string filePath_;
    bool dirty_ = false;
    std::vector<std::string> rawLines_;
    std::unordered_map<std::string, std::string> flatValues_;
    std::vector<std::string> gamemodes_;
};

enum ConfigFileIdentifier : int {
    CONFIG_FILE_INVALID = -1,
    CONFIG_FILE_ACTIVE  = 0,
    CONFIG_FILE_JSON    = 1,
    CONFIG_FILE_CFG     = 2
};

/**
 * Central configuration manager coordinating multiple configuration files and engines.
 * Manages an active file while allowing operations on any specified config file via Identifier.
 */
class ConfigManager {
public:
    using LoggerFunc = std::function<void(const std::string&)>;

    static ConfigManager& Instance();

    void SetLogger(LoggerFunc logger);
    void Log(const std::string& message) const;

    // Startup discovery: loads config.json and/or server.cfg if present
    void InitializeDefaultFiles();

    // Document management
    int LoadFile(const std::string& filepath, ConfigFormat format = ConfigFormat::Auto);
    bool SaveFile(int fileId = 0);
    bool SaveAll();
    bool ReloadFile(int fileId = 0);
    bool CloseFile(int fileId);
    void CloseAll();

    bool HasFile(int fileId) const;
    bool SetActiveFile(int fileId);
    int GetActiveFile() const;
    std::string GetFilePath(int fileId = 0) const;
    bool IsDirty(int fileId = 0) const;

    std::shared_ptr<IConfigDocument> GetDocument(int fileId = 0) const;

    // Forwarding operations on active file or specified file identifier
    bool HasKey(const std::string& key, int fileId = 0) const;
    bool RemoveKey(const std::string& key, int fileId = 0);

    std::optional<std::string> GetString(const std::string& key, int fileId = 0) const;
    std::optional<int> GetInt(const std::string& key, int fileId = 0) const;
    std::optional<int64_t> GetInt64(const std::string& key, int fileId = 0) const;
    std::optional<float> GetFloat(const std::string& key, int fileId = 0) const;
    std::optional<bool> GetBool(const std::string& key, int fileId = 0) const;

    bool SetString(const std::string& key, const std::string& value, int fileId = 0);
    bool SetInt(const std::string& key, int value, int fileId = 0);
    bool SetInt64(const std::string& key, int64_t value, int fileId = 0);
    bool SetFloat(const std::string& key, float value, int fileId = 0);
    bool SetBool(const std::string& key, bool value, int fileId = 0);

    int GetArrayLength(const std::string& key, int fileId = 0) const;
    std::optional<std::string> GetArrayString(const std::string& key, size_t index, int fileId = 0) const;
    bool SetArrayString(const std::string& key, size_t index, const std::string& value, int fileId = 0);
    bool AppendArrayString(const std::string& key, const std::string& value, int fileId = 0);
    bool RemoveArrayElement(const std::string& key, size_t index, int fileId = 0);
    bool ClearArray(const std::string& key, int fileId = 0);

    // Format query & server.cfg specific readers
    ConfigFormat GetFormat(int fileId = 0) const;
    int GetCfgTokenCount(const std::string& key, int fileId = 0) const;
    std::optional<std::string> GetCfgToken(const std::string& key, size_t index, int fileId = 0) const;
    int GetCfgGamemodeCount(int fileId = 0) const;
    std::optional<std::string> GetCfgGamemode(size_t index, int fileId = 0) const;

    // Utility
    static bool AtomicWriteFile(const std::string& filepath, const std::string& content, LoggerFunc logger);
    static ConfigFormat DetectFormat(const std::string& filepath);

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::shared_ptr<IConfigDocument> ResolveDocumentLocked(int fileId) const;

    mutable std::shared_mutex mutex_;
    std::unordered_map<int, std::shared_ptr<IConfigDocument>> documentsById_;
    std::unordered_map<std::string, int> pathToId_;
    int activeFileId_ = 0;
    int nextFileId_ = 3;
    LoggerFunc logger_;
};
