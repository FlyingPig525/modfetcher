#ifndef CONFIGS_HPP
#define CONFIGS_HPP

#include <Geode/Geode.hpp>
#include <Geode/loader/ModSettingsManager.hpp>

#include "server/Server.hpp"
#include "Defines.hpp"

using namespace geode;

$on_game(ModsLoaded) {
    log::info("Setting configs");
    auto mod = geode::Mod::get<>();
    // check completion status
    if (mod->hasSavedValue(STATUS_ID)) {
        auto syncStatus = mod->getSavedValue<bool>(STATUS_ID);
        mod->setSavedValue(STATUS_ID, true);
        if (!syncStatus) {
            Mod::get()->setSavedValue("configs-to-sync", matjson::makeObject({}));
            createQuickPopup(
                "Fatal Error!",
                "Something <cr>interrupted</c> the last load performed!\n\n<cc>Config sync will not be performed.</c>",
                "Restore Backup", "Ok",
                [] (FLAlertLayer *layer, bool btn2) {
                    if (btn2) return;
                    auto restoreRes = restoreBackup(dirs::getTempDir() / BACKUP_PATH);
                    if (restoreRes.isErr()) {
                        log::error("Error restoring backup!");
                        auto alert = FLAlertLayer::create("Backup Error!", "There was an <cr>error</c> restoring the backup!", "Ok");
                        alert->show();
                    }
                }
            );
            return;
        }
    }
    auto configs = mod->getSavedValue<matjson::Value>("configs-to-sync");
    log::debug("{}", configs.dump());

    if (configs.isNull()) return;
    bool needRestart = false;

    for (auto const& cfg : configs) {
        log::debug("{}", cfg.dump());
        auto optKey = cfg.getKey();
        if (!optKey.has_value()) {
            log::error("Error while setting configs: Config key has no value");
            continue;
        }

        auto key = optKey.value();
        auto cfgMod = geode::Loader::get()->getInstalledMod(key);
        if (cfgMod == nullptr) {
            log::error("Error while setting configs: Mod {} is nullptr", key);
            continue;
        }

        auto serverMod = ::SavedMod::fromMod(cfgMod);
        serverMod.config = cfg;
        auto res = serverMod.trySetModConfig();
        if (res.isErr()) {
            log::error("Error while setting configs: {}", res.err().value());
        }

        auto settingsManager = ModSettingsManager::from(cfgMod);
        for (auto obj : cfg) {
            auto settingKey = obj.getKey();
            if (!settingKey.has_value()) continue;

            auto setting = settingsManager->get(settingKey.value());
            if (setting == nullptr) continue;
            if (setting->requiresRestart()) needRestart = true;
        }
    }
    mod->setSavedValue("configs-to-sync", matjson::Value::parse("null").unwrap());

    if (needRestart) {
        geode::createQuickPopup(
            "Restart",
            "One or more mods require an additional restart to work as intended.",
            "Restart Later",
            "Restart",
            [](auto alert, bool restart) {
                if (restart) {
                    game::restart(true);
                }
            }
        );
    }
}


#endif //CONFIGS_HPP
