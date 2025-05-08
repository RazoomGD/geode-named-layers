#include <Geode/Geode.hpp>
#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/SetGroupIDLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <unordered_map>
#include <unordered_set>
#include <matjson.hpp>
#include <matjson/std.hpp>
#include <algorithm>

using namespace geode::prelude;

#include "setNamePopup.hpp"
#include "layerListPopup.hpp"
#include "simpleSelectPopup.hpp"


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
		keybinds::BindManager::get()->registerBindable({
			"change-layer-1"_spr, "Change layer 1",
			"When in <cy>Edit Group</c> menu, open the layer list to select L1 layer",
			{ keybinds::Keybind::create(KEY_G, keybinds::Modifier::Control) },
			"Named Editor Layers"
		});
		keybinds::BindManager::get()->registerBindable({
			"change-layer-2"_spr, "Change layer 2",
			"When in <cy>Edit Group</c> menu, open the layer list to select L2 layer",
			{ keybinds::Keybind::create(KEY_G, keybinds::Modifier::Control | keybinds::Modifier::Shift) },
			"Named Editor Layers"
		});
	}
#endif // GEODE_IS_DESKTOP



struct MyEditorUI;
struct MyEditorUI* myEditorUI;


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
					if (CCScene::get()->getChildByID("set-name-popup"_spr) == nullptr && 
								CCScene::get()->getChildByID("layer-list-popup"_spr)== nullptr &&
								CCScene::get()->getChildByID("SetGroupIDLayer") == nullptr) {
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
		myEditorUI = this;
		m_fields->levelId = EditorIDs::getID(editor->m_level);
		m_fields->loadMap();
		
		if (getChildByID("editor-buttons-menu")->getScale() > 0.85) {
			freeUpSomeSpace();
		}
		
		setupLayerMenu();
		initKeybinds();
		
		updateLayer(editor->m_currentLayer);
		schedule(schedule_selector(MyEditorUI::checkLayer), 0);

		if (Mod::get()->getSettingValue<bool>("use-save-object")) {
			handleSaveObject(2, false);
		}

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


	// action: 1 - save, 2 - load; create - if not exists; doesn't check mod settings
	void handleSaveObject(int action, bool create) {

		auto editor = LevelEditorLayer::get();
		TextGameObject* mySaveObj = nullptr;

		for (auto *obj : CCArrayExt<GameObject*>(editor->m_objects)) {
			if (obj->m_objectID != 914) continue;
			if (auto textObj = typeinfo_cast<TextGameObject*>(obj)) {
				auto text = std::string(textObj->m_text);
				if (text.starts_with("config\n"_spr)) {
					mySaveObj = textObj;
					break;
				}
			}
		}

		if (!mySaveObj) {
			if (!create) return;
			mySaveObj = static_cast<TextGameObject*>(editor->createObject(914, ccp(0,0), true));
			editor->removeObjectFromSection(mySaveObj);
			mySaveObj->updateTextObject("{}", false);
		}

		if (action == 1) { // save
			matjson::Value jsonVal;
			for (const auto& [key, value] : m_fields->layerNames) {
				jsonVal[std::to_string(key)] = value;
			}
			auto saveStr = std::string("config\n"_spr) + jsonVal.dump();
			mySaveObj->updateTextObject(saveStr, false);
			mySaveObj->setPosition(ccp(-6324,-5817));
		} 
		
		if (action == 2) { // load
			auto text = mySaveObj->m_text;
			auto len = std::strlen("config\n"_spr);
			if (text.size() > len) {
				if (auto res = matjson::parse(text.substr(len))) {
					for (auto& [key, value] : *res) {
						if (value.isString()) {
							m_fields->layerNames.insert({std::atoi(key.c_str()), *value.asString()});
						}
					}
				}
			}
		}
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
			TextInput* inputL1;
			TextInput* inputL2;
			std::vector<GameObject*> objects;
		} betterEdit;
	};


	static void onModify(auto& self) {
		// after all Better Edit changes
        (void) self.setHookPriorityAfterPost("SetGroupIDLayer::init", "hjfod.betteredit");
    }


	void initKeybinds() {
		#ifdef GEODE_IS_DESKTOP
			this->template addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
				if (event->isDown()) {
					if (CCScene::get()->getChildByID("simple-select-popup"_spr) == nullptr && CCScene::get()->getChildByID("SetGroupIDLayer") != nullptr) {
						onL1Click(nullptr);
					}
				}
				return ListenerResult::Propagate;
			}, "change-layer-1"_spr);

			this->template addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
				if (event->isDown()) {
					if (CCScene::get()->getChildByID("simple-select-popup"_spr) == nullptr && CCScene::get()->getChildByID("SetGroupIDLayer") != nullptr) {
						onL2Click(nullptr);
					}
				}
				return ListenerResult::Propagate;
			}, "change-layer-2"_spr);
		#endif // GEODE_IS_DESKTOP
	}
	

	bool init(GameObject* obj, CCArray* objects) {
		if (!SetGroupIDLayer::init(obj, objects)) return false;

		if (!Mod::get()->getSettingValue<bool>("in-edit-groups-menu")) {
			return true;
		}

		auto menuL1 = m_mainLayer->getChildByID("editor-layer-menu");
		auto menuL2 = m_mainLayer->getChildByID("editor-layer-2-menu");
		if (!menuL1 || !menuL2) return true;

		if (menuL1->getChildByID("hjfod.betteredit/unmix-button") != nullptr) {
			m_fields->isBetterEdit = true;
			m_fields->betterEdit.inputL1 = menuL1->getChildByType<TextInput>(0);
			m_fields->betterEdit.inputL2 = menuL2->getChildByType<TextInput>(0);
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
		if (str.empty() || !std::all_of(str.begin(), str.end(), ::isdigit)) return -1;
		return std::stoi(str);
	}
	
	
	int getL2Value() {
		if (!m_fields->isBetterEdit) {
			return m_editorLayer2Value;
		}
		// with BetterEdit
		auto str = m_fields->betterEdit.inputL2->getString();
		if (str.empty() || !std::all_of(str.begin(), str.end(), ::isdigit)) return -1;
		return std::stoi(str);
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
		if (auto menu = m_mainLayer->getChildByID("editor-layer-menu")) {
			if (auto btn = menu->getChildByID("hjfod.betteredit/unmix-button")) {
				btn->setVisible(false);
			}
		}
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
		if (auto menu = m_mainLayer->getChildByID("editor-layer-2-menu")) {
			if (auto btn = menu->getChildByID("hjfod.betteredit/unmix-button")) {
				btn->setVisible(false);
			}
		}
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
			auto it = myEditorUI->m_fields->layerNames.find(layer);
			if (it != myEditorUI->m_fields->layerNames.end()) {
				text = (*it).second.c_str();
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
			&myEditorUI->m_fields->layerNames, 
			getL1Value(),
			[this](int layer) {
				setL1Value(layer);
			}
		})->show();
	}

	
	void onL2Click(CCObject*) {
		SelectPopup::create({
			"Select Layer 2",
			&myEditorUI->m_fields->layerNames, 
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
		if (Mod::get()->getSettingValue<bool>("use-save-object")) {
			static_cast<MyEditorUI*>(EditorUI::get())->handleSaveObject(1, true);
		}
		EditorPauseLayer::saveLevel();
	}
};


