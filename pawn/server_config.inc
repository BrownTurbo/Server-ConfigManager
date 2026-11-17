#if defined _server_config_included
    #endinput
#endif
#define _server_config_included

/**
 * ============================================================================
 *  Server-ConfigManager for open.mp
 *  Author: Zorono
 * ============================================================================
 */

/**
 * <summary>Standard file identifiers for open.mp configuration files.</summary>
 */
enum ConfigFile: {
    CONFIG_FILE_INVALID = -1,
    CONFIG_FILE_AUTO    = 0,
    CONFIG_FILE_ACTIVE  = 0,
    CONFIG_FILE_DEFAULT = 0,
    CONFIG_FILE_JSON    = 1,
    CONFIG_FILE_CFG     = 2,
    CONFIG_JSON         = 1,
    CONFIG_CFG          = 2
};

/**
 * <summary>Format types for loaded configuration files.</summary>
 */
enum ConfigFormat: {
    CONFIG_FORMAT_UNKNOWN = 0,
    CONFIG_FORMAT_JSON    = 1,
    CONFIG_FORMAT_CFG     = 2
};

// ============================================================================
// File Management Natives (Identifier-Based)
// ============================================================================

/**
 * <summary>Loads or creates a configuration file into memory and registers an identifier for it.</summary>
 * <param name="filename">Path or name of the file (e.g. "config.json", "server.cfg", "database.json").</param>
 * <returns>ConfigFile identifier on success (>= 1), or CONFIG_FILE_INVALID (-1) on failure.</returns>
 */
native ConfigFile:ConfigLoadFile(const filename[]);

/**
 * <summary>Saves staged modifications back to disk for the specified file identifier (or active file if omitted).</summary>
 * <param name="file">ConfigFile identifier. Defaults to CONFIG_FILE_AUTO (active file).</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigSaveFile(ConfigFile:file = CONFIG_FILE_AUTO);

/**
 * <summary>Flushes all open, modified configuration files atomically back to disk.</summary>
 * <returns>true if all dirty files saved successfully, false otherwise.</returns>
 */
native bool:ConfigSaveAll();

/**
 * <summary>Reloads the specified file (or active file if omitted) from disk.</summary>
 * <param name="file">ConfigFile identifier. Defaults to CONFIG_FILE_AUTO (active file).</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigReload(ConfigFile:file = CONFIG_FILE_AUTO);

/**
 * <summary>Checks whether a configuration file has uncommitted changes staged in memory.</summary>
 * <param name="file">ConfigFile identifier. Defaults to CONFIG_FILE_AUTO (active file).</param>
 * <returns>true if there are unsaved changes, false otherwise.</returns>
 */
native bool:ConfigIsDirty(ConfigFile:file = CONFIG_FILE_AUTO);

/**
 * <summary>Sets the currently active configuration file for standard Config* natives.</summary>
 * <param name="file">ConfigFile identifier to switch to.</param>
 * <returns>true on success, false if file is not loaded.</returns>
 */
native bool:ConfigSetActiveFile(ConfigFile:file);

/**
 * <summary>Retrieves the identifier of the currently active configuration file.</summary>
 * <returns>ConfigFile identifier of the active file.</returns>
 */
native ConfigFile:ConfigGetActiveFile();

/**
 * <summary>Retrieves the file path / name associated with a ConfigFile identifier.</summary>
 * <param name="file">ConfigFile identifier to query.</param>
 * <param name="dest">Output string buffer.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigGetFileName(ConfigFile:file, dest[], max_len = sizeof(dest));

/**
 * <summary>Checks if a configuration file identifier is currently loaded in memory.</summary>
 * <param name="file">ConfigFile identifier to check.</param>
 * <returns>true if loaded, false otherwise.</returns>
 */
native bool:ConfigHasFile(ConfigFile:file);

/**
 * <summary>Closes and unloads a configuration file from memory (auto-saves if modified).</summary>
 * <param name="file">ConfigFile identifier to unload.</param>
 * <returns>true on success, false if file was not loaded or is a core config.</returns>
 */
