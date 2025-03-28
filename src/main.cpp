#include <Geode/Geode.hpp>
#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <unordered_map>
#include <matjson.hpp>
#include <matjson/std.hpp>

using namespace geode::prelude;

#include "setNamePopup.hpp"
#include "layerListPopup.hpp"

// keybinds
#ifdef GEODE_IS_DESKTOP
	#include <geode.custom-keybinds/include/Keybinds.hpp>
	$execute {
		keybinds::BindManager::get()->registerBindable({
			"open-list"_spr, "Open layer list",
			"Open the list of the used editor layers",
			{ keybinds::Keybind::create(KEY_G, keybinds::Modifier::Control) },
			"Named Editor Layers"
		});
	}
#endif // GEODE_IS_DESKTOP


class $modify(MyEditorUI, EditorUI) {
	struct Fields {

		int levelId = 0;
		std::unordered_map<int, std::string> layerNames;
		Ref<CCLabelBMFont> layerNameLabel;
		Ref<CCMenu> layerMenu;

		void loadMap() {
			if (auto res = matjson::parse(Mod::get()->getSavedValue<std::string>(std::to_string(levelId), "{}"))) {
				for (auto& [key, value] : *res) {
					if (value.isString()) layerNames.insert({std::atoi(key.c_str()), *value.asString()});
				}
			}
		}

		~Fields() {
			matjson::Value jsonVal;
			for (const auto& [key, value] : layerNames) {
				jsonVal[std::to_string(key)] = value;
			}
			Mod::get()->setSavedValue(std::to_string(levelId), jsonVal.dump(matjson::NO_INDENTATION));
		}
	};

	static void onModify(auto& self) {
		// to work with BetterEdit UI scale
        (void) self.setHookPriorityAfterPost("EditorUI::init", "hjfod.betteredit");
    }

	void setupLayerMenu() {
		auto menu = CCMenu::create();
		auto layerMenu = getChildByID("layer-menu");
		menu->setID("menu"_spr);
		menu->setContentSize({95,18});
		menu->setAnchorPoint({1,1});
		addChild(menu, 6);
		menu->setPosition(layerMenu->getPosition() + ccp(0,-2) + ccp(layerMenu->getScaledContentWidth() * (1 - layerMenu->getAnchorPoint().x), -layerMenu->getScaledContentHeight() * layerMenu->getAnchorPoint().y));
		menu->setScale(getChildByID("editor-buttons-menu")->getScale());
		
		auto btnSpr = CCSprite::createWithSpriteFrameName("GJ_plus2Btn_001.png");
		btnSpr->setScale(0.7);
		auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(MyEditorUI::onLayerListButton));
		menu->addChild(btn);

		auto label = CCLabelBMFont::create("   ", "bigFont.fnt");
		label->setScale(0.1);
		auto textBtn = CCMenuItemSpriteExtra::create(label, this, menu_selector(MyEditorUI::onTextClick));
		menu->addChild(textBtn);

