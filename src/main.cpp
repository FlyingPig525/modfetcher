#include <Geode/Geode.hpp>
#include <Geode/utils/base64.hpp>


#include <Geode/utils/coro.hpp>
#include "Geode/ui/GeodeUI.hpp"
#include "argon/argon.hpp"
#include "server/Server.hpp"
#include "ui/ModFrame.hpp"
#include "ui/ModTogglePopup.hpp"

using namespace geode::prelude;

#ifdef DEBUG
#define FLUSH log::flush()
#else
#define FLUSH ;
#endif

#include <Geode/modify/AccountLayer.hpp>
struct AccountMenu : Modify<AccountMenu, AccountLayer> {
    struct Fields {
        CCMenuItemSpriteExtra* m_modSaveButton = nullptr;
        CCMenuItemSpriteExtra* m_modLoadButton = nullptr;

        CreateRequest m_createRequest;
        SaveRequest m_saveRequest;
        LoadRequest m_loadRequest;

        bool m_requestInProgress = false;
    };

    void customSetup() override {
        AccountLayer::customSetup();
        changeLayout();
    }

    void changeLayout() {
        const auto sprite = ButtonSprite::create("Save Mods");
        const auto btn = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(AccountMenu::onSave)
        );
        btn->setID("save-mods");
        replaceWithRow(m_backupButton, btn, "backup-row");
        m_fields->m_modSaveButton = btn;
        const auto loadSprite = ButtonSprite::create("Load Mods");
        const auto loadBtn = CCMenuItemSpriteExtra::create(
            loadSprite,
            this,
            menu_selector(AccountMenu::onLoad)
        );
        loadBtn->setID("load-mods");
        replaceWithRow(m_syncButton, loadBtn, "sync-row");
        // m_fields->m_modLoadButton = loadBtn;
        // m_fields->mod = new SavedMod{
        //     .modId = "geode.devtools",
        //     .version = "v1.2.3",
        //     .installed = false,
        //     .toInstall = true
        // };
        // this->addChildAtPosition(ModFrame::create(m_fields->mod), Anchor::Center);
    }

    CCMenu* replaceWithRow(CCNode* existing, CCNode* second, std::string&& id) {
        auto menu = existing->getParent();
        if (menu == nullptr) {
            log::error("menu is nullptr");
            return nullptr;
        }
        auto node = CCMenu::create();
        node->setID(id);
        node->setPosition(existing->getPosition());
        node->setAnchorPoint(CCPoint(0.5f, 0.5f));
        const auto layout = SimpleAxisLayout::create(Axis::Row);
        layout->setGap(16.0f);
        layout->setMainAxisScaling(AxisScaling::Fit);
        layout->setCrossAxisScaling(AxisScaling::Fit);
        node->setLayout(layout);
        existing->removeFromParent();
        node->addChild(existing);
        node->addChild(second);
        menu->updateLayout();
        menu->addChild(node);
        node->updateLayout();
        return node;
    }

    void onLoad(CCObject* sender) {
        log::info("Mod load button clicked");
        if (m_fields->m_requestInProgress) return;
        AccountLayer::showLoadingUI();
        m_fields->m_loadRequest = LoadRequest();
        m_fields->m_loadRequest.m_callback = [this] (web::WebResponse value) {
            loadCallback(std::move(value));
        };
        m_fields->m_requestInProgress = true;
        m_fields->m_loadRequest.send();
    }

    void onSave(CCObject* sender) {
        log::info("Mod save button clicked");
        if (m_fields->m_requestInProgress) return;
        auto mods = Loader::get()->getAllMods();
        std::vector<::SavedMod> serverMods;
        for (auto mod : mods) {
            if (!mod->isLoaded()) continue;
            auto serverMod = SavedMod::fromMod(mod);
            serverMods.push_back(serverMod);

        }
        AccountLayer::showLoadingUI();
        m_fields->m_saveRequest = SaveRequest();
        m_fields->m_saveRequest.m_callback = [this] (web::WebResponse value) {
            saveCallback(std::move(value));
        };
        m_fields->m_requestInProgress = true;
        m_fields->m_saveRequest.send(serverMods);
    }

    void createAuthPopup() {
        geode::createQuickPopup(
            "Unauthorized!",
            "The request returned <cr>401 unauthorized</c>. Creating a <cy>user entry</c> might fix it.\n"
            "<co>An argon token (the authorization scheme behind globed) will be shared with the server to create a"
            " </c><cy>user entry</c>.",
            "Cancel", "Create",
            [this] (FLAlertLayer *layer, bool btn2) {
                layer->setVisible(false);
                if (btn2) {
                    AccountLayer::showLoadingUI();
                    m_fields->m_createRequest = CreateRequest();
                    m_fields->m_createRequest.m_callback = [this] (web::WebResponse value) {
                        createCallback(std::move(value));
                    };
                    m_fields->m_requestInProgress = true;
                    m_fields->m_createRequest.send();
                }
                layer->removeFromParentAndCleanup(true);
            }
        );
    }

    void saveCallback(web::WebResponse value) {
        if (value.cancelled()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            auto alert = FLAlertLayer::create(
                "Error!",
                "Something went wrong while saving mods.",
                "Ok"
            );
            alert->show();
            return;
        }
        m_fields->m_requestInProgress = false;
        AccountLayer::hideLoadingUI();
        if (auto err = value.errorMessage(); err != "") {
            auto alert = FLAlertLayer::create(
                "Error!",
                err.data(),
                "Ok"
            );
            alert->show();
            return;
        }
        if (value.code() == 401) {
            createAuthPopup();
            return;
        }
        auto alert = FLAlertLayer::create(
            "Success!",
            "Successfully saved mods to the server.",
            "Ok"
        );
        alert->show();
    }

    void createCallback(web::WebResponse value) {
        if (value.cancelled()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            auto alert = FLAlertLayer::create(
                "Error!",
                "Something went wrong while creating a user entry.",
                "Ok"
            );
            alert->show();
            return;
        }
        m_fields->m_requestInProgress = false;
        AccountLayer::hideLoadingUI();
        if (auto err = value.errorMessage(); !err.empty()) {
            auto alert = FLAlertLayer::create(
                "Error!",
                err.data(),
                "Ok"
            );
            alert->show();
            return;
        }
        if (value.code() == 409) {
            auto alert = FLAlertLayer::create(
                "Error!",
                "Server returned <cr>409 conflict</c>. A user entry with this username already exists.",
                "Ok"
            );
            alert->show();
            return;
        }
        auto alert = geode::createQuickPopup(
            "Success!",
            "Successfully created a user entry on the server.",
            "Ok", "Save",
            [this] (FLAlertLayer *layer, bool btn2) {
                layer->setVisible(false);
                if (btn2) {
                    onSave(layer->m_button2);
                }
                layer->removeFromParentAndCleanup(true);
            }
        );
        alert->show();
    }

    void loadCallback(web::WebResponse value) {
        if (value.cancelled()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            auto alert = FLAlertLayer::create(
                "Error!",
                "Something went wrong while loading",
                "Ok"
            );
            alert->show();
            return;
        }
        m_fields->m_requestInProgress = false;
        AccountLayer::hideLoadingUI();
        if (auto err = value.errorMessage(); !err.empty()) {
            auto alert = FLAlertLayer::create(
                "Error!",
                err.data(),
                "Ok"
            );
            alert->show();
            return;
        }
        if (value.code() == 401) {
            createAuthPopup();
            return;
        }
        auto res = value.json();
        if (res.isErr()) {
            auto alert = FLAlertLayer::create(
                "Error!",
                fmt::format(
                    "Load (json) failed with error: {}\nData: <cr>{}</c>",
                    res.err().value_or("unknown"),
                    value.string().unwrapOr("unknown")
                ),
                "Ok"
            );
            alert->show();
            return;
        }
        auto const& json = res.unwrap();
        auto uRes = matjson::Serialize<User>::fromJson(json);
        if (uRes.isErr()) {
            auto alert = FLAlertLayer::create(
                "Error!",
                fmt::format("Load (user) failed with error: {}", uRes.err().value_or("unknown")),
                "Ok"
            );
            alert->show();
            return;
        }
        auto user = uRes.unwrap();
        std::vector<::SavedMod> modsToFetch;
        for (auto& mod : user.mods) {
            if (Loader::get()->isModInstalled(mod.modId)) {
                mod.installed = true;
                log::debug("mod installed {}", mod.modId);
                auto inst = Loader::get()->getInstalledMod(mod.modId);
                auto instVersion = inst->getVersion();
                auto modVersionRes = VersionInfo::parse(mod.version);
                if (modVersionRes.isErr()) {
                    auto err = modVersionRes.err();
                    log::error(
                        "Mod version parse error: {}. Mod: {}, version: {}",
                        err.value_or("no message"),
                        mod.modId,
                        mod.version
                    );
                    auto alert = FLAlertLayer::create(
                        "Error!",
                        fmt::format(
                            "An error occurred: Mod version parse error: {}. Mod: {}, version {}",
                            err.value_or("no message"),
                            mod.modId,
                            mod.version
                        ),
                        "Ok"
                    );
                    alert->show();
                    return;
                }
                auto modVersion = modVersionRes.ok().value();
                log::debug("server: {} inst: {}", modVersion.toVString(), instVersion.toVString());
                if (instVersion >= modVersion) {
                    mod.sameOrBetterVersion = true;
                    mod.toInstall = false;
                } else {
                    mod.sameOrBetterVersion = false;
                    mod.toInstall = true;
                }
            } else {
                mod.installed = false;
                mod.sameOrBetterVersion = false;
                mod.toInstall = true;
            }
            modsToFetch.push_back(mod);
        }
        auto alert = ModTogglePopup::create(modsToFetch, [this] (std::vector<SavedMod> vec) {
            log::debug("Spawn download");
            AccountMenu::showLoadingUI();
            async::spawn(downloadMods(std::move(vec)));
        });
        alert->show();
    }

    arc::Future<> downloadMods(std::vector<::SavedMod> mods) {
        m_fields->m_requestInProgress = true;
        log::debug("downloading mods");
        FLUSH;
        log::debug("{}", mods.size());
        bool errored = false;
        int downloaded = 0;
        matjson::Value configs = matjson::makeObject({});
        for (auto mod : mods) {
            if (mod.syncConfig) {
                configs.set(mod.modId, mod.config);
                log::debug("Added config for geode mod {} to sync next restart", mod.modId);
            }

            if (!mod.toInstall) continue;
            log::debug("{} {}", mod.modId, mod.version);
            auto req = ModDownloadRequest(mod.modId, mod.version);
            log::debug("awaiting info req");
            auto resp = co_await req.sendAsync();
            // structured off ModDownload::Impl::onFinished
            if (!resp.ok()) {
                if (resp.code() == -1) {
                    log::error("Failed to make request for {}. e: {}", mod.modId, resp.string().unwrapOr("?"));
                } else {
                    log::error("Server error {}. e: {}", resp.code(), resp.string().unwrapOr("?"));
                }
                log::error("Request error: {}", resp.errorMessage());
                errored = true;
                continue;
            }

            // check if already installed (replace)
            if (auto modPtr = Loader::get()->getInstalledMod(mod.modId)) {
                std::error_code errorCode;
                std::filesystem::remove(modPtr->getPackagePath(), errorCode);
                if (errorCode) {
                    log::error("Unable to delete existing geode file {}", errorCode);
                    errored = true;
                    continue;
                }
            }

            auto filePath = dirs::getModsDir() / (mod.modId + ".geode");
            auto data = resp.data();
            auto ok = file::writeBinary(filePath, data);
            if (!ok) {
                log::error("{}", ok.unwrapErr());
                errored = true;
                continue;
            }

            auto meta = ModMetadata::createFromGeodeFile(filePath);
            if (meta.hasErrors()) {
                log::error("Written metadata has errors: {}", fmt::join(meta.getErrors(), " & "));
                errored = true;
                continue;
            }
            log::debug("Successfully downloaded geode mod {}", mod.modId);
            downloaded++;
        }

        Mod::get()->setSavedValue("configs-to-sync", configs);

        m_fields->m_requestInProgress = false;
        queueInMainThread([this, errored, downloaded] {
            AccountLayer::hideLoadingUI();
            if (errored) {
                auto alert = FLAlertLayer::create("Error!", "One or more <cr>errors</c> has occurred!", "Ok");
                alert->show();
                return;
            }
            if (downloaded == 0) {
                FLAlertLayer::create("Done!", "No mods have been installed", "Ok");
                return;
            }
            geode::createQuickPopup(
                "Done!",
                fmt::format("<cy>{}</c> mods have been <cg>installed</c>", downloaded),
                "Restart Later",
                "Restart",
                [] (FLAlertLayer *layer, bool btn2) {
                    layer->setVisible(false);
                    if (btn2) {
                        utils::game::restart(true);
                    }
                    layer->removeFromParentAndCleanup(true);
                }
            );
        });
        co_return;
    };
};