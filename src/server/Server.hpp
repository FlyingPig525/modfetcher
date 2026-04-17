#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <Geode/utils/base64.hpp>
#include <matjson.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/ModSettingsManager.hpp>

#include "argon/argon.hpp"
#include "Geode/utils/coro.hpp"



using namespace geode::prelude;

struct SavedMod {
    std::string modId;
    std::string version;
    matjson::Value config;
    matjson::Value data;
    bool installed = false;
    bool sameOrBetterVersion = false;
    bool toInstall = !sameOrBetterVersion;
    bool syncConfig = false;

    Result<> trySetModConfig() {
        auto mod = Loader::get()->getLoadedMod(modId);
        auto manager = ModSettingsManager::from(mod);
        mod->getSaveContainer() = data;
        return manager->load(this->config);
    }

    static SavedMod fromMod(geode::Mod *mod) {
        auto manager = ModSettingsManager::from(mod);
        log::debug("{}", mod->getSaveContainer().dump());
        return SavedMod{
            .modId = mod->getID(),
            .version = mod->getVersion().toVString(true),
            .config = manager->getSaveData(),
            .data = mod->getSaveContainer(),
            .installed = true,
            .sameOrBetterVersion = true
        };
    }
};

struct User {
    std::vector<::SavedMod> mods;
};

template <>
struct matjson::Serialize<::SavedMod> {
    static Result<::SavedMod> fromJson(Value const& value) {
        GEODE_UNWRAP_INTO(std::string modId, value["modId"].asString());
        GEODE_UNWRAP_INTO(std::string version, value["version"].asString());
        Value config = value["config"];
        Value data = value["data"];
        return Ok(::SavedMod{ modId, version, config, data });
    }

    static Value toJson(::SavedMod const& mod) {
        auto value = Value();
        value["modId"] = mod.modId;
        value["version"] = mod.version;
        value["config"] = mod.config;
        value["data"] = mod.data;
        return value;
    }
};

template <>
struct matjson::Serialize<User> {
    static geode::Result<User> fromJson(Value const& value) {
        GEODE_UNWRAP_INTO(std::vector<::SavedMod> mods, value["mods"].as<std::vector<::SavedMod>>());
        return geode::Ok(User{ mods });
    }

    static Value toJson(User const& user) {
        auto value = Value();
        value["mods"] = Serialize<std::vector<::SavedMod>>::toJson(user.mods);
        return value;
    }
};

using WebFunc = arc::Future<web::WebResponse>;

class ServerRequest {
public:
    web::WebRequest m_request;
    std::function<void(web::WebResponse)> m_callback;
    async::TaskHolder<web::WebResponse> m_listener;

    ServerRequest();

protected:

    void authorization(std::string id, std::string token) {
        auto basic = utils::base64::encode(fmt::format("{}:{}", id, token));
        m_request.header("Authorization", fmt::format("Basic {}", basic));
    }
};

class ModDownloadRequest {
public:
    web::WebRequest m_request;
    std::function<void(web::WebResponse)> m_callback;
    async::TaskHolder<web::WebResponse> m_listener;

    ModDownloadRequest(std::string modId, std::string modVersion);

    void send();
    WebFunc sendAsync();
private:
    std::string m_modId;
    std::string m_modVersion;
};

class CreateRequest : public ServerRequest {
public:
    CreateRequest() = default;

    void send();
    WebFunc sendAsync();
};

class SaveRequest : public ServerRequest {
public:
    SaveRequest() = default;

    void send(std::vector<::SavedMod> const& mods);
    WebFunc sendAsync(std::vector<::SavedMod> mods);
};

class LoadRequest : public ServerRequest {
public:
    LoadRequest() = default;

    void send();
    WebFunc sendAsync();
};
#endif //SERVER_HPP