native bool:ConfigCloseFile(ConfigFile:file);

/**
 * <summary>Retrieves the format type of a loaded configuration file.</summary>
 * <param name="file">ConfigFile identifier. Defaults to CONFIG_FILE_AUTO (active file).</param>
 * <returns>ConfigFormat: CONFIG_FORMAT_JSON (1), CONFIG_FORMAT_CFG (2), or CONFIG_FORMAT_UNKNOWN (0).</returns>
 */
native ConfigFormat:ConfigGetFormat(ConfigFile:file = CONFIG_FILE_AUTO);

// ============================================================================
// Active-File Operations (Operates on the currently active config file)
// ============================================================================

/**
 * <summary>Checks if a key exists in the active config file.</summary>
 * <param name="key">The key or dot-path (e.g. "network.port", "name", "hostname").</param>
 * <returns>true if key exists, false otherwise.</returns>
 */
native bool:ConfigKeyExists(const key[]);

/**
 * <summary>Removes a key from the active config file.</summary>
 * <param name="key">The key or dot-path to remove.</param>
 * <returns>true if removed, false if key did not exist.</returns>
 */
native bool:ConfigRemoveKey(const key[]);

/**
 * <summary>Retrieves a string value from the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="dest">Output string buffer.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false otherwise.</returns>
 */
native bool:ConfigGetString(const key[], dest[], max_len = sizeof(dest));

/**
 * <summary>Retrieves an integer value from the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="default_val">Fallback value if missing.</param>
 * <returns>The integer value, or default_val if missing.</returns>
 */
native ConfigGetInt(const key[], default_val = 0);

/**
 * <summary>Retrieves a floating-point value from the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="default_val">Fallback value if missing.</param>
 * <returns>The float value, or default_val if missing.</returns>
 */
native Float:ConfigGetFloat(const key[], Float:default_val = 0.0);

/**
 * <summary>Retrieves a boolean value from the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="default_val">Fallback value if missing.</param>
 * <returns>The boolean value, or default_val if missing.</returns>
 */
native bool:ConfigGetBool(const key[], bool:default_val = false);

/**
 * <summary>Sets a string value in the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The string value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigSetString(const key[], const value[]);

/**
 * <summary>Sets an integer value in the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The integer value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigSetInt(const key[], value);

/**
 * <summary>Sets a floating-point value in the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The float value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigSetFloat(const key[], Float:value);

/**
 * <summary>Sets a boolean value in the active config file.</summary>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The boolean value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigSetBool(const key[], bool:value);

/**
 * <summary>Retrieves the number of elements in an array in the active config file.</summary>
 * <param name="key">The array key or dot-path.</param>
 * <returns>Array length, or -1 if the key is not an array.</returns>
 */
native ConfigGetArrayLength(const key[]);

/**
 * <summary>Retrieves a string element from an array by index in the active config file.</summary>
 * <param name="key">The array key or dot-path.</param>
 * <param name="index">0-based element index.</param>
 * <param name="dest">Output string buffer.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false on out-of-bounds or non-array.</returns>
 */
native bool:ConfigGetArrayString(const key[], index, dest[], max_len = sizeof(dest));

/**
 * <summary>Overwrites a string element in an array at a specific index in the active config file.</summary>
 * <param name="key">The array key or dot-path.</param>
 * <param name="index">0-based index to overwrite.</param>
 * <param name="value">The new element value.</param>
 * <returns>true on success, false on out-of-bounds or failure.</returns>
 */
native bool:ConfigSetArrayString(const key[], index, const value[]);

/**
 * <summary>Appends a string value to an array in the active config file.</summary>
 * <param name="key">The array key or dot-path.</param>
 * <param name="value">The value to append.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigAppendArrayString(const key[], const value[]);

/**
 * <summary>Removes an element from an array by index in the active config file.</summary>
 * <param name="key">The array key or dot-path.</param>
 * <param name="index">0-based index to remove.</param>
 * <returns>true on success, false on out-of-bounds or failure.</returns>
 */
