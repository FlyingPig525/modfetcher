#ifndef CONFIGS_HPP
#define CONFIGS_HPP

#include <Geode/Geode.hpp>
#include <Geode/loader/ModSettingsManager.hpp>

using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>
struct SetConfigs : Modify<SetConfigs, MenuLayer> {
    bool init() override {
        if (!MenuLayer::init()) return false;
        if (!Mod::get<>()->getSettingValue<bool>("set-configs")) return true;
        log::info("Setting configs");
        auto mod = geode::Mod::get<>();
        auto configs = mod->getSavedValue<matjson::Value>("configs-to-sync");
        log::debug("{}", configs.dump());
        if (configs.isNull()) return true;
        for (auto cfg : configs) {
            log::debug("{}", cfg.dump());
            auto optKey = cfg.getKey();
            if (!optKey.has_value()) {
                log::error("Error while setting configs: Config key has no value");
                continue;
            }
            auto key = optKey.value();
            auto cfgMod = Loader::get()->getInstalledMod(key);
            if (cfgMod == nullptr) {
                log::error("Error while setting configs: Mod {} is nullptr", key);
                continue;
            }
            const auto _ = ModSettingsManager::from(cfgMod)->load(cfg);
        }
        mod->setSavedValue("configs-to-sync", matjson::Value::parse("null").unwrap());
        return true;
    }
};

#endif //CONFIGS_HPP