		menu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::End)->setAxisReverse(true)->setAutoScale(false));

		m_fields->layerNameLabel = label;
		m_fields->layerMenu = menu;
	}

	void updateLabel(const char* text) {
		auto lab = m_fields->layerNameLabel;
		lab->setString(text);
		float availableSpace = m_fields->layerMenu->getContentWidth() - 20;
		float takenSpace = std::max(lab->getContentWidth(), 1.f);
		lab->setScale(std::min(0.5f, availableSpace / takenSpace));
		auto parent = static_cast<CCMenuItemSpriteExtra*>(lab->getParent());
		parent->updateSprite();
		parent->setPositionX(availableSpace - parent->getScaledContentWidth() / 2.f);
	}

	void checkLayer(float) {
		static int layer = -500;
		if (m_editorLayer->m_currentLayer == layer) return;
		// layer changed
		layer = m_editorLayer->m_currentLayer;
		updateLayer(layer);
	}

	void updateLayer(int layer) {
		if (layer == -1) { // all
			updateLabel("");
		} else {
			auto it = m_fields->layerNames.find(layer);
			if (it != m_fields->layerNames.end()) {
				updateLabel((*it).second.c_str());
			} else {
				updateLabel(" - ");
			}
		}
	}

	void freeUpSomeSpace() {
		auto bigMenu = getChildByID("editor-buttons-menu");
		auto topRight = bigMenu->getPosition() + ccp(bigMenu->getScaledContentWidth() * (1 - bigMenu->getAnchorPoint().x), bigMenu->getScaledContentHeight() * (1 - bigMenu->getAnchorPoint().y));
		float bottomOld = bigMenu->getPositionY() - bigMenu->getScaledContentHeight() * bigMenu->getAnchorPoint().y;
		bigMenu->setScale(.88f);
		bigMenu->setPosition(topRight - ccp(bigMenu->getScaledContentWidth() * (1 - bigMenu->getAnchorPoint().x), bigMenu->getScaledContentHeight() * (1 - bigMenu->getAnchorPoint().y)));
		float bottomNew = bigMenu->getPositionY() - bigMenu->getScaledContentHeight() * bigMenu->getAnchorPoint().y;
		float yDiff = bottomNew - bottomOld;

		auto smallMenu = getChildByID("layer-menu");
		auto right = smallMenu->getPosition() + ccp(smallMenu->getScaledContentWidth() * (1 - bigMenu->getAnchorPoint().x), 0);
		smallMenu->setScale(.88f);
		smallMenu->setPosition(right - ccp(smallMenu->getScaledContentWidth() * (1 - bigMenu->getAnchorPoint().x), -yDiff));

		auto lockSpr = getChildByID("layer-locked-sprite");
		lockSpr->setPosition(lockSpr->getPosition() + ccp(2, yDiff));
	}

	void initKeybinds() {
		#ifdef GEODE_IS_DESKTOP
			this->template addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
				if (event->isDown()) {
					if (CCScene::get()->getChildByID("set-name-popup"_spr) == nullptr && CCScene::get()->getChildByID("layer-list-popup"_spr) == nullptr) {
						onLayerListButton(nullptr);
					}
				}
				return ListenerResult::Propagate;
			}, "open-list"_spr);
		#endif // GEODE_IS_DESKTOP
	}

	bool init(LevelEditorLayer* editor) {
		if (!EditorUI::init(editor)) {
			return false;
		}
		m_fields->levelId = EditorIDs::getID(editor->m_level);
		m_fields->loadMap();
		
		if (getChildByID("editor-buttons-menu")->getScale() > 0.85) {
			freeUpSomeSpace();
		}
		
		setupLayerMenu();
		initKeybinds();
		
		updateLayer(editor->m_currentLayer);
		schedule(schedule_selector(MyEditorUI::checkLayer), 0);

		return true;
	}

	void showUI(bool b) {
		EditorUI::showUI(b);
		m_fields->layerMenu->setVisible(b);
	}

	void nameUpdated(int layer, const char* name) {
		if (layer == -1) return;
		if (*name == '\0') {
			m_fields->layerNames.erase(layer);
			if (m_editorLayer->m_currentLayer == layer) {
				updateLabel(" - ");
			}
		} else {
			m_fields->layerNames.insert_or_assign(layer, name);
			if (m_editorLayer->m_currentLayer == layer) {
				updateLabel(name);
			}
		}
	}

	void onLayerListButton(CCObject*) {
		auto editor = LevelEditorLayer::get();
		auto allObjects = editor->m_objects;
		std::unordered_map<int, int> layerCountMap;
		// layers with objects
		for (auto* obj : CCArrayExt<GameObject*>(allObjects)) {
			auto it = layerCountMap.find(obj->m_editorLayer);
			if (it != layerCountMap.end()) (*it).second++;
			else layerCountMap.insert({obj->m_editorLayer, 1});

			if (obj->m_editorLayer2 != obj->m_editorLayer && obj->m_editorLayer2 > 0) {
				it = layerCountMap.find(obj->m_editorLayer2);
				if (it != layerCountMap.end()) (*it).second++;
				else layerCountMap.insert({obj->m_editorLayer2, 1});						
			}
		}
		// layers that are named
		for (auto const &layer : m_fields->layerNames) {
			layerCountMap.insert({layer.first, 0});
		}
		// current layer
		layerCountMap.insert({std::max((int)editor->m_currentLayer, 0), 0});
		
		LayerListPopup::create({
			std::move(layerCountMap),
			&m_fields->layerNames,
			m_editorLayer->m_currentLayer,
			[this] (int layer, const char* name) {nameUpdated(layer, name);}
		})->show();
	}

	void onTextClick(CCObject*) {
		int layer = m_editorLayer->m_currentLayer;
		if (layer == -1) return;
		auto it = m_fields->layerNames.find(layer);
		auto name = (it != m_fields->layerNames.end()) ? (*it).second : std::string("");
		SetNamePopup::create({layer, name,
			[this] (int layer, const char* name) {nameUpdated(layer, name);}
		})->show();
	}
};

// made in 1 day