native bool:ConfigRemoveArrayElement(const key[], index);

/**
 * <summary>Clears all elements from an array in the active config file.</summary>
 * <param name="key">The array key or dot-path.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigClearArray(const key[]);

// ============================================================================
// File-Specific Key Operations (Target any loaded file by its Identifier)
// ============================================================================

/**
 * <summary>Checks if a key exists in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier (e.g. CONFIG_FILE_JSON, CONFIG_FILE_CFG).</param>
 * <param name="key">The key or dot-path.</param>
 * <returns>true if key exists, false otherwise.</returns>
 */
native bool:ConfigFileKeyExists(ConfigFile:file, const key[]);

/**
 * <summary>Removes a key from a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path to remove.</param>
 * <returns>true if removed, false if key did not exist.</returns>
 */
native bool:ConfigFileRemoveKey(ConfigFile:file, const key[]);

/**
 * <summary>Retrieves a string value from a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="dest">Output string buffer.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false otherwise.</returns>
 */
native bool:ConfigFileGetString(ConfigFile:file, const key[], dest[], max_len = sizeof(dest));

/**
 * <summary>Retrieves an integer value from a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="default_val">Fallback value if missing.</param>
 * <returns>The integer value, or default_val if missing.</returns>
 */
native ConfigFileGetInt(ConfigFile:file, const key[], default_val = 0);

/**
 * <summary>Retrieves a floating-point value from a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="default_val">Fallback value if missing.</param>
 * <returns>The float value, or default_val if missing.</returns>
 */
native Float:ConfigFileGetFloat(ConfigFile:file, const key[], Float:default_val = 0.0);

/**
 * <summary>Retrieves a boolean value from a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="default_val">Fallback value if missing.</param>
 * <returns>The boolean value, or default_val if missing.</returns>
 */
native bool:ConfigFileGetBool(ConfigFile:file, const key[], bool:default_val = false);

/**
 * <summary>Sets a string value in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The string value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigFileSetString(ConfigFile:file, const key[], const value[]);

/**
 * <summary>Sets an integer value in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The integer value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigFileSetInt(ConfigFile:file, const key[], value);

/**
 * <summary>Sets a floating-point value in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The float value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigFileSetFloat(ConfigFile:file, const key[], Float:value);

/**
 * <summary>Sets a boolean value in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The key or dot-path.</param>
 * <param name="value">The boolean value.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigFileSetBool(ConfigFile:file, const key[], bool:value);

/**
 * <summary>Retrieves the number of elements in an array in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The array key or dot-path.</param>
 * <returns>Array length, or -1 if the key is not an array.</returns>
 */
native ConfigFileGetArrayLength(ConfigFile:file, const key[]);

/**
 * <summary>Retrieves a string element from an array by index in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The array key or dot-path.</param>
 * <param name="index">0-based element index.</param>
 * <param name="dest">Output string buffer.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false on out-of-bounds or non-array.</returns>
 */
native bool:ConfigFileGetArrayString(ConfigFile:file, const key[], index, dest[], max_len = sizeof(dest));

/**
 * <summary>Overwrites a string element in an array at a specific index in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The array key or dot-path.</param>
 * <param name="index">0-based index to overwrite.</param>
 * <param name="value">The new element value.</param>
 * <returns>true on success, false on out-of-bounds or failure.</returns>
 */
native bool:ConfigFileSetArrayString(ConfigFile:file, const key[], index, const value[]);

/**
 * <summary>Appends a string value to an array in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The array key or dot-path.</param>
 * <param name="value">The value to append.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigFileAppendArrayString(ConfigFile:file, const key[], const value[]);

/**
 * <summary>Removes an element from an array by index in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The array key or dot-path.</param>
 * <param name="index">0-based index to remove.</param>
 * <returns>true on success, false on out-of-bounds or failure.</returns>
 */
