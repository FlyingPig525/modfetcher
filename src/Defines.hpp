#ifndef MODFETCHER_DEFINES_HPP
#define MODFETCHER_DEFINES_HPP

#define BACKUP_PATH "mods-backup.zip"
#define CONFIG_ID "configs-to-sync"
#define STATUS_ID "sync-complete-status"
#define DATA_ID "data-to-sync"
#define SERVER_IP Mod::get<>()->getSettingValue<std::string>("server-ip")
#define CREATE_IP SERVER_IP + "/create"
#define SAVE_IP   SERVER_IP + "/save"
#define LOAD_IP   SERVER_IP + "/load"
#define MOD_INFO_IP "https://api.geode-sdk.org/v1/mods"

static Result<file::Zip::Path> backupMods() {
    auto filePath = dirs::getTempDir() / BACKUP_PATH;
    if (std::filesystem::exists(filePath)) {
        auto deleteSuccess = std::filesystem::remove(filePath);
        if (!deleteSuccess) {
            log::error("Error deleting old file");
            return Err("Deletion unsuccessful");
        }
    }
    GEODE_UNWRAP_INTO(auto file, file::Zip::create(filePath));
    auto success = file.addAllFrom(dirs::getModsDir());
    if (success.isErr()) {
        return Err(success.unwrapErr());
    }
    return Ok(file.getPath());
}

static Result<> restoreBackup(const file::Zip::Path& file) {
    return file::Unzip::intoDir(file, dirs::getGeodeDir());
}

static FLAlertLayer *createSingleButtonQuickPopup(
    char const *title,
    std::string content,
    char const *btn,
    Function<void(FLAlertLayer*)> selected,
    bool cancelledByEscape = true
) {
    auto s = new Function(std::move(selected));
    auto alert = createQuickPopup(
        title,
        std::move(content),
        btn, "",
        [s] (FLAlertLayer *layer, bool btn2) {
            s->operator()(layer);
            delete s;
        },
        false, cancelledByEscape
    );
    alert->show();
    alert->m_button1->getParent()->setPositionX(0.0f);
    alert->m_button2->getParent()->setPositionX(10000.0f);
    return alert;
}

#endif // MODFETCHER_DEFINES_HPP
