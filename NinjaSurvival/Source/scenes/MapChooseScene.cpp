#include "MapChooseScene.h"
#include "Utils.h"
#include "MainScene.h"
#include "GameScene.h"
#include "GameLoadingScene.h"
#include "systems/GameData.h"
#include "systems/ShopSystem.h"
#include "systems/SystemManager.h"
#include "AudioManager.h"

using namespace ax;

MapChooseScene::MapChooseScene() : SceneBase("MapChooseScene") {}

bool MapChooseScene::init()
{
    if (!SceneBase::init())
    {
        return false;
    }
    SystemManager::getInstance()->resetSystems();
    menuUISetup();

    auto entityTemplates = GameData::getInstance()->getEntityTemplates();
    for (const auto& [type, innerMap] : entityTemplates)
    {
        if (type == "weapon_melee" || type == "weapon_projectile")
            for (const auto& [name, entity] : innerMap)
            {
                if (entity.cooldown)
                {
                    AXLOG("Weapon %s có cooldown duration: %f", name.c_str(), entity.cooldown->cooldownDuration);
                }
                else
                {
                    AXLOG("%s không có cooldown", name.c_str());
                }
            }
    }

    ShopSystem::getInstance()->getShopBuff("LootRange");
    ShopSystem::getInstance()->getShopBuff("SpawnRate");
    ShopSystem::getInstance()->getShopBuff("Attack");
    ShopSystem::getInstance()->getShopBuff("XPGain");
    ShopSystem::getInstance()->getShopBuff("CoinGain");
    ShopSystem::getInstance()->getShopBuff("RerollWeapon");
    ShopSystem::getInstance()->getShopBuff("ReduceCooldown");


    return true;
}

void MapChooseScene::update(float dt) {}

void MapChooseScene::menuCloseCallback(Object* sender)
{
    AudioManager::getInstance()->playSound("button_click", false, 1.0f, "click");
    AudioManager::getInstance()->stopSound("background");
    _director->replaceScene(TransitionFade::create(TRANSITION_TIME, utils::createInstance<MainScene>()));
}

void MapChooseScene::menuPlayCallback(Object* sender)
{
    if (selectedMapName.empty())
    {
        AXLOG("Error: No map selected");
        return;
    }
    _director->getEventDispatcher()->setEnabled(false);
    GameData::getInstance()->setSelectedMap(selectedMapName);
    auto gameScene    = utils::createInstance<GameScene>();
    auto loadingScene = GameLoadingScene::create();
    loadingScene->setNextScene(gameScene);
    AudioManager::getInstance()->playSound("button_click", false, 1.0f, "click");
    AudioManager::getInstance()->stopSound("background");
    _director->replaceScene(TransitionFade::create(TRANSITION_TIME, loadingScene));
    this->scheduleOnce([this](float) { _director->getEventDispatcher()->setEnabled(true); }, TRANSITION_TIME,
                       "enableInput");
}

