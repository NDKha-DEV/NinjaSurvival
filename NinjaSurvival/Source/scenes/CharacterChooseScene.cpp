#include "CharacterChooseScene.h"
#include "Utils.h"
#include "MapChooseScene.h"
#include "MainScene.h"
#include "systems/GameData.h"
#include "systems/ShopSystem.h"
#include "AudioManager.h"
#include "WeaponUpgradeUtils.h"

using namespace ax;

CharacterChooseScene::CharacterChooseScene() : SceneBase("CharacterChooseScene") {}

bool CharacterChooseScene::init()
{
    if (!SceneBase::init())
    {
        return false;
    }

    GameData::getInstance()->syncStatsWithShopSystem();
    menuUISetup();
    return true;
}

void CharacterChooseScene::update(float dt) {}

void CharacterChooseScene::menuReturnCallback(Object* sender)
{
    AudioManager::getInstance()->playSound("button_click", false, 1.0f, "click");
    AudioManager::getInstance()->stopSound("background");
    _director->replaceScene(TransitionFade::create(TRANSITION_TIME, utils::createInstance<MainScene>()));
}

void CharacterChooseScene::menuNextCallback(Object* sender)
{
    if (selectedCharacterName.empty())
    {
        AXLOG("Lỗi: Chưa chọn nhân vật nào");
        return;
    }
    _director->getEventDispatcher()->setEnabled(false);
    GameData::getInstance()->setSelectedCharacter(selectedCharacterName);
    AudioManager::getInstance()->playSound("button_click", false, 1.0f, "click");
    _director->replaceScene(TransitionFade::create(TRANSITION_TIME, utils::createInstance<MapChooseScene>()));
    this->scheduleOnce([this](float) { _director->getEventDispatcher()->setEnabled(true); }, TRANSITION_TIME,
                       "enableInput");
}

void CharacterChooseScene::menuUISetup()
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

    auto panelChooseCharacter = Sprite::create("UI/NeopanelChoseCharacter2.png");
    if (!panelChooseCharacter)
    {
        AXLOG("Lỗi: Không thể tạo NeopanelChoseCharacter2.png");
        return;
    }
    float panelY = safeOrigin.y + safeSize.height * 0.95f - panelChooseCharacter->getContentSize().height / 2;
    panelChooseCharacter->setPosition(safeOrigin.x + safeSize.width / 2, panelY);
    this->addChild(panelChooseCharacter, 2);

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

    returnButton = Utils::createMenuItem("UI/buttonback.png", "UI/buttonback.png",
                                         AX_CALLBACK_1(CharacterChooseScene::menuReturnCallback, this),
                                         Vec2(safeOrigin.x + marginX, safeOrigin.y + safeSize.height - marginY));
    if (!returnButton)
    {
        AXLOG("Lỗi: Không thể tạo nút Return");
        return;
    }
    menuItems.pushBack(returnButton);

    auto panelDescription = Sprite::create("UI/NeoDescriptionCharacter.png");
    if (!panelDescription)
    {
        AXLOG("Lỗi: Không thể tạo NeoDescriptionCharacter.png");
        return;
    }
    float descY = panelChooseCharacter->getPositionY() - panelChooseCharacter->getContentSize().height / 2 -
                  safeSize.height * 0.01f - panelDescription->getContentSize().height / 2;
    panelDescription->setPosition(safeOrigin.x + safeSize.width / 2, descY);
    this->addChild(panelDescription, 2);

    auto chooseCharacterLabel = Label::createWithTTF("Choose your character", "fonts/Pixelpurl-0vBPP.ttf", 30);
    if (!chooseCharacterLabel)
    {
        AXLOG("Lỗi: Không thể tạo chooseCharacterLabel");
        return;
    }
    chooseCharacterLabel->setPosition(panelDescription->getContentSize().width / 2,
                                      panelDescription->getContentSize().height / 2);
    chooseCharacterLabel->setAlignment(TextHAlignment::CENTER);
    chooseCharacterLabel->setVisible(true);
    panelDescription->addChild(chooseCharacterLabel, 6, "chooseCharacterLabel");

    auto nextSprite = Sprite::create("UI/buttonNext.png");
    if (!nextSprite)
    {
        AXLOG("Lỗi: Không thể tạo buttonNext.png");
        return;
    }
    float panelDescRight  = panelDescription->getPositionX() + panelDescription->getContentSize().width / 2;
    float panelDescBottom = panelDescription->getPositionY() - panelDescription->getContentSize().height / 2;
    float playX           = panelDescRight - (nextSprite->getContentSize().width / 2 * 0.8f) -
                  panelDescription->getContentSize().width * 0.045f;
    float playY = panelDescBottom + (nextSprite->getContentSize().height / 2 * 0.8f) +
                  panelDescription->getContentSize().height * 0.075f;

    auto nextButton =
        Utils::createButton("UI/buttonNext.png", AX_CALLBACK_1(CharacterChooseScene::menuNextCallback, this),
                            Vec2(playX, playY), 0.8f, false);
    if (!nextButton)
    {
        AXLOG("Lỗi: Không thể tạo nút Next");
        return;
    }
    menuItems.pushBack(nextButton);

    auto buyButton = createBuyButton(dynamic_cast<MenuItemSprite*>(nextButton), panelDescription, playX, playY);
    if (!buyButton)
    {
        AXLOG("Lỗi: Không thể tạo nút Buy");
        return;
    }
    menuItems.pushBack(buyButton);

    setupCharacterButtons(panelChooseCharacter, panelDescription, dynamic_cast<MenuItemSprite*>(nextButton), buyButton,
                          menuItems);

    auto menu = Menu::createWithArray(menuItems);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 6);
}

