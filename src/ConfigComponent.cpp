#include "ConfigComponent.hpp"
#include "Natives.hpp"
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>
#include <pawn-natives/NativesMain.hpp>

ConfigComponent::~ConfigComponent() {
    if (pawn_) {
        pawn_->getEventDispatcher().removeEventHandler(this);
    }
}

void ConfigComponent::onLoad(ICore* core) {
    core_ = core;

    ConfigManager::Instance().SetLogger([this](const std::string& msg) {
        if (core_) {
            core_->printLn("%s", msg.c_str());
        }
    });

    ConfigManager::Instance().Log("=================================================");
    ConfigManager::Instance().Log("  Server-ConfigManager Component by Zorono       ");
    ConfigManager::Instance().Log("=================================================");

    ConfigManager::Instance().InitializeDefaultFiles();
}

void ConfigComponent::onInit(IComponentList* components) {
    pawn_ = components->queryComponent<IPawnComponent>();
    if (pawn_) {
        setAmxFunctions(pawn_->getAmxFunctions());
        pawn_->getEventDispatcher().addEventHandler(this);
    }
}

void ConfigComponent::onReady() {
    ConfigManager::Instance().Log("[ConfigManager] Server-ConfigManager is ready.");
}

void ConfigComponent::onFree(IComponent* component) {
    if (component == pawn_) {
        pawn_ = nullptr;
        setAmxFunctions();
    }
}

void ConfigComponent::free() {
    ConfigManager::Instance().Log("[ConfigManager] Flushing any unsaved changes to disk on unload...");
    ConfigManager::Instance().SaveAll();
    delete this;
}

void ConfigComponent::reset() {
    ConfigManager::Instance().SaveAll();
}

void ConfigComponent::onAmxLoad(IPawnScript& script) {
    AMX* amx = script.GetAMX();
    if (amx) {
        pawn_natives::AmxLoad(amx);
    }
}

void ConfigComponent::onAmxUnload(IPawnScript& script) { /* */ }

COMPONENT_ENTRY_POINT() {
    return new ConfigComponent();
}
