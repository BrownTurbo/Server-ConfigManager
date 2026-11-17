#include "Natives.hpp"
#include "ConfigEngine.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

#include <amx/amx.h>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>

namespace {
inline cell FloatToPawnCell(float val) {
	double d = static_cast<double>(val);
	return amx_ftoc(d);
}
}

#undef PAWN_NATIVE_DEFN_
#define PAWN_NATIVE_DEFN_(used_namespace, failret, func, params)                     \
                                                                                     \
    template <>                                                                      \
    cell AMX_NATIVE_CALL Native_##func::Call(AMX *amx, cell *args)                   \
    {                                                                                \
        if (amx && args)                                                             \
        {                                                                            \
            AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER *>(amx->base);             \
            if (hdr && hdr->magic == 0xf1e0)                                         \
            {                                                                        \
                const uint32_t *args32 = reinterpret_cast<const uint32_t *>(args);   \
                uint32_t byte_count = args32[0];                                     \
                uint32_t num_args = byte_count / sizeof(uint32_t);                   \
                cell args64[64];                                                     \
                uint32_t count = (num_args < 63) ? num_args : 63;                    \
                args64[0] = static_cast<cell>(count * sizeof(cell));                 \
                for (uint32_t i = 1; i <= count; ++i)                                \
                {                                                                    \
                    args64[i] = static_cast<cell>(static_cast<uint64_t>(args32[i])); \
                }                                                                    \
                return used_namespace::func.CallDoOuter<failret>(amx, args64);       \
            }                                                                        \
        }                                                                            \
        return used_namespace::func.CallDoOuter<failret>(amx, args);                 \
    }                                                                                \
                                                                                     \
    template <>                                                                      \
    Native_##func::Native_##func##_()                                                \
        : Base(#func, (AMX_NATIVE) & Call)                                           \
    {                                                                                \
    }                                                                                \
                                                                                     \
    Native_##func used_namespace::func;                                              \
                                                                                     \
    template <>                                                                      \
    PAWN_NATIVE__RETURN(params)                                                      \
    Native_##func::                                                                  \
        Do(PAWN_NATIVE__PARAMETERS(params)) const;                                   \
                                                                                     \
    template <typename RET, typename... TS>                                          \
    typename pawn_natives::ReturnResolver<RET>::type NATIVE_##func(TS... args)       \
    {                                                                                \
        try                                                                          \
        {                                                                            \
            PAWN_NATIVE__GET_RETURN(params)                                          \
            (used_namespace::func.Do(args...));                                      \
        }                                                                            \
        catch (std::exception & e)                                                   \
        {                                                                            \
            char msg[1024];                                                          \
            sprintf(msg, "Exception in _" #func ": \"%s\"", e.what());               \
            LOG_NATIVE_ERROR(msg);                                                   \
        }                                                                            \
        catch (...)                                                                  \
        {                                                                            \
            LOG_NATIVE_ERROR("Unknown exception in _" #func);                        \
        }                                                                            \
        PAWN_NATIVE__DEFAULT_RETURN(params);                                         \
    }                                                                                \
                                                                                     \
    template <>                                                                      \
    PAWN_NATIVE__RETURN(params)                                                      \
    Native_##func::                                                                  \
        Do(PAWN_NATIVE__PARAMETERS(params)) const

// ============================================================================
// File Management Natives (pawn-natives SCRIPT_API)
// ============================================================================

SCRIPT_API_FAILRET(ConfigLoadFile, -1, int(const std::string& filename))
{
	return ConfigManager::Instance().LoadFile(filename);
}

SCRIPT_API(ConfigGetFormat, int(int file))
{
	return static_cast<int>(ConfigManager::Instance().GetFormat(file));
}

SCRIPT_API(ConfigSaveFile, bool(int file))
{
	return ConfigManager::Instance().SaveFile(file);
}

SCRIPT_API(ConfigSaveAll, bool())
{
	return ConfigManager::Instance().SaveAll();
}

SCRIPT_API(ConfigReload, bool(int file))
{
	return ConfigManager::Instance().ReloadFile(file);
}

SCRIPT_API(ConfigIsDirty, bool(int file))
{
	return ConfigManager::Instance().IsDirty(file);
}

SCRIPT_API(ConfigSetActiveFile, bool(int file))
{
	return ConfigManager::Instance().SetActiveFile(file);
}

SCRIPT_API(ConfigGetActiveFile, int())
{
	return ConfigManager::Instance().GetActiveFile();
}

SCRIPT_API(ConfigGetFileName, bool(int file, std::string& dest))
{
	std::string path = ConfigManager::Instance().GetFilePath(file);
	if (path.empty()) {
		dest.clear();
		return false;
	}
	dest = std::move(path);
	return true;
}

SCRIPT_API(ConfigHasFile, bool(int file))
{
	return ConfigManager::Instance().HasFile(file);
}

SCRIPT_API(ConfigCloseFile, bool(int file))
{
	return ConfigManager::Instance().CloseFile(file);
}

// ============================================================================
// Active-File Operations (Operates on active config file)
// ============================================================================

SCRIPT_API(ConfigKeyExists, bool(const std::string& key))
{
	return ConfigManager::Instance().HasKey(key, 0);
}

SCRIPT_API(ConfigRemoveKey, bool(const std::string& key))
{
	return ConfigManager::Instance().RemoveKey(key, 0);
}

SCRIPT_API(ConfigGetString, bool(const std::string& key, std::string& dest))
{
	auto val = ConfigManager::Instance().GetString(key, 0);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}

SCRIPT_API(ConfigGetInt, int(const std::string& key, int default_val))
{
	return ConfigManager::Instance().GetInt(key, 0).value_or(default_val);
}

SCRIPT_API(ConfigGetFloat, cell(const std::string& key, float default_val))
{
	float val = ConfigManager::Instance().GetFloat(key, 0).value_or(default_val);
	return FloatToPawnCell(val);
}

SCRIPT_API(ConfigGetBool, bool(const std::string& key, bool default_val))
{
	return ConfigManager::Instance().GetBool(key, 0).value_or(default_val);
}

SCRIPT_API(ConfigSetString, bool(const std::string& key, const std::string& value))
{
	return ConfigManager::Instance().SetString(key, value, 0);
}

SCRIPT_API(ConfigSetInt, bool(const std::string& key, int value))
{
	return ConfigManager::Instance().SetInt(key, value, 0);
}

SCRIPT_API(ConfigSetFloat, bool(const std::string& key, float value))
{
	return ConfigManager::Instance().SetFloat(key, value, 0);
}

SCRIPT_API(ConfigSetBool, bool(const std::string& key, bool value))
{
	return ConfigManager::Instance().SetBool(key, value, 0);
}

SCRIPT_API_FAILRET(ConfigGetArrayLength, -1, int(const std::string& key))
{
	return ConfigManager::Instance().GetArrayLength(key, 0);
}

SCRIPT_API(ConfigGetArrayString, bool(const std::string& key, int index, std::string& dest))
{
	if (index < 0) {
		dest.clear();
		return false;
	}
	auto val = ConfigManager::Instance().GetArrayString(key, static_cast<size_t>(index), 0);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}

SCRIPT_API(ConfigSetArrayString, bool(const std::string& key, int index, const std::string& value))
{
	if (index < 0) return false;
	return ConfigManager::Instance().SetArrayString(key, static_cast<size_t>(index), value, 0);
}

SCRIPT_API(ConfigAppendArrayString, bool(const std::string& key, const std::string& value))
{
	return ConfigManager::Instance().AppendArrayString(key, value, 0);
}

SCRIPT_API(ConfigRemoveArrayElement, bool(const std::string& key, int index))
{
	if (index < 0) return false;
	return ConfigManager::Instance().RemoveArrayElement(key, static_cast<size_t>(index), 0);
}

SCRIPT_API(ConfigClearArray, bool(const std::string& key))
{
	return ConfigManager::Instance().ClearArray(key, 0);
}

// ============================================================================
// File-Specific Operations (Target any file by Identifier)
// ============================================================================

SCRIPT_API(ConfigFileKeyExists, bool(int file, const std::string& key))
{
	return ConfigManager::Instance().HasKey(key, file);
}

SCRIPT_API(ConfigFileRemoveKey, bool(int file, const std::string& key))
{
	return ConfigManager::Instance().RemoveKey(key, file);
}

SCRIPT_API(ConfigFileGetString, bool(int file, const std::string& key, std::string& dest))
{
	auto val = ConfigManager::Instance().GetString(key, file);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}

SCRIPT_API(ConfigFileGetInt, int(int file, const std::string& key, int default_val))
{
	return ConfigManager::Instance().GetInt(key, file).value_or(default_val);
}

SCRIPT_API(ConfigFileGetFloat, cell(int file, const std::string& key, float default_val))
{
	float val = ConfigManager::Instance().GetFloat(key, file).value_or(default_val);
	return FloatToPawnCell(val);
}

SCRIPT_API(ConfigFileGetBool, bool(int file, const std::string& key, bool default_val))
{
	return ConfigManager::Instance().GetBool(key, file).value_or(default_val);
}

SCRIPT_API(ConfigFileSetString, bool(int file, const std::string& key, const std::string& value))
{
	return ConfigManager::Instance().SetString(key, value, file);
}

SCRIPT_API(ConfigFileSetInt, bool(int file, const std::string& key, int value))
{
	return ConfigManager::Instance().SetInt(key, value, file);
}

SCRIPT_API(ConfigFileSetFloat, bool(int file, const std::string& key, float value))
{
	return ConfigManager::Instance().SetFloat(key, value, file);
}

SCRIPT_API(ConfigFileSetBool, bool(int file, const std::string& key, bool value))
{
	return ConfigManager::Instance().SetBool(key, value, file);
}

SCRIPT_API_FAILRET(ConfigFileGetArrayLength, -1, int(int file, const std::string& key))
{
	return ConfigManager::Instance().GetArrayLength(key, file);
}

SCRIPT_API(ConfigFileGetArrayString, bool(int file, const std::string& key, int index, std::string& dest))
{
	if (index < 0) {
		dest.clear();
		return false;
	}
	auto val = ConfigManager::Instance().GetArrayString(key, static_cast<size_t>(index), file);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}

SCRIPT_API(ConfigFileSetArrayString, bool(int file, const std::string& key, int index, const std::string& value))
{
	if (index < 0) return false;
	return ConfigManager::Instance().SetArrayString(key, static_cast<size_t>(index), value, file);
}

SCRIPT_API(ConfigFileAppendArrayString, bool(int file, const std::string& key, const std::string& value))
{
	return ConfigManager::Instance().AppendArrayString(key, value, file);
}

SCRIPT_API(ConfigFileRemoveArrayElement, bool(int file, const std::string& key, int index))
{
	if (index < 0) return false;
	return ConfigManager::Instance().RemoveArrayElement(key, static_cast<size_t>(index), file);
}

SCRIPT_API(ConfigFileClearArray, bool(int file, const std::string& key))
{
	return ConfigManager::Instance().ClearArray(key, file);
}

// ============================================================================
// server.cfg Dedicated Token & Gamemode Operations
// ============================================================================

SCRIPT_API_FAILRET(ConfigCfgGetTokenCount, -1, int(const std::string& key))
{
	return ConfigManager::Instance().GetCfgTokenCount(key, 0);
}

SCRIPT_API(ConfigCfgGetToken, bool(const std::string& key, int token_index, std::string& dest))
{
	if (token_index < 0) {
		dest.clear();
		return false;
	}
	auto val = ConfigManager::Instance().GetCfgToken(key, static_cast<size_t>(token_index), 0);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}

SCRIPT_API_FAILRET(ConfigCfgGetGamemodeCount, -1, int())
{
	return ConfigManager::Instance().GetCfgGamemodeCount(0);
}

SCRIPT_API(ConfigCfgGetGamemode, bool(int index, std::string& dest))
{
	if (index < 0) {
		dest.clear();
		return false;
	}
	auto val = ConfigManager::Instance().GetCfgGamemode(static_cast<size_t>(index), 0);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}

SCRIPT_API_FAILRET(ConfigFileCfgGetTokenCount, -1, int(int file, const std::string& key))
{
	return ConfigManager::Instance().GetCfgTokenCount(key, file);
}

SCRIPT_API(ConfigFileCfgGetToken, bool(int file, const std::string& key, int token_index, std::string& dest))
{
	if (token_index < 0) {
		dest.clear();
		return false;
	}
	auto val = ConfigManager::Instance().GetCfgToken(key, static_cast<size_t>(token_index), file);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}

SCRIPT_API_FAILRET(ConfigFileCfgGetGamemodeCount, -1, int(int file))
{
	return ConfigManager::Instance().GetCfgGamemodeCount(file);
}

SCRIPT_API(ConfigFileCfgGetGamemode, bool(int file, int index, std::string& dest))
{
	if (index < 0) {
		dest.clear();
		return false;
	}
	auto val = ConfigManager::Instance().GetCfgGamemode(static_cast<size_t>(index), file);
	if (!val.has_value()) {
		dest.clear();
		return false;
	}
	dest = std::move(*val);
	return true;
}
