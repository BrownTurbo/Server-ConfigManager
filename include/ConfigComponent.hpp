#pragma once

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include "ConfigEngine.hpp"

#include "../pluginconfig.h"

class ConfigComponent final : public IComponent, public PawnEventHandler {
public:
    PROVIDE_UID(0x5345525643464731);

    ~ConfigComponent();

    StringView componentName() const override {
        return "Server-ConfigManager";
    }

    SemanticVersion componentVersion() const override {
        return SemanticVersion(PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH, PLUGIN_VERSION_BUILD);
    }

    void onLoad(ICore* core) override;
    void onInit(IComponentList* components) override;
    void onReady() override;
    void onFree(IComponent* component) override;
    void free() override;
    void reset() override;

    // PawnEventHandler lifecycle hooks
    void onAmxLoad(IPawnScript& script) override;
    void onAmxUnload(IPawnScript& script) override;

private:
    ICore* core_ = nullptr;
    IPawnComponent* pawn_ = nullptr;
};
