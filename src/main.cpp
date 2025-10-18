#include <Geode/Geode.hpp>
#include <Geode/utils/base64.hpp>

#include <../../../../../Shared/Geode/sdk/loader/src/server/DownloadManager.hpp>
#include "server/Server.hpp"

using namespace geode::prelude;

#include <Geode/modify/AccountLayer.hpp>
struct AccountMenu : Modify<AccountMenu, AccountLayer> {
    struct Fields {
        CCMenuItemSpriteExtra* m_modSaveButton = nullptr;
        CCMenuItemSpriteExtra* m_modLoadButton = nullptr;

        CreateRequest m_createRequest;
        SaveRequest m_saveRequest;
        LoadRequest m_loadRequest;

        std::vector<::Mod> m_downloadingMods;
        EventListener<server::ModDownloadFilter> m_downloadListener;

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
        m_fields->m_modLoadButton = loadBtn;
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
        m_fields->m_loadRequest.m_listener.bind([this] (web::WebTask::Event *e) {
            loadCallback(e);
        });
        m_fields->m_requestInProgress = true;
        m_fields->m_loadRequest.m_listener.setFilter(m_fields->m_loadRequest.send());
    }

    void onSave(CCObject* sender) {
        log::info("Mod save button clicked");
        if (m_fields->m_requestInProgress) return;
        auto mods = Loader::get()->getAllMods();
        std::vector<::Mod> serverMods;
        for (auto mod : mods) {
            if (!mod->isEnabled()) continue;
            auto serverMod = ::Mod::fromMod(mod);
            serverMods.push_back(serverMod);

        }
        AccountLayer::showLoadingUI();
        m_fields->m_saveRequest = SaveRequest();
        m_fields->m_saveRequest.m_listener.bind([this] (web::WebTask::Event *e) {
            saveCallback(e);
        });
        m_fields->m_requestInProgress = true;
        m_fields->m_saveRequest.m_listener.setFilter(m_fields->m_saveRequest.send(serverMods));
    }

    void createPopup() {
        geode::createQuickPopup(
            "Unauthorized!",
            "The request returned <cr>401 unauthorized</c>. Creating a <cy>user entry</c> might fix it.\n"
            "<co>The password provided in the config (a non-reversible sha hash of your account password by default)"
            " will be shared with the server to create a </c><cy>user entry</c>.",
            "Cancel", "Create",
            [this] (FLAlertLayer *layer, bool btn2) {
                layer->setVisible(false);
                if (btn2) {
                    AccountLayer::showLoadingUI();
                    m_fields->m_createRequest = CreateRequest();
                    m_fields->m_createRequest.m_listener.bind([this] (web::WebTask::Event *e) {
                        createCallback(e);
                    });
                    m_fields->m_requestInProgress = true;
                    m_fields->m_createRequest.m_listener.setFilter(m_fields->m_createRequest.send());
                }
                layer->removeFromParentAndCleanup(true);
            }
        );
    }

    void saveCallback(web::WebTask::Event *e) {
        if (web::WebResponse* value = e->getValue()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            if (auto err = value->errorMessage(); err != "") {
                auto alert = FLAlertLayer::create(
                    "Error!",
                    err,
                    "Ok"
                );
                alert->show();
                return;
            }
            if (value->code() == 401) {
                createPopup();
                return;
            }
            auto alert = FLAlertLayer::create(
                "Success!",
                "Successfully saved mods to the server.",
                "Ok"
            );
            alert->show();
        } else if (web::WebProgress* progress = e->getProgress()) {
            log::info("save {}", progress->downloaded());
        } else if (e->isCancelled()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            auto alert = FLAlertLayer::create(
                "Error!",
                "Something went wrong while saving mods.",
                "Ok"
            );
            alert->show();
        }
    }

