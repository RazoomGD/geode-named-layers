#include <Geode/Geode.hpp>
#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <razoom.save_level_data_api/include/SaveLevelDataApi.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/SetGroupIDLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/utils/general.hpp>
#include <unordered_map>
#include <matjson.hpp>
#include <matjson/std.hpp>
#include <algorithm>

using namespace geode::prelude;

#include "setNamePopup.hpp"
#include "layerListPopup.hpp"
#include "simpleSelectPopup.hpp"


class $modify(MyEditorUI, EditorUI) {
	struct Fields {
		std::unordered_map<int, std::string> layerNames;
		Ref<CCLabelBMFont> layerNameLabel;
		Ref<CCMenu> layerMenu;
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
		updateLayerText(layer);
	}


	void updateLayerText(int layer) {
		if (layer == -1) { // all
			updateLabel("");
		} else {
			auto it = m_fields->layerNames.find(layer);
			if (it != m_fields->layerNames.end()) {
				updateLabel(it->second.c_str());
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
		addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "open-list"), [this](const Keybind& keybind, bool down, bool repeat, double timestamp) {
            if (down && !repeat) {
                if (CCScene::get()->getChildByID("set-name-popup"_spr) == nullptr && 
							CCScene::get()->getChildByID("layer-list-popup"_spr)== nullptr &&
							CCScene::get()->getChildByID("SetGroupIDLayer") == nullptr) {
					onLayerListButton(nullptr);
					return ListenerResult::Stop;
				}
            }
			return ListenerResult::Propagate;
        });
	}


	bool init(LevelEditorLayer* editor) {
		if (!EditorUI::init(editor))
			return false;

		auto f = m_fields.self();
		int levelId = EditorIDs::getID(editor->m_level);

		bool useObject = Mod::get()->getSettingValue<bool>("use-save-object");
		auto layers = SaveLevelDataAPI::getSavedValue(editor->m_level, "layers", true, useObject);
		if (layers.isOk() && (*layers).isObject()) {
			for (auto& [key, value] : *layers) {
				if (value.isString()) {
					f->layerNames.insert({std::atoi(key.c_str()), *value.asString()});
				}
			}
		}

		// todo: deprecated, exists only to recover old saves
		if (f->layerNames.empty()) {
			if (auto res = matjson::parse(Mod::get()->getSavedValue<std::string>(std::to_string(levelId), "{}"))) {
				for (auto& [key, value] : *res) {
					if (value.isString()) {
						f->layerNames.insert({std::atoi(key.c_str()), *value.asString()});
					}
				}
			}
		}
		
		if (getChildByID("editor-buttons-menu")->getScale() > 0.85) {
			freeUpSomeSpace();
		}
		
		setupLayerMenu();
		initKeybinds();
		
		updateLayerText(editor->m_currentLayer);
		schedule(schedule_selector(MyEditorUI::checkLayer), 0);

		return true;
	}


	void showUI(bool b) {
		EditorUI::showUI(b);
		m_fields->layerMenu->setVisible(b);
	}


