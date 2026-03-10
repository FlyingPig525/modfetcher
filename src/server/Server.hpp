#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <Geode/utils/base64.hpp>
#include <matjson.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/ModSettingsManager.hpp>

#include "argon/argon.hpp"
#include "Geode/utils/coro.hpp"

#define SERVER_IP "http://localhost:8080"
#define CREATE_IP "http://localhost:8080/create"
#define SAVE_IP   "http://localhost:8080/save"
#define LOAD_IP   "http://localhost:8080/load"
#define MOD_INFO_IP "https://api.geode-sdk.org/v1/mods"

using namespace geode::prelude;

struct SavedMod {
    std::string modId;
    std::string version;
    matjson::Value config;
    bool installed = false;
    bool sameOrBetterVersion = false;
    bool toInstall = !sameOrBetterVersion;
    bool syncConfig = false;

    Result<> trySetModConfig() {
        auto mod = Loader::get()->getLoadedMod(modId);
        auto manager = ModSettingsManager::from(mod);
        return manager->load(this->config);
    }

    static SavedMod fromMod(geode::Mod *mod) {
        auto manager = ModSettingsManager::from(mod);
        return SavedMod{
            .modId = mod->getID(),
            .version = mod->getVersion().toVString(true),
            .config = manager->getSaveData(),
            .installed = true,
            .sameOrBetterVersion = true
        };
    }
};

struct Iteration {
    int16_t iteration;
    int64_t epochSeconds;
};

struct User {
    std::vector<::SavedMod> mods;
    Iteration iteration;
};

template <>
struct matjson::Serialize<Iteration> {
    static geode::Result<Iteration> fromJson(Value const& value) {
        GEODE_UNWRAP_INTO(const int16_t iteration, value["iteration"].asInt());
        GEODE_UNWRAP_INTO(const int64_t epochSeconds, value["epochSeconds"].asInt());
        return Ok(Iteration{ iteration, epochSeconds });
    }

    static Value toJson(Iteration const& i) {
        auto value = Value();
        value["iteration"] = i.iteration;
        value["epochSeconds"] = i.epochSeconds;
        return value;
    }
};

template <>
struct matjson::Serialize<::SavedMod> {
    static Result<::SavedMod> fromJson(Value const& value) {
        GEODE_UNWRAP_INTO(std::string modId, value["modId"].asString());
        GEODE_UNWRAP_INTO(std::string version, value["version"].asString());
        Value config = value["config"];
        return Ok(::SavedMod{ modId, version, config });
    }

    static Value toJson(::SavedMod const& mod) {
        auto value = Value();
        value["modId"] = mod.modId;
        value["version"] = mod.version;
        value["config"] = mod.config;
        return value;
    }
};

template <>
struct matjson::Serialize<User> {
    static geode::Result<User> fromJson(Value const& value) {
        GEODE_UNWRAP_INTO(std::vector<::SavedMod> mods, value["mods"].as<std::vector<::SavedMod>>());
        GEODE_UNWRAP_INTO(Iteration iteration, value["iteration"].as<Iteration>());
        return geode::Ok(User{ mods, iteration });
    }

    static Value toJson(User const& user) {
        auto value = Value();
        value["mods"] = Serialize<std::vector<::SavedMod>>::toJson(user.mods);
        value["iteration"] = matjson::Serialize<Iteration>::toJson(user.iteration);
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