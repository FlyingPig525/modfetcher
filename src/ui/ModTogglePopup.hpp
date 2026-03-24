#ifndef MODTOGGLEPOPUP_HPP
#define MODTOGGLEPOPUP_HPP
#include "../server/Server.hpp"
#include "Geode/ui/Popup.hpp"


class ModTogglePopup : public geode::Popup {
protected:
    ScrollLayer *m_scroll = nullptr;
    Scrollbar *m_scrollbar = nullptr;
    std::vector<CCNode*> m_rows;
    CCMenuItemSpriteExtra *m_cancelButton = nullptr;
    CCMenuItemSpriteExtra *m_downloadButton = nullptr;

    std::vector<SavedMod> m_mods;
    bool m_syncConfigs = false;

    Function<void(std::vector<SavedMod>)> m_downloadCallback;

    bool init(std::vector<SavedMod> mods, Function<void(std::vector<SavedMod>)> downloadCallback) {
        if (!Popup::init(520.0f, 280.0f)) return false;
        m_mods = mods;
        m_downloadCallback = std::move(downloadCallback);

        this->setTitle("Mod Selection");
        m_scroll = ScrollLayer::create(CCSize{510.0f, 245.0f});
        m_scroll->setAnchorPoint(ccp(0.5, 0.55));

        m_scroll->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout());

        const auto rows = static_cast<unsigned int>(ceil(static_cast<float>(m_mods.size()) / 4.0f));
        log::debug("rows {} {} {}", rows, ((float)m_mods.size() / 4.0f), m_mods.size());
        for (unsigned int row = 0; row < rows; row++) {
            log::debug("row {}", row);
            auto rowNode = CCNode::create();
            rowNode->setID(fmt::format("row-{}", row));
            rowNode->setLayout(SimpleRowLayout::create()
                ->setMainAxisAlignment(MainAxisAlignment::Around)
                ->setMainAxisScaling(AxisScaling::Grow)
                ->setMainAxisAlignment(MainAxisAlignment::Start)
                ->setCrossAxisScaling(AxisScaling::Grow)
                ->setGap(3.0f)
            );
            for (int column = 0; column < 4; column++) {
                log::debug("column {}", column);
                if (m_mods.size() - 1 < (row * 4) + column) {
                    log::debug("vector cannot handle column. size: {}, accessing: {}", m_mods.size(), row * column);
                    break;
                };
                auto mod = &m_mods[(row * 4) + column];
                auto frame = ModFrame::create(mod);
                frame->setID(fmt::format("mod-{}", (row * 4) + column));
                rowNode->addChild(frame);
            }
            rowNode->updateLayout();
            m_scroll->m_contentLayer->addChild(rowNode);
        }
        m_scroll->m_contentLayer->updateLayout();
        m_scroll->scrollToTop();

        m_scrollbar = Scrollbar::create(m_scroll);
        m_scrollbar->setAnchorPoint(ccp(1, 0.5));
        auto indicator = m_scrollbar->getChildByIndex(1);
        indicator->setScaleY(0.45f);
        m_mainLayer->addChildAtPosition(m_scrollbar, Anchor::Right, ccp(-5, 0));

        m_mainLayer->addChildAtPosition(m_scroll, Anchor::Bottom, ccp(-m_scroll->getScaledContentWidth() / 2, 5));


        m_cancelButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("Cancel"), this, menu_selector(ModTogglePopup::cancel));
        m_cancelButton->setAnchorPoint(ccp(1, 0.5));
        m_buttonMenu->addChildAtPosition(m_cancelButton, Anchor::Bottom, ccp(-5, 0));

        m_downloadButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("Download"), this, menu_selector(ModTogglePopup::download));
        m_downloadButton->setAnchorPoint(ccp(0, 0.5));

        m_buttonMenu->addChildAtPosition(m_downloadButton, Anchor::Bottom, ccp(5, 0));

        return true;
    }

    void togglerConfigSync(CCObject *sender) {
        m_syncConfigs = !m_syncConfigs;
    }

    void cancel(CCObject *sender) {
        log::debug("Cancelled");
        onClose(sender);
        // onClose(m_closeBtn);
    }

    void download(CCObject *sender) {
        log::debug("Download");
        onClose(sender);
        for (auto mod : m_mods) {
            log::info(
                "id: {} version: {} installed: {} toInstall: {} sameVersion: {} syncConfig: {}",
                mod.modId,
                mod.version,
                mod.installed,
                mod.toInstall,
                mod.sameOrBetterVersion,
                mod.syncConfig
            );
        }
        m_downloadCallback(m_mods);
    }
public:
    static ModTogglePopup *create(std::vector<SavedMod> const& mods, Function<void(std::vector<SavedMod>)> downloadCallback) {
        auto ret = new ModTogglePopup();
        if (ret->init(mods, std::move(downloadCallback))) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};



#endif //MODTOGGLEPOPUP_HPP
