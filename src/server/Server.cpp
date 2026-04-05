#include "Server.hpp"
#include <Geode/binding/GJAccountManager.hpp>
#include "../Defines.hpp"

std::string token;

$on_mod(Loaded) {
    async::spawn(
        argon::startAuth(),
        [](Result<std::string> res) {
            if (res.isOk()) {
                token = std::move(res).unwrap();
                log::debug("Token: {}", token);
            } else {
                log::warn("Auth failed: {}", res.unwrapErr());
            }
        }
    );
}

ServerRequest::ServerRequest() {
    m_request = web::WebRequest();;
    m_request.userAgent("Mod Fetcher (FlyingPig525)");
    auto manager = GJAccountManager::get();
    if (manager == nullptr) {
        log::error("GJAccountManager::get() returned nullptr!!");
        return;
    }
    auto id = manager->m_accountID;
    authorization(utils::numToString(id), token);
}

ModDownloadRequest::ModDownloadRequest(std::string modId, std::string modVersion) : m_modId(std::move(modId)), m_modVersion(std::move(modVersion)) {
    m_request = web::WebRequest();
    m_request.userAgent("Mod Fetcher (FlyingPig525)");
}

void ModDownloadRequest::send() {
    m_listener.spawn(
        sendAsync(),
        m_callback
    );

}

using namespace std::chrono_literals;
WebFunc ModDownloadRequest::sendAsync() {
    auto ver = utils::string::filter(m_modVersion, "1234567890.");
    auto resp = co_await m_request.timeout(15s).get(
        fmt::format(
            MOD_INFO_IP"/{}/versions/{}/download?gd={}&platforms={}",
            m_modId,
            ver,
            Loader::get()->getGameVersion(),
            GEODE_PLATFORM_SHORT_IDENTIFIER
        )
    );

    co_return resp;
}



void CreateRequest::send() {
    m_listener.spawn(sendAsync(), m_callback);
}

WebFunc CreateRequest::sendAsync() {
    auto resp = co_await m_request.timeout(15s).post(CREATE_IP);
    co_return resp;
}


void SaveRequest::send(std::vector<SavedMod> const& mods) {
    m_listener.spawn(sendAsync(mods), m_callback);
}

WebFunc SaveRequest::sendAsync(std::vector<SavedMod> mods) {
    auto json = matjson::Serialize<std::vector<::SavedMod>>::toJson(mods);
    m_request.bodyJSON(json);
    auto resp = co_await m_request.timeout(15s).post(SAVE_IP);
    co_return resp;
}


void LoadRequest::send() {
    m_listener.spawn(sendAsync(), m_callback);
}

WebFunc LoadRequest::sendAsync() {
    auto resp = co_await m_request.timeout(15s).get(LOAD_IP);
    co_return resp;
}