void MapChooseScene::menuUISetup()
{
    const auto& safeOrigin = safeArea.origin;
    const auto& safeSize   = safeArea.size;
    float marginX          = safeSize.width * 0.05f;
    float marginY          = safeSize.height * 0.025f;

    auto background = Sprite::create("UI/background3.png");
    if (!background)
    {
        AXLOG("Lỗi: Không thể tạo background3.png");
        return;
    }
    background->setPosition(origin + visibleSize / 2);
    this->addChild(background, 0);

    auto panel = Sprite::create("UI/panelMap.png");
    if (!panel)
    {
        AXLOG("Lỗi: Không thể tạo panelMap.png");
        return;
    }
    panel->setPosition(safeOrigin.x + safeSize.width / 2, safeOrigin.y + safeSize.height / 2 - 10);
    this->addChild(panel, 2);

    // Hiển thị coin
    auto shopData = ShopSystem::getInstance();
    auto coinLabel =
        Label::createWithTTF(StringUtils::format("%d", shopData->getCoins()), "fonts/Pixelpurl-0vBPP.ttf", 40);
    if (!coinLabel)
    {
        AXLOG("Lỗi: Không thể tạo coinLabel");
        return;
    }
    auto coinSprite = Sprite::create("UI/coinUI.png");
    if (!coinSprite)
    {
        AXLOG("Lỗi: Không thể tạo coinSprite");
        return;
    }

    // Tính toán vị trí cố định cho coinSprite (căn phải trong vùng an toàn)
    float coinSpriteWidth = coinSprite->getContentSize().width * 1.5f;  // Nhân với scale
    float coinSpritePosX  = safeOrigin.x + safeSize.width - marginX - coinSpriteWidth / 2.0f;
    float posY            = safeOrigin.y + safeSize.height - marginY;

    // Đặt vị trí và scale cho coinSprite
    coinSprite->setPosition(coinSpritePosX, posY);
    coinSprite->setScale(1.5f);
    this->addChild(coinSprite, 5, "coinSprite");

    // Tính toán vị trí cho coinLabel (mép phải cố định ngay trước coinSprite)
    float spacing       = 5.0f;  // Khoảng cách giữa coinLabel và coinSprite
    float coinLabelPosX = coinSpritePosX - coinSpriteWidth / 2.0f - spacing;  // Mép phải của coinLabel

    // Đặt thuộc tính cho coinLabel
    coinLabel->setAnchorPoint(Vec2(1.0f, 0.5f));  // Điểm neo ở mép phải
    coinLabel->setPosition(coinLabelPosX, posY);
    coinLabel->setAlignment(ax::TextHAlignment::RIGHT);
    this->addChild(coinLabel, 5, "coinLabel");

    Vector<MenuItem*> menuItems;

    closeItem = Utils::createMenuItem("UI/buttonback.png", "UI/buttonback.png",
                                      AX_CALLBACK_1(MapChooseScene::menuCloseCallback, this),
                                      Vec2(safeOrigin.x + marginX, safeOrigin.y + safeSize.height - marginY));
    if (!closeItem)
    {
        AXLOG("Lỗi: Không thể tạo closeItem");
        return;
    }
    menuItems.pushBack(closeItem);

    float panelRight  = panel->getPositionX() + panel->getContentSize().width / 2;
    float panelBottom = panel->getPositionY() - panel->getContentSize().height / 2;
    auto playSprite   = Sprite::create("UI/buttonPlayGame.png");
    if (!playSprite)
    {
        AXLOG("Lỗi: Không thể tạo buttonPlayGame.png");
        return;
    }
    float playX = panelRight - (playSprite->getContentSize().width / 2 * 0.8f) - panel->getContentSize().width * 0.075f;
    float playY =
        panelBottom + (playSprite->getContentSize().height / 2 * 0.8f) + panel->getContentSize().height * 0.03f;

    auto playButton = dynamic_cast<MenuItemSprite*>(
        Utils::createButton("UI/buttonPlayGame.png", AX_CALLBACK_1(MapChooseScene::menuPlayCallback, this),
                            Vec2(playX, playY), 0.8f, false));
    if (!playButton)
    {
        AXLOG("Lỗi: Không thể tạo playButton");
        return;
    }
    menuItems.pushBack(playButton);

    auto buyButton = createBuyButton(playButton, panel, playX, playY);
    if (!buyButton)
    {
        AXLOG("Lỗi: Không thể tạo buyButton");
        return;
    }
    menuItems.pushBack(buyButton);

    setupMapButtons(panel, playButton, buyButton, menuItems);

    auto menu = Menu::createWithArray(menuItems);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 3);
}

MenuItemSprite* MapChooseScene::createBuyButton(MenuItemSprite* playButton, Node* panel, float x, float y)
{
    auto button = dynamic_cast<MenuItemSprite*>(
        Utils::createButton("UI/buttonBuy.png", [this, playButton, panel](Object* sender) {
        auto shopData    = ShopSystem::getInstance();
        auto gameData    = GameData::getInstance();
        int currentCoins = shopData->getCoins();
        int cost         = shopData->getCost("maps", selectedMapName);

        if (!selectedMapName.empty() && currentCoins >= cost)
        {
            shopData->setCoins(currentCoins - cost);
            shopData->setAvailable("maps", selectedMapName, true);
            gameData->setMapAvailable(selectedMapName, true);
            shopData->syncMapsWithGameData();

            if (selectedMapItem)
            {
                auto& maps = const_cast<std::unordered_map<std::string, MapData>&>(gameData->getMaps());
                if (maps.find(selectedMapName) != maps.end())
                {
                    maps[selectedMapName].available = true;
                }
                updateMapUI(selectedMapItem, selectedMapName, true, playButton, dynamic_cast<MenuItemSprite*>(sender));
            }

            // Cập nhật coinLabel
            updateCoinLabel(shopData->getCoins());

            shopData->saveToFile(FileUtils::getInstance()->getWritablePath() + "savegame.dat");
            AXLOG("Đã mở khóa map: %s", selectedMapName.c_str());
        }
        else
        {
            auto errorLabel = Label::createWithTTF("Không đủ coin!", "fonts/Pixelpurl-0vBPP.ttf", 20);
            if (errorLabel)
            {
                errorLabel->setPosition(safeArea.origin + safeArea.size / 2);
                this->addChild(errorLabel, 20);
                errorLabel->runAction(Sequence::create(FadeOut::create(2.0f), RemoveSelf::create(), nullptr));
            }
            dynamic_cast<Node*>(sender)->setVisible(false);
        }
    }, Vec2(x, y), 0.8f, false));
    if (!button)
    {
        AXLOG("Lỗi: Không thể tạo buyButton");
        return nullptr;
    }
    return button;
}