    void createCallback(web::WebTask::Event *e) {
        if (web::WebResponse* value = e->getValue()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            if (auto err = value->errorMessage(); err != "") {
                auto alert = FLAlertLayer::create(
                    "Error!",
                    err,
                    "Ok"
                );
                alert->show();
                return;
            }
            if (value->code() == 409) {
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
        } else if (web::WebProgress* progress = e->getProgress()) {
            log::info("create {}", progress->downloaded());
        } else if (e->isCancelled()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            auto alert = FLAlertLayer::create(
                "Error!",
                "Something went wrong while creating a user entry.",
                "Ok"
            );
            alert->show();
        }
    }

    void loadCallback(web::WebTask::Event *e) {
        if (web::WebResponse* value = e->getValue()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            if (auto err = value->errorMessage(); err != "") {
                auto alert = FLAlertLayer::create(
                    "Error!",
                    err,
                    "Ok"
                );
                alert->show();
                return;
            }
            if (value->code() == 401) {
                createPopup();
                return;
            }
            auto res = value->json();
            if (res.isErr()) {
                auto alert = FLAlertLayer::create(
                    "Error!",
                    fmt::format(
                        "Load (json) failed with error: {}\nData: <cr>{}</c>",
                        res.err().value_or("unknown"),
                        value->string().unwrapOr("unknown")
                    ),
                    "Ok"
                );
                alert->show();
                return;
            }
            auto json = res.unwrap();
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
            std::vector<std::string> truncatedIds;
            std::vector<::Mod> modsToFetch;
            for (auto mod : user.mods) {
                std::string color;
                if (Loader::get()->isModInstalled(mod.modId)) {
                    color = "<cg>";
                } else {
                    color =  "<cr>";
                    modsToFetch.push_back(mod);
                }
                // i dont want to deal with FLAlertLayer stuff to use scrolling, so im gonna truncate
                if (truncatedIds.size() == 9) {
                    truncatedIds.push_back(color.substr().append("...</c>"));
                } else if (truncatedIds.size() < 9) {
                    truncatedIds.push_back(mod.modId.substr().insert(0, color).append("</c>"));
                }
            }
            auto alert = geode::createQuickPopup(
                "Success!",
                fmt::format("Fetched mods (<cy>{}</c>) successfully:\n{}", user.mods.size(), fmt::join(truncatedIds, "\n")),
                "Cancel", "Load",
                [this, modsToFetch] (FLAlertLayer *layer, bool btn2) {
                    layer->setVisible(false);
                    if (btn2) {
                        log::info("Loading mods");
                        downloadMods(modsToFetch);
                    }
                    layer->removeFromParentAndCleanup(true);
                },
                false
            );
            alert->show();
        } else if (web::WebProgress* progress = e->getProgress()) {
            log::info("load {}", progress->downloaded());
        } else if (e->isCancelled()) {
            m_fields->m_requestInProgress = false;
            AccountLayer::hideLoadingUI();
            auto alert = FLAlertLayer::create(
                "Error!",
                "Something went wrong while loading",
                "Ok"
            );
            alert->show();
        }
    }

    void downloadMods(std::vector<::Mod> mods) {
        if (mods.size() == 0) return;
        AccountLayer::showLoadingUI();
        m_fields->m_requestInProgress = true;
        // makes it correctly capture `this`
        auto a = [this] (server::ModDownloadEvent *e) {
            for (int i = 0; i < m_fields->m_downloadingMods.size(); i++) {
                auto mod = m_fields->m_downloadingMods[i];
                if (mod.modId == e->id) {
                    log::info("{} finished downloading", e->id);
                    auto configs = geode::Mod::get<>()->getSavedValue<matjson::Value>("configs-to-sync");
                    if (configs == nullptr) {
                        configs = matjson::Value::object();
                    }
                    configs[e->id] = mod.config;
                    geode::Mod::get<>()->setSavedValue("configs-to-sync", configs);
                    m_fields->m_downloadingMods.erase(m_fields->m_downloadingMods.begin() + i);
                    if (m_fields->m_downloadingMods.size() == 0) {
                        AccountLayer::hideLoadingUI();
                        m_fields->m_requestInProgress = false;
                        createQuickPopup(
                            "Success!",
                            "All mods have downloaded.",
                            "Ok", "Restart",
                            [] (FLAlertLayer *layer, bool btn2) {
                                layer->setVisible(false);
                                if (btn2) {
                                    game::restart(true);
                                } else {
                                    layer->removeFromParentAndCleanup(true);
                                }
                            }
                        );
                    }
                    return ListenerResult::Stop;
                }
            }
            return ListenerResult::Propagate;
        };
        m_fields->m_downloadListener = { a };
        auto manager = server::ModDownloadManager::get();
        for (auto mod : mods) {
            log::info("Downloading {}", mod.modId);
            auto v = VersionInfo::parse(mod.version).unwrapOr(VersionInfo(0, 0, 0));
            if (v == VersionInfo(0, 0, 0)) {
                log::error("invalid version string {}", mod.version);
                continue;
            }
            m_fields->m_downloadingMods.push_back(mod);
            //                                                 replace old versions
            manager->startDownload(mod.modId, v, std::nullopt, mod.modId);
        }
    }
};