struct CurrentLayerInfo {
    int m_id;
    std::string m_name;
    std::function<void(int layer, const char* name)> m_updateCallback;
};

class SetNamePopup : public Popup {
private:
    const float m_width = 260.f;
    const float m_height = 100.f;

    CurrentLayerInfo m_layerInfo;

protected:

    bool init(CurrentLayerInfo layerInfo) {
        if (!Popup::init(m_width, m_height))
            return false;

        m_layerInfo = layerInfo;
        m_closeBtn->setVisible(false);
        setTitle(fmt::format("Name For Layer: {}", layerInfo.m_id));

        auto menu = CCMenu::create();
        menu->setContentSize(m_mainLayer->getContentSize());
        m_mainLayer->addChildAtPosition(menu, Anchor::Center);

        auto infoBtn = InfoAlertButton::create("Set Name Help", "Enter a name that will be assigned to the layer", 0.75);
        menu->addChildAtPosition(infoBtn, Anchor::TopRight, ccp(-18, -18));

        auto okBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("ok", "goldFont.fnt", "GJ_button_01.png", 1),this, menu_selector(SetNamePopup::onClose));
        menu->addChildAtPosition(okBtn, Anchor::Bottom);

        auto nameInput = TextInput::create((m_width - 40), "(empty)");
        nameInput->setString(layerInfo.m_name, false);
        nameInput->setCommonFilter(CommonFilter::Any);
        nameInput->setCallback([this] (const std::string& str) {
            m_layerInfo.m_updateCallback(m_layerInfo.m_id, str.c_str());
        });
        m_mainLayer->addChild(nameInput);
        nameInput->setPosition({m_width / 2, m_height - 55});

        setID("set-name-popup"_spr);

        return true;
    }

public:
    static SetNamePopup* create(CurrentLayerInfo layer) {
        auto ret = new SetNamePopup();
        if (ret && ret->init(layer)) {
            ret->autorelease();
            return ret; 
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
