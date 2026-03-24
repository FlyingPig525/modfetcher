#ifndef MODFRAME_HPP
#define MODFRAME_HPP
#include "../server/Server.hpp"
#include "Geode/ui/GeodeUI.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/SimpleAxisLayout.hpp"

class ModFrame : public cocos2d::CCNode {
protected:
    SavedMod *m_mod = nullptr;
    NineSlice *m_bg = nullptr;
    CCNode *m_content = nullptr;
    CCNode *m_info = nullptr;
    CCLabelBMFont *m_label = nullptr;
    CCLabelBMFont *m_version = nullptr;
    CCNode *m_menuNode = nullptr;
    CCMenu *m_installMenu = nullptr;
    CCMenuItemToggler *m_installToggler = nullptr;
    CCMenu *m_configMenu = nullptr;
    CCMenuItemToggler *m_configToggler = nullptr;

    bool init() {
        if (!CCNode::init()) return false;
        this->setAnchorPoint(CCPoint{0.5f, 0.5f});
        this->setContentSize(CCSize{120.0f, 120.0f});

        m_bg = NineSlice::create("GJ_square02.png");
        m_bg->setAnchorPoint(CCPoint{0.5f, 0.5f});
        m_bg->setContentSize(CCSize{120.0f, 120.0f});
        this->addChildAtPosition(m_bg, Anchor::Center);

        m_content = CCNode::create();
        m_content->setAnchorPoint(CCPoint{0.5f, 0.5f});
        m_content->setContentSize(CCSize{120.0f, 110.0f});
        m_content->setID("content");
        m_content->setLayout(SimpleColumnLayout::create()
            ->setMainAxisDirection(AxisDirection::TopToBottom)
            ->setMainAxisAlignment(MainAxisAlignment::Around)
            ->setMainAxisScaling(AxisScaling::ScaleDown)
            ->setCrossAxisAlignment(CrossAxisAlignment::Center)
            ->setCrossAxisScaling(AxisScaling::ScaleDown)
            ->setGap(0.0f)
        );

        auto logo = createServerModLogo(m_mod->modId);
        logo->setID("mod-logo");
        m_content->addChild(logo);

        m_label = CCLabelBMFont::create(m_mod->modId.c_str(), "bigFont.fnt");
        m_label->setID("mod-label");
        m_label->setScale(0.4f);

        m_version = CCLabelBMFont::create(m_mod->version.c_str(), "bigFont.fnt");
        m_version->setID("mod-version");
        m_version->setScale(0.4f);
        if (m_mod->installed && m_mod->sameOrBetterVersion) {
            m_version->setColor(ccColor3B{120, 255, 105});
        } else if (m_mod->installed) {
            m_version->setColor(ccColor3B{255, 247, 105});
        } else {
            m_version->setColor(ccColor3B{255, 106, 106});
        }

        m_info = CCNode::create();
        m_info->setID("info");
        m_info->setLayout(
            SimpleColumnLayout::create()
                ->setMainAxisDirection(AxisDirection::TopToBottom)
                ->setMainAxisScaling(AxisScaling::Fit)
                ->setMainAxisAlignment(MainAxisAlignment::Start)
                ->setCrossAxisAlignment(CrossAxisAlignment::Center)
                ->setCrossAxisScaling(AxisScaling::ScaleDown)
                ->setGap(-2.0f)
        );
        m_info->setContentWidth(110.0f);
        m_info->addChild(m_label);
        m_info->addChild(m_version);
        m_info->updateLayout();
        m_content->addChild(m_info);

        m_menuNode = CCNode::create();
        m_menuNode->setLayout(SimpleRowLayout::create()
            ->setMainAxisAlignment(MainAxisAlignment::Around)
            ->setMainAxisDirection(AxisDirection::LeftToRight)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisAlignment(CrossAxisAlignment::Start)
            ->setCrossAxisScaling(AxisScaling::Fit)
            ->setGap(2.0f)
        );

        m_installMenu = CCMenu::create();
        m_installMenu->setLayout(SimpleColumnLayout::create()
            ->setMainAxisAlignment(MainAxisAlignment::Start)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setMainAxisDirection(AxisDirection::TopToBottom)
            ->setCrossAxisAlignment(CrossAxisAlignment::Center)
            ->setCrossAxisScaling(AxisScaling::Fit)
        );
        m_installMenu->setContentSize(CCSize{0.0f, 0.0f});
        m_installMenu->setScale(0.775);

        auto installText = CCLabelBMFont::create("Install", "bigFont.fnt");
        m_installMenu->addChild(installText);

        m_installToggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ModFrame::toggleInstall), 1.0f);
        m_installToggler->setID("install-toggle");
        m_installToggler->toggle(m_mod->toInstall);
        m_installToggler->setScale(1.4f);

        m_installMenu->addChild(m_installToggler);
        m_installMenu->updateLayout();

        m_menuNode->addChild(m_installMenu);

        m_configMenu = CCMenu::create();
        m_configMenu->setLayout(SimpleColumnLayout::create()
            ->setMainAxisAlignment(MainAxisAlignment::Start)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setMainAxisDirection(AxisDirection::TopToBottom)
            ->setCrossAxisAlignment(CrossAxisAlignment::Center)
            ->setCrossAxisScaling(AxisScaling::Fit)
        );
        m_configMenu->setContentSize(CCSize{0.0f, 0.0f});
        m_configMenu->setScale(0.775f);

        auto configText = CCLabelBMFont::create("Config", "bigFont.fnt");
        m_configMenu->addChild(configText);

        m_configToggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ModFrame::toggleConfig), 1.0f);
        m_configToggler->setID("config-toggle");
        m_configToggler->toggle(m_mod->syncConfig);
        m_configToggler->setScale(1.4f);
        m_configMenu->addChild(m_configToggler);

        m_configMenu->updateLayout();

        m_menuNode->addChild(m_configMenu);
        m_menuNode->updateLayout();
        m_menuNode->setScale(0.5f);

        m_content->addChild(m_menuNode);
        m_content->updateLayout();

        this->addChildAtPosition(m_content, Anchor::Center);
        return true;
    }

    void toggleInstall(CCObject *caller) {
        m_mod->toInstall = !m_mod->toInstall;
    }

    void toggleConfig(CCObject *caller) {
        m_mod->syncConfig = !m_mod->syncConfig;
    }
public:
    static ModFrame *create(SavedMod *mod) {
        auto ret = new ModFrame();
        ret->m_mod = mod;
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

#endif //MODFRAME_HPP