native bool:ConfigFileRemoveArrayElement(ConfigFile:file, const key[], index);

/**
 * <summary>Clears all elements from an array in a specific loaded config file.</summary>
 * <param name="file">ConfigFile identifier.</param>
 * <param name="key">The array key or dot-path.</param>
 * <returns>true on success, false on failure.</returns>
 */
native bool:ConfigFileClearArray(ConfigFile:file, const key[]);

// ============================================================================
// server.cfg Specific Operations (Space-delimited argument tokens & gamemodes)
// ============================================================================

/**
 * <summary>Retrieves the count of space-delimited tokens/arguments in a directive in the active server.cfg.</summary>
 * <param name="key">Directive name (e.g. "filterscripts", "plugins").</param>
 * <returns>Number of tokens, or -1 if the directive does not exist or file is not CFG.</returns>
 */
native ConfigCfgGetTokenCount(const key[]);

/**
 * <summary>Retrieves a specific space-delimited token/argument from a directive in the active server.cfg.</summary>
 * <param name="key">Directive name (e.g. "filterscripts", "plugins").</param>
 * <param name="token_index">0-based token index.</param>
 * <param name="dest">Output string buffer.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false on failure or out of bounds.</returns>
 */
native bool:ConfigCfgGetToken(const key[], token_index, dest[], max_len = sizeof(dest));

/**
 * <summary>Retrieves the number of gamemode directives (gamemode0, gamemode1, ...) in the active server.cfg.</summary>
 * <returns>Count of gamemodes defined in server.cfg, or -1 if not CFG.</returns>
 */
native ConfigCfgGetGamemodeCount();

/**
 * <summary>Retrieves a gamemode script name from the active server.cfg by index.</summary>
 * <param name="index">0-based gamemode index (e.g. 0 for gamemode0).</param>
 * <param name="dest">Output string buffer for gamemode script name.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false on failure or out of bounds.</returns>
 */
native bool:ConfigCfgGetGamemode(index, dest[], max_len = sizeof(dest));

/**
 * <summary>Retrieves the count of space-delimited tokens in a directive for a specific loaded server.cfg.</summary>
 * <param name="file">ConfigFile identifier (e.g. CONFIG_FILE_CFG).</param>
 * <param name="key">Directive name.</param>
 * <returns>Number of tokens, or -1 if the directive does not exist or file is not CFG.</returns>
 */
native ConfigFileCfgGetTokenCount(ConfigFile:file, const key[]);

/**
 * <summary>Retrieves a specific space-delimited token from a directive for a specific loaded server.cfg.</summary>
 * <param name="file">ConfigFile identifier (e.g. CONFIG_FILE_CFG).</param>
 * <param name="key">Directive name.</param>
 * <param name="token_index">0-based token index.</param>
 * <param name="dest">Output string buffer.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false on failure or out of bounds.</returns>
 */
native bool:ConfigFileCfgGetToken(ConfigFile:file, const key[], token_index, dest[], max_len = sizeof(dest));

/**
 * <summary>Retrieves the number of gamemode directives for a specific loaded server.cfg.</summary>
 * <param name="file">ConfigFile identifier (e.g. CONFIG_FILE_CFG).</param>
 * <returns>Count of gamemodes, or -1 if not CFG.</returns>
 */
native ConfigFileCfgGetGamemodeCount(ConfigFile:file);

/**
 * <summary>Retrieves a gamemode script name by index for a specific loaded server.cfg.</summary>
 * <param name="file">ConfigFile identifier (e.g. CONFIG_FILE_CFG).</param>
 * <param name="index">0-based gamemode index.</param>
 * <param name="dest">Output string buffer for gamemode script name.</param>
 * <param name="max_len">Maximum buffer capacity.</param>
 * <returns>true if found, false on failure or out of bounds.</returns>
 */
native bool:ConfigFileCfgGetGamemode(ConfigFile:file, index, dest[], max_len = sizeof(dest));