void MapChooseScene::updateMapUI(MenuItemSprite* item,
                                 const std::string& mapName,
                                 bool isAvailable,
                                 MenuItemSprite* playButton,
                                 MenuItemSprite* buyButton)
{
    Utils::updateItemUI(item, nullptr, isAvailable);
    if (auto* label = dynamic_cast<Label*>(item->getChildByName("label")))
    {
        if (isAvailable)
        {
            label->setString(mapName);
        }
        else
        {
            auto shopData = ShopSystem::getInstance();
            int cost      = shopData->getCost("maps", mapName);
            label->setString(StringUtils::format("Locked - %d", cost));
        }
        label->setVisible(true);
    }
    buyButton->setVisible(!isAvailable);
    playButton->setVisible(isAvailable);
}

void MapChooseScene::setupMapButtons(Node* panel,
                                     MenuItemSprite* playButton,
                                     MenuItemSprite* buyButton,
                                     Vector<MenuItem*>& menuItems)
{
    auto gameData    = GameData::getInstance();
    const auto& maps = gameData->getMaps();
    if (maps.empty())
    {
        AXLOG("Lỗi: Không có map trong gameData");
        return;
    }

    // Tạo mapOrder từ MapData
    std::vector<std::pair<std::string, int>> mapOrder;
    int index = 2;
    for (const auto& [name, mapData] : maps)
    {
        if (name == "Plains")
        {
            mapOrder.emplace_back(name, 0);
        }
        else if (name == "Snow Field")
        {
            mapOrder.emplace_back(name, 1);
        }
        else
        {
            mapOrder.emplace_back(name, index++);
        }
    }

    // Thiết lập lưới: 1 cột
    const int cols    = 1;
    const int numMaps = static_cast<int>(maps.size());
    const int rows    = (numMaps + cols - 1) / cols;

    // Tính khoảng cách dựa trên kích thước panel
    float iconSpacingY = panel->getContentSize().height * 0.06f;
    // Đặt startY sát đỉnh panel
    float startY = panel->getPositionY() + panel->getContentSize().height * 1 / 3 - iconSpacingY;

    for (const auto& [name, map] : maps)
    {
        // Tìm index từ mapOrder
        int index = -1;
        for (const auto& pair : mapOrder)
        {
            if (pair.first == name)
            {
                index = pair.second;
                break;
            }
        }

        // Bỏ qua nếu không tìm thấy
        if (index == -1)
        {
            AXLOG("Cảnh báo: Map %s không có trong mapOrder, bỏ qua", name.c_str());
            continue;
        }

        auto button =
            Utils::createMapButton(map.sprite, map.name, map.available, selectedMapName, selectedMapItem, playButton);
        if (!button)
        {
            AXLOG("Lỗi: Không thể tạo button map: %s", map.name.c_str());
            continue;
        }

        // Giữ logic callback của MapChooseScene
        button->setCallback([this, mapName = map.name, playButton, buyButton, panel](Object* sender) {
            selectedMapName = mapName;

            if (selectedMapItem && selectedMapItem != sender)
            {
                if (auto* oldLabel = dynamic_cast<Label*>(selectedMapItem->getChildByName("label")))
                {
                    oldLabel->setVisible(false);
                }
                if (auto* oldBorder = selectedMapItem->getChildByName("border"))
                {
                    oldBorder->setVisible(false);
                }
            }
            if (selectedMapItem)
            {
                selectedMapItem->setEnabled(true);
            }
            selectedMapItem = dynamic_cast<MenuItemSprite*>(sender);
            selectedMapItem->setEnabled(false);
            if (auto* border = selectedMapItem->getChildByName("border"))
            {
                border->setVisible(true);
            }

            auto gameData    = GameData::getInstance();
            bool isAvailable = gameData->getMaps().at(mapName).available;
            updateMapUI(selectedMapItem, mapName, isAvailable, playButton, buyButton);
        });

        // Tính vị trí: căn giữa ngang, từ trên xuống dưới
        int row    = index / cols;
        float posX = panel->getPositionX();  // Căn giữa panel
        float posY = startY - row * (button->getContentSize().height + iconSpacingY);
        button->setPosition(Vec2(posX, posY));
        menuItems.pushBack(button);
    }
}

void MapChooseScene::updateCoinLabel(int newCoinValue)
{
    auto coinLabel  = dynamic_cast<ax::Label*>(this->getChildByName("coinLabel"));
    auto coinSprite = dynamic_cast<ax::Sprite*>(this->getChildByName("coinSprite"));
    if (!coinLabel || !coinSprite)
    {
        AXLOG("Lỗi: Không tìm thấy coinLabel hoặc coinSprite");
        return;
    }

    // Cập nhật giá trị coin
    coinLabel->setString(StringUtils::format("%d", newCoinValue));

    // Cập nhật vị trí coinLabel để giữ mép phải cố định
    float coinSpriteWidth = coinSprite->getContentSize().width * coinSprite->getScaleX();
    float coinSpritePosX  = coinSprite->getPositionX();
    float spacing         = 5.0f;
    float coinLabelPosX   = coinSpritePosX - coinSpriteWidth / 2.0f - spacing;

    coinLabel->setPosition(coinLabelPosX, coinLabel->getPositionY());
}
