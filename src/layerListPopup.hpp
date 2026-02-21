struct LayersInfo {
    std::unordered_map<int, int> m_layersToInclude;
    std::unordered_map<int, std::string> *m_layerNames;
    int m_currentLayer;
    std::function<void(int layer, const char* name)> m_updateCallback;
};


class LayerListPopup : public Popup {
private:
    const float m_width = 380.f;
    const float m_height = 280.f;

    LayersInfo m_layersInfo;

protected:
    bool init(LayersInfo layerInfo) {

        if (!Popup::init(m_width, m_height))
            return false;

        m_layersInfo = layerInfo;
        // m_closeBtn->setVisible(false);
        setTitle("Used Editor Layers");

        auto menu = CCMenu::create();
        menu->setContentSize(m_mainLayer->getContentSize());
        m_mainLayer->addChildAtPosition(menu, Anchor::Center);

        auto infoBtn = InfoAlertButton::create("Help", 
            "This is a list of <cl>named layers</c> and <cl>unnamed layers with objects</c>.\n"
            "Use the <cy>lock</c> button to lock/unlock the layer.\n"
            "Use the <cy>plus</c> button to change layer name.\n"
            "Use the <cy>go to layer</c> button to jump to that layer.\n"
            "<cg>You can also change the name of the layer by CLICKING ON "
            "ITS TEXT directly in the editor!</c>", 0.75);
        menu->addChildAtPosition(infoBtn, Anchor::TopRight, ccp(-18, -18));

        setupScrollLayer();
        setID("layer-list-popup"_spr);
        return true;
    }


    void setupScrollLayer() {
        const float cellHeight = 25;
        const float cellWidth = m_width - 40;

        auto scroll = ScrollLayer::create({m_width - 40, m_height - 55});
        m_mainLayer->addChild(scroll);
        scroll->setPosition({20,20});

        std::vector<std::pair<int, int>> layers(m_layersInfo.m_layersToInclude.begin(), m_layersInfo.m_layersToInclude.end());
        std::sort(layers.begin(), layers.end(), [](std::pair<int, int> a, std::pair<int, int> b){return a.first < b.first;});

        auto editor = LevelEditorLayer::get();
        bool isLockingEnabled = editor->m_layerLockingEnabled;

        int btnCount = 0;
        for (auto [layer, objCount] : layers) {
            auto cell = CCLayerColor::create(btnCount % 2 ? ccc4(161,88,44,255) : ccc4(194,114,62,255), cellWidth, cellHeight);

            auto indexLab = CCLabelBMFont::create(fmt::format("{}.", layer).c_str(), "bigFont.fnt");
            indexLab->limitLabelWidth(25, 0.5, 0);
            indexLab->setAnchorPoint({0,0.5});
            cell->addChildAtPosition(indexLab, Anchor::Left, ccp(10, 0));

            auto it = m_layersInfo.m_layerNames->find(layer);
            auto name = (it != m_layersInfo.m_layerNames->end()) ? it->second : std::string("");
            auto nameLab = CCLabelBMFont::create((name == "") ? "-" : name.c_str(), "bigFont.fnt");
            nameLab->setAnchorPoint({0,0.5});
            nameLab->limitLabelWidth(150, 0.5, 0);
            cell->addChildAtPosition(nameLab, Anchor::Left, ccp(45, 0));

            auto menu = CCMenu::create();
            cell->addChild(menu);
            menu->setContentSize(cell->getContentSize());
            menu->setPosition(cell->getContentSize() / 2.f);

            auto gotoSpr = CCSprite::createWithSpriteFrameName("GJ_goToLayerBtn_001.png");
            gotoSpr->setScale(0.63);
            auto gotoBtn = CCMenuItemSpriteExtra::create(gotoSpr, this, menu_selector(LayerListPopup::onGoToLayerButton));
            gotoBtn->setTag(layer);
            menu->addChildAtPosition(gotoBtn, Anchor::Right, ccp(-25, 0));

            auto plusSpr = CCSprite::createWithSpriteFrameName("GJ_plus2Btn_001.png");
            plusSpr->setScale(0.73);
            auto plusBtn = CCMenuItemSpriteExtra::create(plusSpr, this, menu_selector(LayerListPopup::onPlusButton));
            plusBtn->setTag(layer);
            plusBtn->setUserObject(nameLab);
            menu->addChildAtPosition(plusBtn, Anchor::Right, ccp(-51, 0));

            if (isLockingEnabled) {
                auto lockBtn = CCMenuItemToggler::createWithSize("GJ_lockGray_001.png", "GJ_lock_001.png", this, menu_selector(LayerListPopup::onLockButton), 0.55f);
                lockBtn->setTag(layer);
                static_cast<CCSprite*>(lockBtn->m_offButton->getNormalImage())->setOpacity(90);
                menu->addChildAtPosition(lockBtn, Anchor::Right, ccp(-73, 0));
                lockBtn->toggle(editor->isLayerLocked(layer));
            }

            auto countLab = CCLabelBMFont::create(fmt::format("Obj: {}", objCount).c_str(), "chatFont.fnt");
            countLab->setAnchorPoint({0,0.5});
            countLab->limitLabelWidth(40, 0.6, 0);
            countLab->setColor(ccc3(86,48,14));
            cell->addChildAtPosition(countLab, Anchor::Right, ccp(-135, 0));

            scroll->m_contentLayer->addChild(cell);
            btnCount++;
        }

        scroll->m_contentLayer->setContentHeight(std::max(cellHeight * btnCount, scroll->getContentHeight()));
        scroll->m_contentLayer->setLayout(ColumnLayout::create()->setAutoScale(false)->setAxisReverse(true)->setGap(0)->setCrossAxisLineAlignment(AxisAlignment::Start)->setAxisAlignment(AxisAlignment::End));
        scroll->scrollToTop();
        
        if (cellHeight * btnCount > scroll->getContentHeight()) {
            auto bar = Scrollbar::create(scroll);
            bar->setPosition(scroll->getPosition() + scroll->getContentSize() + ccp(3,0));
            bar->setAnchorPoint({0,1});
            bar->setScaleX(1.15);
            m_mainLayer->addChild(bar, 5);
        }

        auto border = ListBorders::create();
        border->setSpriteFrames("GJ_commentTop_001.png", "GJ_commentSide_001.png");
        scroll->addChild(border, 3);
        border->setContentSize(scroll->getContentSize());
        border->setPosition(scroll->getContentSize() / 2);
    }


