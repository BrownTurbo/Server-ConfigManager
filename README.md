# Server-ConfigManager

---
## Features

- **Independent Format Engines**:
  - **JSON Engine (`JsonConfigDocument`)**:
    - Dedicated engine for `config.json` and `.json` documents.
    - Full support for nested dot-notation paths (`network.port`, `rcon.password`, `logging.log_chat`).
    - Supports C/C++ style comments (`//` and `/* */`) in JSON files via `nlohmann/json`.
    - Structured array handling for lists (`pawn.filterscripts`, `pawn.main_scripts`).
  - **CFG Engine (`CfgConfigDocument`)**:
    - Dedicated engine for line-oriented SA-MP `server.cfg` and `.cfg` documents.
    - Preserves custom formatting, blank lines, ordering, and comments (`#` and `//`) upon write-back.
    - Safe comment parsing that prevents truncating URLs (e.g. `weburl https://open.mp` is never stripped).
    - Full support for duplicate directive keys (such as `gamemode0`, `gamemode1`, etc.) as well as space-delimited array lists.
- **Multi-File Architecture & Active File Concept**:
  - Automatically discovers and loads `config.json` and/or `server.cfg` upon server initialization.
  - Set and switch the active file dynamically at runtime via `ConfigSetActiveFile` / `ConfigGetActiveFile`.
  - Perform operations on the active file (`ConfigGet*`, `ConfigSet*`, `Config*Array*`) or target any specific file directly without switching context (`ConfigFileGet*`, `ConfigFileSet*`, `ConfigFile*Array*`).
  - Open and manage custom configuration files at runtime (e.g. `ConfigLoadFile("database.json")`).
- **Bidirectional Directive Aliasing**:
  - Translates legacy SA-MP directives (`hostname`, `maxplayers`, `port`, `rcon_password`) to open.mp equivalents (`name`, `max_players`, `network.port`, `rcon.password`) and vice-versa seamlessly.
- **Atomic Disk Persistence**:
  - Staged in-memory changes are flushed safely (`.tmp` write + atomic OS replace) to prevent configuration file corruption on server crashes or power failures.

---

## Example Gamemode Usage

```pawn
#include <open.mp>
#include <server_config>

public OnGameModeInit() {
    new serverName[128], rconPass[64], fsName[64], activeName[64];

    // Check which configuration file is currently active and its format
    new ConfigFile:activeFile = ConfigGetActiveFile();
    ConfigGetFileName(activeFile, activeName);
    new ConfigFormat:format = ConfigGetFormat(activeFile);
    printf("[Config] Active config ID: %d (%s), Format: %s", _:activeFile, activeName,
        format == CONFIG_FORMAT_JSON ? "JSON" : format == CONFIG_FORMAT_CFG ? "server.cfg" : "Unknown");

    // Read common settings from the active file
    ConfigGetString("name", serverName);
    ConfigGetString("rcon.password", rconPass);
    new maxPlayers = ConfigGetInt("max_players", 50);
    new port = ConfigGetInt("network.port", 7777);
    new Float:streamRadius = ConfigGetFloat("network.stream_radius", 200.0);
    new bool:lanMode = ConfigGetBool("network.lan_mode", false);

    printf("[Config] Server: %s | Port: %d | Max Players: %d | Radius: %.1f", serverName, port, maxPlayers, streamRadius);

    // If active file is JSON, query structured array:
    if (format == CONFIG_FORMAT_JSON) {
        new count = ConfigGetArrayLength("pawn.filterscripts");
        for (new i = 0; i < count; i++) {
            if (ConfigGetArrayString("pawn.filterscripts", i, fsName)) {
                printf("  [JSON] Filterscript [%d]: %s", i, fsName);
            }
        }
    }

    // Inspect server.cfg directly using dedicated token and gamemode readers
    if (ConfigHasFile(CONFIG_FILE_CFG)) {
        new tokenCount = ConfigFileCfgGetTokenCount(CONFIG_FILE_CFG, "filterscripts");
        for (new i = 0; i < tokenCount; i++) {
            if (ConfigFileCfgGetToken(CONFIG_FILE_CFG, "filterscripts", i, fsName)) {
                printf("  [CFG] Filterscript token [%d]: %s", i, fsName);
            }
        }

        new gmCount = ConfigFileCfgGetGamemodeCount(CONFIG_FILE_CFG);
        new gmName[64];
        for (new i = 0; i < gmCount; i++) {
            if (ConfigFileCfgGetGamemode(CONFIG_FILE_CFG, i, gmName)) {
                printf("  [CFG] Gamemode [%d]: %s", i, gmName);
            }
        }
    }

    // Dynamic runtime update: Change hostname and stream radius
    ConfigSetString("name", "open.mp High-Performance Server");
    ConfigSetFloat("network.stream_radius", 300.0);

    // Update server.cfg directly using the CONFIG_FILE_CFG identifier if loaded
    if (ConfigHasFile(CONFIG_FILE_CFG)) {
        ConfigFileSetString(CONFIG_FILE_CFG, "hostname", "open.mp High-Performance Server");
    }

    // Flush all staged changes atomically to disk
    if (ConfigIsDirty()) {
        if (ConfigSaveAll()) {
            print("[Config] All modified configuration files successfully saved to disk.");
        }
    }

    // Load a custom secondary configuration file (returns a new ConfigFile identifier)
    new ConfigFile:dbConfig = ConfigLoadFile("database.json");
    if (dbConfig != CONFIG_FILE_INVALID) {
        new dbHost[64];
        ConfigFileGetString(dbConfig, "mysql.host", dbHost);
        new dbPort = ConfigFileGetInt(dbConfig, "mysql.port", 3306);
        printf("[Config] Database Host: %s, Port: %d", dbHost, dbPort);

        // Optionally unload when done
        // ConfigCloseFile(dbConfig);
    }

    return 1;
}
```
