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
#ifndef GEODE_IS_MOBILE
	#include <geode.custom-keybinds/include/Keybinds.hpp>
	$execute {
		keybinds::BindManager::get()->registerBindable({
			"open-list"_spr, "Open layer list",
			"Open the list of the used editor layers",
			{ keybinds::Keybind::create(KEY_G, keybinds::Modifier::Control) },
			"Named Editor Layers"
		});
	}
#endif // GEODE_IS_MOBILE

// support for BetterEdit scale factor
inline float getBetterEditInterfaceScale() {
	if (Loader::get()->isModInstalled("hjfod.betteredit")) {
		auto betterEdit = Loader::get()->getInstalledMod("hjfod.betteredit");
		if (betterEdit->isEnabled() && betterEdit->hasSetting("scale-factor")) {
			float scale = betterEdit->getSettingValue<double>("scale-factor");
			if (scale > 0.1) return scale;
		}
	}
	return 1;
}

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
			Mod::get()->setSavedValue(std::to_string(levelId), jsonVal.dump());
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
		menu->setPosition(layerMenu->getPosition() + ccp(0,-2) + ccp(layerMenu->getScaledContentWidth() / 2.f, -layerMenu->getScaledContentHeight() / 2.f));
		
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
		// log::debug("layer changed {}", layer);
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
		auto topRight = bigMenu->getPosition() + bigMenu->getScaledContentSize() / 2.f;
		bigMenu->setScale(.88f);
		bigMenu->setPosition(topRight - bigMenu->getScaledContentSize() / 2.f);

		auto smallMenu = getChildByID("layer-menu");
		auto right = smallMenu->getPosition() + ccp(smallMenu->getScaledContentWidth() / 2.f, 0);
		smallMenu->setScale(.88f);
		smallMenu->setPosition(right - ccp(smallMenu->getScaledContentWidth() / 2.f, -17));

		auto lockSpr = getChildByID("layer-locked-sprite");
		lockSpr->setPosition(lockSpr->getPosition() + ccp(2, 17));
	}

	void initKeybinds() {
		#ifndef GEODE_IS_MOBILE
			this->template addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
				if (event->isDown()) {
					if (CCScene::get()->getChildByID("set-name-popup"_spr) == nullptr && CCScene::get()->getChildByID("layer-list-popup"_spr) == nullptr) {
						onLayerListButton(nullptr);
					}
				}
				return ListenerResult::Propagate;
			}, "open-list"_spr);
		#endif // GEODE_IS_MOBILE
	}

	bool init(LevelEditorLayer* editor) {
		if (!EditorUI::init(editor)) {
			return false;
		}
		m_fields->levelId = EditorIDs::getID(editor->m_level);
		m_fields->loadMap();
		
		if (getBetterEditInterfaceScale() > 0.85) {
			freeUpSomeSpace();
		}
		
		setupLayerMenu();
		initKeybinds();
		
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
		std::vector<int> layersToInclude = {1, 2, 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
		LayerListPopup::create({
			std::move(layersToInclude),
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