ax::ui::ScrollView* CharacterChooseScene::createWeaponDesScrollView(const std::string& name,
                                                                    const std::string& tag,
                                                                    float baseY,
                                                                    float xPos,
                                                                    Node* parent)
{
    auto scrollView = ax::ui::ScrollView::create();
    if (!scrollView)
    {
        AXLOG("Lỗi: Không thể tạo ScrollView cho weaponDes");
        return nullptr;
    }

    auto panelSize     = parent->getContentSize();
    float maxWidth     = panelSize.width - xPos - 20.0f;
    float scrollHeight = 60.0f;

    scrollView->setContentSize(ax::Size(maxWidth, scrollHeight));
    scrollView->setPosition(Vec2(xPos, baseY));
    scrollView->setAnchorPoint(Vec2(0.0f, 0.5f));
    scrollView->setDirection(ax::ui::ScrollView::Direction::VERTICAL);
    scrollView->setBounceEnabled(true);
    scrollView->setScrollBarEnabled(false);
    scrollView->setVisible(false);

    auto label = ax::Label::createWithTTF("", "fonts/Pixelpurl-0vBPP.ttf", 18);
    if (!label)
    {
        AXLOG("Lỗi: Không thể tạo Label trong ScrollView");
        return nullptr;
    }
    label->setAnchorPoint(Vec2(0.0f, 1.0f));
    label->setPosition(Vec2(0.0f, scrollHeight));
    label->setAlignment(ax::TextHAlignment::LEFT);
    label->setMaxLineWidth(maxWidth);
    label->setDimensions(200, 0);
    scrollView->addChild(label, 0, "weaponDesLabel");

    parent->addChild(scrollView, 6, "weaponDesScrollView" + name);
    return scrollView;
}