	void nameUpdated(int layer, const char* name) {
		if (layer == -1) return;
		if (name == nullptr || *name == '\0') {
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
			if (it != layerCountMap.end()) it->second++;
			else layerCountMap.insert({obj->m_editorLayer, 1});

			if (obj->m_editorLayer2 != obj->m_editorLayer && obj->m_editorLayer2 > 0) {
				it = layerCountMap.find(obj->m_editorLayer2);
				if (it != layerCountMap.end()) it->second++;
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
		auto name = (it != m_fields->layerNames.end()) ? it->second : std::string();
		SetNamePopup::create({layer, name,
			[this] (int layer, const char* name) {nameUpdated(layer, name);}
		})->show();
	}
};



class $modify(MySetGroupIDLayer, SetGroupIDLayer) {
	struct Fields {
		int layer1 = -990;
		int layer2 = -990;
		Ref<CCLabelBMFont> lab1;
		Ref<CCLabelBMFont> lab2;

		bool isBetterEdit = false;
		struct {
			TextInput* inputL1{};
			TextInput* inputL2{};
			std::vector<GameObject*> objects;
			CCNode* unmix1{};
			CCNode* unmix2{};
		} betterEdit;
	};


	static void onModify(auto& self) {
		// after all Better Edit changes
        (void) self.setHookPriorityAfterPost("SetGroupIDLayer::init", "hjfod.betteredit");
    }


	void initKeybinds() {
		addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "change-layer-1"), [this](const Keybind& keybind, bool down, bool repeat, double timestamp) {
            if (down && !repeat) {
                if (CCScene::get()->getChildByID("simple-select-popup"_spr) == nullptr && CCScene::get()->getChildByID("SetGroupIDLayer") != nullptr) {
					onL1Click(nullptr);
					return ListenerResult::Stop;
				}
            }
			return ListenerResult::Propagate;
        });
		addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "change-layer-2"), [this](const Keybind& keybind, bool down, bool repeat, double timestamp) {
            if (down && !repeat) {
                if (CCScene::get()->getChildByID("simple-select-popup"_spr) == nullptr && CCScene::get()->getChildByID("SetGroupIDLayer") != nullptr) {
					onL2Click(nullptr);
					return ListenerResult::Stop;
				}
            }
			return ListenerResult::Propagate;
        });
	}
	

	bool init(GameObject* obj, CCArray* objects) {
		if (!SetGroupIDLayer::init(obj, objects)) return false;

		if (!Mod::get()->getSettingValue<bool>("in-edit-groups-menu")) {
			return true;
		}

		auto menuL1 = m_mainLayer->getChildByID("editor-layer-menu");
		auto menuL2 = m_mainLayer->getChildByID("editor-layer-2-menu");
		if (!menuL1 || !menuL2) return true;

		if (menuL1->getChildByID("hjfod.betteredit/unmix-button")) {
			m_fields->isBetterEdit = true;
			m_fields->betterEdit.inputL1 = menuL1->getChildByType<TextInput>(0);
			m_fields->betterEdit.inputL2 = menuL2->getChildByType<TextInput>(0);
			m_fields->betterEdit.unmix1 = menuL1->getChildByID("hjfod.betteredit/unmix-button");
			m_fields->betterEdit.unmix2 = menuL2->getChildByID("hjfod.betteredit/unmix-button");
			if (obj) {
				m_fields->betterEdit.objects.push_back(obj);
			} else {
				for (auto obj : CCArrayExt<GameObject*>(objects)) {
					m_fields->betterEdit.objects.push_back(obj);
				}
			}
		}

		// best idea how to fix overlapping
		if (Loader::get()->isModLoaded("spaghettdev.named-editor-groups")) {
			if (auto menu = m_mainLayer->getChildByID("add-group-id-menu")) {
				if (auto label = menu->getChildByID("add-group-id-label")) {
					label->setVisible(false);
				}
			}
		}

		// menuL1->addChildAtPosition()
		m_fields->lab1 = CCLabelBMFont::create("   ", "bigFont.fnt");
		auto textBtn = CCMenuItemSpriteExtra::create(m_fields->lab1, this, menu_selector(MySetGroupIDLayer::onL1Click));
		menuL1->addChildAtPosition(textBtn, Anchor::Bottom, ccp(0,-1));
		textBtn->setID("layer-1-label"_spr);
		
		m_fields->lab2 = CCLabelBMFont::create("   ", "bigFont.fnt");
		textBtn = CCMenuItemSpriteExtra::create(m_fields->lab2, this, menu_selector(MySetGroupIDLayer::onL2Click));
		menuL2->addChildAtPosition(textBtn, Anchor::Bottom, ccp(0,-1));
		textBtn->setID("layer-2-label"_spr);

		checkLayers(0);
		schedule(schedule_selector(MySetGroupIDLayer::checkLayers), 0);

		initKeybinds();

		return true;
	}


	int getL1Value() {
		if (!m_fields->isBetterEdit) {
			return m_editorLayerValue;
		}
		// with BetterEdit
		auto str = m_fields->betterEdit.inputL1->getString();
		for (int i = 0; i < str.size(); i++) {
			if (str[i] < '0' || str[i] > '9') return -1;
		}
		return utils::numFromString<int>(str).unwrapOr(-1);
	}
	
	
	int getL2Value() {
		if (!m_fields->isBetterEdit) {
			return m_editorLayer2Value;
		}
		// with BetterEdit
		auto str = m_fields->betterEdit.inputL2->getString();
		for (int i = 0; i < str.size(); i++) {
			if (str[i] < '0' || str[i] > '9') return -1;
		}
		return utils::numFromString<int>(str).unwrapOr(-1);
	}


	void setL1Value(int value) {
		if (!m_fields->isBetterEdit) {
			return onArrow(5, value - m_editorLayerValue);
		}
		// with BetterEdit
		for (auto* obj : m_fields->betterEdit.objects) {
			obj->m_editorLayer = value;
		}
		m_fields->betterEdit.inputL1->setString(std::to_string(value));
	}


	void setL2Value(int value) {
		if (!m_fields->isBetterEdit) {
			return onArrow(6, value - m_editorLayer2Value);
		}
		// with BetterEdit
		for (auto* obj : m_fields->betterEdit.objects) {
			obj->m_editorLayer2 = value;
		}
		m_fields->betterEdit.inputL2->setString(std::to_string(value));
	}


	void checkLayers(float) {
		if (getL1Value() != m_fields->layer1) {
			// L1 changed
			updateLabelText(m_fields->lab1, getL1Value());
			m_fields->layer1 = getL1Value();
		}
		if (getL2Value() != m_fields->layer2) {
			// L2 changed
			updateLabelText(m_fields->lab2, getL2Value());
			m_fields->layer2 = getL2Value();
		}
	}


	void updateLabelText(CCLabelBMFont* lab, int layer) {
		const char* text = " - ";
		if (layer != -1) {
			auto editor = reinterpret_cast<MyEditorUI*>(EditorUI::get());
			auto it = editor->m_fields->layerNames.find(layer);
			if (it != editor->m_fields->layerNames.end()) {
				text = it->second.c_str();
			}
		} else if (m_fields->isBetterEdit) {
			text = "";
		}

		// update label
		lab->setString(text);
		float availableSpace = 90;
		float takenSpace = std::max(lab->getContentWidth(), 1.f);
		lab->setScale(std::min(0.5f, availableSpace / takenSpace));
		auto parent = static_cast<CCMenuItemSpriteExtra*>(lab->getParent());
		parent->updateSprite();
		// parent->setPositionX(availableSpace - parent->getScaledContentWidth() / 2.f);

	}


	void onL1Click(CCObject*) {
		SelectPopup::create({
			"Select Layer 1",
			&reinterpret_cast<MyEditorUI*>(EditorUI::get())->m_fields->layerNames, 
			getL1Value(),
			[this](int layer) {
				setL1Value(layer);
			}
		})->show();
	}

	
	void onL2Click(CCObject*) {
		SelectPopup::create({
			"Select Layer 2",
			&reinterpret_cast<MyEditorUI*>(EditorUI::get())->m_fields->layerNames, 
			getL2Value(),
			[this](int layer) {
				setL2Value(layer);
			}
		})->show();
	}


	void onClose(CCObject* sender) {
		unschedule(schedule_selector(MySetGroupIDLayer::checkLayers));
		SetGroupIDLayer::onClose(sender);
		EditorUI::get()->updateButtons();
	}
};


class $modify(GJGameLevel) {
	// copy layer names with level info
	void copyLevelInfo(GJGameLevel* oldLvl) {
		GJGameLevel::copyLevelInfo(oldLvl);
		auto newLvl = this;
		int newId = EditorIDs::getID(newLvl);
		int oldId = EditorIDs::getID(oldLvl);

		auto config = Mod::get()->getSavedValue<std::string>(std::to_string(oldId));
		if (!config.empty()) {
			Mod::get()->setSavedValue(std::to_string(newId), config);
		}
	}
};


class $modify(EditorPauseLayer) {
	void saveLevel() {
		matjson::Value jsonVal;
		auto editor = reinterpret_cast<MyEditorUI*>(EditorUI::get());
		for (const auto& [key, value] : editor->m_fields->layerNames) {
			jsonVal[std::to_string(key)] = value;
		}
		bool useObject = Mod::get()->getSettingValue<bool>("use-save-object");
		SaveLevelDataAPI::setSavedValue(editor->m_editorLayer->m_level, "layers", jsonVal, true, useObject);
		EditorPauseLayer::saveLevel();
	}
};