    void onGoToLayerButton(CCObject* sender) {
        // in a strange way...
        auto arrowBtn = static_cast<CCMenuItemSpriteExtra*>(EditorUI::get()->getChildByID("layer-menu")->getChildByID("prev-layer-button"));
        LevelEditorLayer::get()->m_currentLayer = sender->getTag() + 1;
        (arrowBtn->m_pListener->*(arrowBtn->m_pfnSelector))(arrowBtn); // call the selector
        onClose(nullptr);
    }


    void onPlusButton(CCObject* sender) {
        auto lab = static_cast<CCLabelBMFont*>(static_cast<CCNode*>(sender)->getUserObject());
        int layer = sender->getTag();
        auto it = m_layersInfo.m_layerNames->find(layer);
        auto name = (it != m_layersInfo.m_layerNames->end()) ? it->second : std::string("");
        SetNamePopup::create({
            layer, name,
            [this, lab] (int layer, const char* name) {
                if (layer == -1) return;
                m_layersInfo.m_updateCallback(layer, name);
                lab->setString((*name == '\0') ? "-" : name);
                lab->limitLabelWidth(150, 0.5, 0);
            }
        })->show();
    }


    void onLockButton(CCObject* sender) {
        auto editor = LevelEditorLayer::get();
        int layer = sender->getTag();
        if (editor->m_currentLayer == layer) {
            EditorUI::get()->onLockLayer(nullptr);
        } else {
            if (layer < 10000) {
                editor->m_lockedLayers[layer] = !editor->m_lockedLayers[layer];
            }
        }
    }


public:
    static LayerListPopup* create(LayersInfo layer) {
        auto ret = new LayerListPopup();
        if (ret && ret->init(layer)) {
            ret->autorelease();
            return ret; 
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }


    void onClose(CCObject* sender) override {
        Popup::onClose(sender);
    }
};
    