void CharacterChooseScene::updateCharacterStats(const std::string& name, Node* panelDescription, bool isAvailable)
{
    auto gameData       = GameData::getInstance();
    auto shopData       = ShopSystem::getInstance();
    auto characterLabel = dynamic_cast<Label*>(panelDescription->getChildByName("label" + name));
    auto healthLabel    = dynamic_cast<Label*>(panelDescription->getChildByName("healthLabel" + name));
    auto weaponLabel    = dynamic_cast<Label*>(panelDescription->getChildByName("weaponLabel" + name));
    auto weaponDesScrollView =
        dynamic_cast<ax::ui::ScrollView*>(panelDescription->getChildByName("weaponDesScrollView" + name));
    auto speedLabel  = dynamic_cast<Label*>(panelDescription->getChildByName("speedLabel" + name));
    auto unlockLabel = dynamic_cast<Label*>(panelDescription->getChildByName("unlockLabel" + name));

    if (!characterLabel || !healthLabel || !weaponLabel || !weaponDesScrollView || !speedLabel || !unlockLabel)
    {
        AXLOG("Lỗi: Thiếu label hoặc ScrollView cho nhân vật %s", name.c_str());
        return;
    }

    for (auto* child : panelDescription->getChildren())
    {
        if (dynamic_cast<Label*>(child) || dynamic_cast<ax::ui::ScrollView*>(child))
            child->setVisible(false);
    }

    characterLabel->setVisible(true);

    if (!isAvailable)
    {
        unlockLabel->setString(StringUtils::format("Unlock: %d", shopData->getCost("entities", name)));
        unlockLabel->setVisible(true);
        return;
    }

    const auto& entities = gameData->getEntityTemplates();
    if (entities.find("player") == entities.end() || entities.at("player").find(name) == entities.at("player").end())
    {
        AXLOG("Lỗi: Không tìm thấy nhân vật %s trong entityTemplates", name.c_str());
        return;
    }

    auto templ = entities.at("player").at(name);
    healthLabel->setString(templ.health.has_value() ? StringUtils::format("- Health: %.0f", templ.health->maxHealth)
                                                    : "Health: N/A");
    speedLabel->setString(templ.speed.has_value() ? StringUtils::format("- Speed: %.0f", templ.speed->speed)
                                                  : "Speed: N/A");

    std::string mainWpName;
    std::string desWeapon;
    if (templ.weaponInventory.has_value() && !templ.weaponInventory->weapons.empty() &&
        !templ.weaponInventory->weapons[0].first.empty())
    {
        if (templ.weaponInventory->weapons[0].first == "sword")
        {
            mainWpName = "Katana";
        }
        else if (templ.weaponInventory->weapons[0].first == "shuriken")
        {
            mainWpName = "Shuriken Storm";
        }
        else if (templ.weaponInventory->weapons[0].first == "kunai")
        {
            mainWpName = "Kunai";
        }
        else if (templ.weaponInventory->weapons[0].first == "big_kunai")
        {
            mainWpName = "Bounce Kunai";
        }
        else if (templ.weaponInventory->weapons[0].first == "spinner")
        {
            mainWpName = "Death Spinner";
        }
        else if (templ.weaponInventory->weapons[0].first == "explosion_kunai")
        {
            mainWpName = "Explosive Shot";
        }
        else if (templ.weaponInventory->weapons[0].first == "ninjutsu_spell")
        {
            mainWpName = "Ninjutsu Spell";
        }
        else if (templ.weaponInventory->weapons[0].first == "lightning_scroll")
        {
            mainWpName = "Lightning Strike";
        }

        weaponLabel->setString(StringUtils::format("*Main Weapon:\n%s\n*Description:", mainWpName.c_str()));
        desWeapon = templ.weaponInventory->weapons[0].first;
    }
    else
    {
        weaponLabel->setString("Main Weapon: None");
    }

    auto weaponDesLabel = dynamic_cast<Label*>(weaponDesScrollView->getChildByName("weaponDesLabel"));
    if (weaponDesLabel)
    {
        std::string description = !desWeapon.empty() ? WeaponUpgradeUtils::getDescription(desWeapon, 0) : "None";
        weaponDesLabel->setString(description);

        auto labelSize  = weaponDesLabel->getContentSize();
        auto scrollSize = weaponDesScrollView->getContentSize();
        weaponDesScrollView->setInnerContainerSize(
            ax::Size(scrollSize.width, std::max(scrollSize.height, labelSize.height)));
        weaponDesLabel->setPosition(Vec2(0.0f, weaponDesScrollView->getInnerContainerSize().height));
    }

    healthLabel->setVisible(true);
    speedLabel->setVisible(true);
    weaponLabel->setVisible(true);
    weaponDesScrollView->setVisible(true);
}

Label* CharacterChooseScene::createStatLabel(const std::string& name,
                                             const std::string& tag,
                                             float baseY,
                                             float xPos,
                                             Node* parent)
{
    auto label = Label::createWithTTF("", "fonts/Pixelpurl-0vBPP.ttf", 18);
    if (!label)
    {
        AXLOG("Lỗi: Không thể tạo label %s", tag.c_str());
        return nullptr;
    }
    label->setAnchorPoint(Vec2(0.0f, 0.5f));
    label->setPosition(xPos, baseY);
    label->setAlignment(TextHAlignment::LEFT);
    label->setVisible(false);
    parent->addChild(label, 6, tag + name);
    return label;
}

