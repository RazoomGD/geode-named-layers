struct LayersInfoReduced {
    const char* title;
    std::unordered_map<int, std::string> *m_layerNames;
    int m_currentLayer;
    std::function<void(int layer)> m_updateCallback;
};


class SelectPopup : public Popup {
private:
    const float m_width = 300.f;
    const float m_height = 280.f;

    LayersInfoReduced m_layersInfo;

protected:

    bool init(LayersInfoReduced layerInfo) {

        if (!Popup::init(m_width, m_height)) 
            return false;

        m_layersInfo = layerInfo;
        setTitle(layerInfo.title);

        auto menu = CCMenu::create();
        menu->setContentSize(m_mainLayer->getContentSize());
        m_mainLayer->addChildAtPosition(menu, Anchor::Center);

        auto infoBtn = InfoAlertButton::create("Help", "The list of named layers. Unnamed layers aren't here", 0.75);
        menu->addChildAtPosition(infoBtn, Anchor::TopRight, ccp(-18, -18));

        setupScrollLayer();
        setID("simple-select-popup"_spr);
        
        return true;
    }


    void setupScrollLayer() {
        const float cellHeight = 25;
        const float cellWidth = m_width - 40;

        auto scroll = ScrollLayer::create({m_width - 40, m_height - 55});
        m_mainLayer->addChild(scroll);
        scroll->setPosition({20,20});

        std::vector<std::pair<int, std::string>> layers(m_layersInfo.m_layerNames->begin(), m_layersInfo.m_layerNames->end());
        if (!m_layersInfo.m_layerNames->contains(m_layersInfo.m_currentLayer)) {
            layers.push_back({m_layersInfo.m_currentLayer, "-"});
        }
        std::sort(layers.begin(), layers.end(), [](std::pair<int, std::string> a, std::pair<int, std::string> b){return a.first < b.first;});

        int btnCount = 0;
        for (auto [layer, name] : layers) {
            auto cell = CCLayerColor::create(btnCount % 2 ? ccc4(161,88,44,255) : ccc4(194,114,62,255), cellWidth, cellHeight);

            auto indexLab = CCLabelBMFont::create(fmt::format("{}.", layer).c_str(), "bigFont.fnt");
            indexLab->limitLabelWidth(25, 0.5, 0);
            indexLab->setAnchorPoint({0,0.5});
            cell->addChildAtPosition(indexLab, Anchor::Left, ccp(10, 0));

            auto nameLab = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
            nameLab->setAnchorPoint({0,0.5});
            nameLab->limitLabelWidth(120, 0.5, 0);
            cell->addChildAtPosition(nameLab, Anchor::Left, ccp(45, 0));

            if (layer == m_layersInfo.m_currentLayer) {
                nameLab->setColor(ccc3(255,150,0)); 
                indexLab->setColor(ccc3(255,150,0));
            }

            auto menu = CCMenu::create();
            cell->addChild(menu);
            menu->setContentSize(cell->getContentSize());
            menu->setPosition(cell->getContentSize() / 2.f);

            auto selectSpr = ButtonSprite::create("set", "bigFont.fnt", "GJ_button_01.png", 0.8);
            selectSpr->setScale(0.5);
            auto selectBtn = CCMenuItemSpriteExtra::create(selectSpr, this, menu_selector(SelectPopup::onSelectButton));
            selectBtn->setTag(layer);
            menu->addChildAtPosition(selectBtn, Anchor::Right, ccp(-25, 0));

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
        border->setPosition(scroll->getContentSize() / 2);
        border->setContentSize(scroll->getContentSize());
    }


    void onSelectButton(CCObject* sender) {
        int layer = sender->getTag();
        m_layersInfo.m_updateCallback(layer);
        onClose(nullptr);
    }

public:
    static SelectPopup* create(LayersInfoReduced layer) {
        auto ret = new SelectPopup();
        if (ret && ret->init(layer)) {
            ret->autorelease();
            return ret; 
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
    