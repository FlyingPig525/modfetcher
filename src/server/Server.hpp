#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

#include "matjson.hpp"
#include "Geode/loader/Mod.hpp"
#include "Geode/loader/ModSettingsManager.hpp"

#ifndef SERVER_IP
#define SERVER_IP "http://localhost:8080"
#define CREATE_IP "http://localhost:8080/create"
#define SAVE_IP   "http://localhost:8080/save"
#define LOAD_IP   "http://localhost:8080/load"
#endif


struct Mod {
    std::string modId;
    std::string version;
    matjson::Value config;

    geode::Result<> setModConfig() {
        auto mod = geode::Loader::get()->getLoadedMod(modId);
        auto manager = geode::ModSettingsManager::from(mod);
        return manager->load(this->config);
    }

    static Mod fromMod(geode::Mod *mod) {
        auto manager = geode::ModSettingsManager::from(mod);
        return Mod{
            mod->getID(),
            mod->getVersion().toVString(true),
            manager->getSaveData()
        };
    }
};

struct Iteration {
    int16_t iteration;
    int64_t epochSeconds;
};

struct User {
    std::vector<::Mod> mods;
    Iteration iteration;
};


template <>
struct matjson::Serialize<Iteration> {
    static Result<Iteration> fromJson(Value const& value) {
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
struct matjson::Serialize<::Mod> {
    static Result<::Mod> fromJson(Value const& value) {
        GEODE_UNWRAP_INTO(std::string modId, value["modId"].asString());
        GEODE_UNWRAP_INTO(std::string version, value["version"].asString());
        Value config = value["config"];
        return Ok(::Mod{ modId, version, config });
    }

    static Value toJson(::Mod const& mod) {
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
        GEODE_UNWRAP_INTO(std::vector<::Mod> mods, value["mods"].as<std::vector<::Mod>>());
        GEODE_UNWRAP_INTO(Iteration iteration, value["iteration"].as<Iteration>());
        return geode::Ok(User{ mods, iteration });
    }

    static Value toJson(User const& user) {
        auto value = Value();
        value["mods"] = Serialize<std::vector<::Mod>>::toJson(user.mods);
        value["iteration"] = matjson::Serialize<Iteration>::toJson(user.iteration);
        return value;
    }
};

class ServerRequest {
public:
    web::WebRequest m_request;
    EventListener<web::WebTask> m_listener;

    ServerRequest() {
        m_request = web::WebRequest();;
        m_request.userAgent("Mod Fetcher (FlyingPig525)");
        auto manager = GJAccountManager::get();
        if (manager == nullptr) {
            log::error("GJAccountManager::get() returned nullptr!!");
            return;
        }
        auto username = manager->m_username;
        auto mod = geode::Mod::get<>();
        auto pass = mod->getSettingValue<std::string>("server-password");
        if (pass == "") {
            pass = manager->getShaPassword(manager->m_password);
            mod->setSettingValue("server-password", pass);
            // ReSharper disable once CppNoDiscardExpression
            mod->saveData();
        }
        authorization(username, pass);
    }
protected:
    void authorization(std::string user, std::string pass) {
        auto basic = base64::encode(fmt::format("{}:{}", user, pass));
        m_request.header("Authorization", fmt::format("Basic {}", basic));
    }
};

class CreateRequest : public ServerRequest {
public:
    CreateRequest() { }

    web::WebTask send() {
        return m_request.post(CREATE_IP);
    }
};

class SaveRequest : public ServerRequest {
public:
    SaveRequest() { }

    web::WebTask send(std::vector<::Mod> mods) {
        auto json = matjson::Serialize<std::vector<::Mod>>::toJson(mods);
        m_request.bodyJSON(json);
        return m_request.post(SAVE_IP);
    }
};

class LoadRequest : public ServerRequest {
public:
    LoadRequest() { }

    web::WebTask send() {
        return m_request.get(LOAD_IP);
    }
};

#endif //SERVER_HPP