MenuItemSprite* CharacterChooseScene::createBuyButton(MenuItemSprite* nextButton,
                                                      Node* panelDescription,
                                                      float x,
                                                      float y)
{
    return dynamic_cast<MenuItemSprite*>(
        Utils::createButton("UI/buttonBuy.png", [this, nextButton, panelDescription](Object* sender) {
        auto shopData    = ShopSystem::getInstance();
        auto gameData    = GameData::getInstance();
        int currentCoins = shopData->getCoins();
        int cost         = shopData->getCost("entities", selectedCharacterName);

        if (selectedCharacterName.empty() || currentCoins < cost)
        {
            auto errorLabel = Label::createWithTTF("You so poor :)))!", "fonts/Pixelpurl-0vBPP.ttf", 20);
            if (errorLabel)
            {
                errorLabel->setPosition(safeArea.origin + safeArea.size / 2);
                this->addChild(errorLabel, 20);
                errorLabel->runAction(Sequence::create(FadeOut::create(2.0f), RemoveSelf::create(), nullptr));
            }
            dynamic_cast<Node*>(sender)->setVisible(false);
            return;
        }

        shopData->setCoins(currentCoins - cost);
        shopData->setAvailable("entities", selectedCharacterName, true);
        gameData->setCharacterAvailable(selectedCharacterName, true);
        shopData->syncCharactersWithGameData();

        if (selectedCharacterItem)
        {
            auto& entities =
                const_cast<std::unordered_map<std::string, std::unordered_map<std::string, EntityTemplate>>&>(
                    gameData->getEntityTemplates());
            if (entities.find("player") != entities.end() &&
                entities["player"].find(selectedCharacterName) != entities["player"].end())
            {
                entities["player"][selectedCharacterName].available = true;
            }

            Utils::updateItemUI(selectedCharacterItem, panelDescription, true);
            dynamic_cast<Node*>(sender)->setVisible(false);
            nextButton->setVisible(true);
            updateCharacterStats(selectedCharacterName, panelDescription, true);
        }

        // Cập nhật coinLabel
        updateCoinLabel(shopData->getCoins());

        shopData->saveToFile(FileUtils::getInstance()->getWritablePath() + "savegame.dat");
        AXLOG("Đã mở khóa nhân vật: %s", selectedCharacterName.c_str());
    }, Vec2(x, y), 0.8f, false));
}

void CharacterChooseScene::setupCharacterButtons(Node* panelChooseCharacter,
                                                 Node* panelDescription,
                                                 MenuItemSprite* nextButton,
                                                 MenuItemSprite* buyButton,
                                                 Vector<MenuItem*>& menuItems)
{
    auto gameData        = GameData::getInstance();
    const auto& entities = gameData->getEntityTemplates();
    if (entities.find("player") == entities.end())
    {
        AXLOG("Lỗi: Không có nhân vật loại 'player' trong entityTemplates");
        return;
    }

    // Tạo characterOrder từ entityTemplates
    std::vector<std::pair<std::string, int>> characterOrder;
    int index = 0;
    for (const auto& [name, templ] : entities.at("player"))
    {
        characterOrder.emplace_back(name, index++);
    }

    int numCharacters = entities.at("player").size();
    const int cols    = 4;                                  // Số cột cố định là 4
    const int rows    = (numCharacters + cols - 1) / cols;  // Tính số hàng

    float iconSpacingX = panelChooseCharacter->getContentSize().width * 0.093f;
    float iconSpacingY = panelChooseCharacter->getContentSize().height * 0.03f;
    float panelLeft    = panelChooseCharacter->getPositionX() - panelChooseCharacter->getContentSize().width / 2.4f;
    float startY =
        panelChooseCharacter->getPositionY() + panelChooseCharacter->getContentSize().height / 2.5f - iconSpacingY;
    float startX = panelLeft + iconSpacingX;

    float leftMargin = panelDescription->getContentSize().width * 0.06f;

    for (const auto& [name, templ] : entities.at("player"))
    {
        // Tìm index từ characterOrder
        int index = -1;
        for (const auto& pair : characterOrder)
        {
            if (pair.first == name)
            {
                index = pair.second;
                break;
            }
        }

        if (index == -1)
        {
            AXLOG("Cảnh báo: Nhân vật %s không có trong characterOrder, bỏ qua", name.c_str());
            continue;
        }

        auto characterLabel = Label::createWithTTF(name, "fonts/Pixelpurl-0vBPP.ttf", 30);
        if (!characterLabel)
        {
            AXLOG("Lỗi: Không thể tạo label cho nhân vật %s", name.c_str());
            continue;
        }
        characterLabel->setPosition(panelDescription->getContentSize().width / 2,
                                    panelDescription->getContentSize().height * 0.85f);
        characterLabel->setAlignment(TextHAlignment::CENTER);
        characterLabel->setVisible(false);
        panelDescription->addChild(characterLabel, 6, "label" + name);

        float characterLabelBottomY =
            panelDescription->getContentSize().height * 0.85f - characterLabel->getContentSize().height * 0.5f;
        float baseY = characterLabelBottomY - 10.0f;

        auto weaponLabel       = createStatLabel(name, "weaponLabel", baseY - 15, leftMargin, panelDescription);
        float weaponLabelWidth = weaponLabel ? weaponLabel->getContentSize().width : 100.0f;
        float healthLabelX     = leftMargin + weaponLabelWidth + 170.0f;
        auto healthLabel       = createStatLabel(name, "healthLabel", baseY, healthLabelX, panelDescription);
        auto weaponDesScrollView =
            createWeaponDesScrollView(name, "weaponDesScrollView", baseY - 70.0f, leftMargin, panelDescription);
        auto speedLabel  = createStatLabel(name, "speedLabel", baseY - 15.0f, healthLabelX, panelDescription);
        auto unlockLabel = createStatLabel(name, "unlockLabel", baseY - 50.0f, leftMargin, panelDescription);

        if (!healthLabel || !weaponLabel || !weaponDesScrollView || !speedLabel || !unlockLabel)
        {
            AXLOG("Lỗi: Không thể tạo đầy đủ label cho nhân vật %s", name.c_str());
            continue;
        }

        auto button = Utils::createCharacterButton(
            templ.profilePhoto.has_value() ? templ.profilePhoto.value() : "CloseNormal.png", name, templ.available,
            selectedCharacterName, selectedCharacterItem, nextButton);
        if (!button)
        {
            AXLOG("Lỗi: Không thể tạo button cho nhân vật %s", name.c_str());
            continue;
        }

        button->setCallback([this, characterName = name, buyButton, nextButton, panelDescription](Object* sender) {
            selectedCharacterName = characterName;

            if (selectedCharacterItem && selectedCharacterItem != sender)
            {
                if (auto* oldBorder = selectedCharacterItem->getChildByName("border"))
                    oldBorder->setVisible(false);
            }
            if (selectedCharacterItem)
                selectedCharacterItem->setEnabled(true);
            selectedCharacterItem = dynamic_cast<MenuItemSprite*>(sender);
            selectedCharacterItem->setEnabled(false);
            if (auto* border = selectedCharacterItem->getChildByName("border"))
                border->setVisible(true);

            bool isAvailable     = false;
            const auto& entities = GameData::getInstance()->getEntityTemplates();
            if (entities.find("player") != entities.end() &&
                entities.at("player").find(characterName) != entities.at("player").end())
            {
                isAvailable = entities.at("player").at(characterName).available;
            }
            else
            {
                AXLOG("Lỗi: Không tìm thấy nhân vật %s trong entityTemplates", characterName.c_str());
            }

            updateCharacterStats(characterName, panelDescription, isAvailable);
            Utils::updateItemUI(selectedCharacterItem, panelDescription, isAvailable);
            buyButton->setVisible(!isAvailable);
            nextButton->setVisible(isAvailable);
        });

        button->setUserData(characterLabel);

        int col            = index % cols;
        int row            = index / cols;
        float buttonWidth  = button->getContentSize().width;
        float buttonHeight = button->getContentSize().height;
        button->setPosition(
            Vec2(startX + col * (buttonWidth + iconSpacingX), startY - row * (buttonHeight + iconSpacingY)));
        menuItems.pushBack(button);
    }
}

void CharacterChooseScene::updateCoinLabel(int newCoinValue)
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
