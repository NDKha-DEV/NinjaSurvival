#include "MovementSystem.h"

#include "JoystickSystem.h"
#include "SpawnSystem.h"
#include "MapSystem.h"
#include "CollisionSystem.h"
#include "PickupSystem.h"
#include "CleanupSystem.h"
#include "WeaponSystem.h"
#include "SystemManager.h"
#include "Utils.h"

MovementSystem::MovementSystem(EntityManager& em,
                               ComponentManager<IdentityComponent>& im,
                               ComponentManager<TransformComponent>& tm,
                               ComponentManager<VelocityComponent>& vm,
                               ComponentManager<AnimationComponent>& am,
                               ComponentManager<HitboxComponent>& hm,
                               ComponentManager<SpeedComponent>& spm,
                               ComponentManager<CooldownComponent>& cdm)
    : entityManager(em)
    , identityMgr(im)
    , transformMgr(tm)
    , velocityMgr(vm)
    , animationMgr(am)
    , hitboxMgr(hm)
    , speedMgr(spm)
    , cooldownMgr(cdm)
{
    // Khai báo các kiểu di chuyển
    // Kiểu di chuyển của player
    movementStrategies["player"]        = [this](Entity e, float dt) { movePlayer(e, dt); };
    // Enemy_Slime_Bear_Snake
    movementStrategies["enemy_Slime"]   = [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
    movementStrategies["enemy_Bear"]    = [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
    movementStrategies["enemy_Snake"]   = [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
    movementStrategies["enemy_Blue Bat"]= [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
    movementStrategies["enemy_Eye"]     = [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
    movementStrategies["enemy_Kappa"]   = [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
    movementStrategies["enemy_Reptile"] = [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
    // Enemy_Octopus
    movementStrategies["enemy_Octopus"] = [this](Entity e, float dt) { moveRangedEnemy(e, dt); };
    // Boss
    movementStrategies["boss"]          = [this](Entity e, float dt) { moveMeleeEnemy(e, dt); };
}

void MovementSystem::init()
{
    //Tạo batch để cập nhật di chuyển cho entity
    initializeBatches();

    //Khởi tạo weaponMovementSystem
    weaponMovementSystem = std::make_unique<WeaponMovementSystem>(entityManager, identityMgr, transformMgr, velocityMgr,
                                                              animationMgr, hitboxMgr, speedMgr);
}

// update toàn bộ hệ thống di chuyển, gọi mỗi frame
void MovementSystem::update(float dt)
{
    auto start = std::chrono::high_resolution_clock::now();
    // Lấy hệ thống CollisionSystem từ SystemManager
    auto collisionSystem = SystemManager::getInstance()->getSystem<CollisionSystem>();
    if (!collisionSystem)
        return;// Thoát nếu không có CollisionSystem

    // Xử lý player và boss
    auto spawnSystem = SystemManager::getInstance()->getSystem<SpawnSystem>();
    if (spawnSystem)
    {
        // Xử lý player
        Entity player = spawnSystem->getPlayerEntity();
        updateEntityMovement(player, dt);

        // Xử lý boss
        auto bossList = spawnSystem->getBossList();
        if (!bossList.empty())
        {
            for (auto& boss : spawnSystem->getBossList())
            {
                updateEntityMovement(boss, dt);
            }
        }
    }

    // Xử lý một batch enemy mỗi frame
    if (!enemyBatches.empty())
    {
        processBatch(currentBatchIndex, dt);
        currentBatchIndex = (currentBatchIndex + 1) % BATCH_COUNT;  // Chuyển sang batch tiếp theo
    }

    // Xử lý move của weapon
    weaponMovementSystem->update(dt);

    // Nhặt item
    moveItem(dt);

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //AXLOG("Thời gian thực thi MovementSystem: %ld ms", duration);
}

// Hàm cập nhật di chuyển cho entity
void MovementSystem::updateEntityMovement(Entity entity, float dt)
{
    auto transform = transformMgr.getComponent(entity);
    auto velocity  = velocityMgr.getComponent(entity);
    if (!transform || !velocity)
        return;

    // Xác định key để tìm kiểu di chuyển
    std::string key;
    if (auto identity = identityMgr.getComponent(entity))
    {
        // Nếu là player hoặc item, dùng type làm key; nếu không, kết hợp type và name (cần update thêm, viết lại cho rõ khi thêm boss, projectile)
        key = (Utils::in(identity->type, "player", "item", "boss")) ? identity->type : identity->type + "_" + identity->name;

        if (Utils::not_in(key, "item", "weapon_melee", "weapon_projectile", "enemy_projectile"))  // Không cần chạy với key này
        {
            // Tìm hàm di chuyển qua key
            auto it = movementStrategies.find(key);
            if (it != movementStrategies.end())
            {
                // Gọi hàm di chuyển tương ứng
                it->second(entity, dt);
            }
        }
    }
}

// Xử lý sub-stepping và đặt vị trí mới
ax::Vec2 MovementSystem::subSteppingHandle(Entity entity, float dt)
{
    // Sub-stepping
    ax::Vec2 currentPos = ax::Vec2(transformMgr.getComponent(entity)->x, transformMgr.getComponent(entity)->y);  // Lấy vị trí hiện tại
    ax::Vec2 moveStep   = ax::Vec2(velocityMgr.getComponent(entity)->vx, velocityMgr.getComponent(entity)->vy) * dt;  // Một bước di chuyển (Vec2) trong 1 frame

    // Di chuyển trực tiếp nếu vận tốc nhỏ
    ax::Vec2 newPos = currentPos + moveStep;
    newPos          = SystemManager::getInstance()->getSystem<CollisionSystem>()->resolvePosition(entity, newPos);
    transformMgr.getComponent(entity)->x = newPos.x;
    transformMgr.getComponent(entity)->y = newPos.y;
    return newPos;
}

// Logic di chuyển của player (tính toán velocity và đặt animation state)
void MovementSystem::movePlayer(Entity entity, float dt)
{
    auto transform      = transformMgr.getComponent(entity);
    auto velocity       = velocityMgr.getComponent(entity);
    auto animation      = animationMgr.getComponent(entity);
    auto speed          = speedMgr.getComponent(entity);
    auto joystickSystem = SystemManager::getInstance()->getSystem<JoystickSystem>();

    if (!transform || !velocity || !joystickSystem || !speed)
        return; // Thoát nếu thiếu

    // Lấy hướng từ JoystickSystem và tính vận tốc
    ax::Vec2 joystickDirection = joystickSystem->getDirection();
    velocity->vx               = joystickDirection.x * speed->speed;
    velocity->vy               = joystickDirection.y * speed->speed;

    if (animation)
    {
        // Tính góc di chuyển từ hướng joystick (độ sang radian)
        // Chuyển đổi vector direction thành hệ góc (-180;180)
        float angle = AX_RADIANS_TO_DEGREES(atan2(joystickDirection.y, joystickDirection.x));
        if (angle < 0)
            angle += 360.0f;  // Nếu là góc âm chuyển thành hệ góc (0;360)

        // Kiểm tra góc cho "moveDown" (255° đến 285°)
        if (angle >= 255.0f && angle <= 285.0f)
            animation->currentState = "moveDown";
        else if (velocity->vx < 0)
            animation->currentState = "moveLeft";
        else if (velocity->vx > 0)
            animation->currentState = "moveRight";
        else  // Mặc định (= 0)
            animation->currentState = "idle";
    }

    // Xử lý sub-stepping và đặt vị trí mới
    ax::Vec2 newPos = subSteppingHandle(entity, dt);

    //Kiểm tra chunk và cập nhật khi player di chuyển
    SystemManager::getInstance()->getSystem<MapSystem>()->onPlayerPositionChanged(newPos);
}

// Logic di chuyển cho enemy loại melee enemy
void MovementSystem::moveMeleeEnemy(Entity entity, float dt)
{
    auto velocity  = velocityMgr.getComponent(entity);
    auto transform = transformMgr.getComponent(entity);
    auto speed     = speedMgr.getComponent(entity);
    auto animation = animationMgr.getComponent(entity);
    if (!velocity || !transform || !speed)
        return;

    //Re-position nếu quá xa
    isOutOfView(entity);

    auto spawnSystem = SystemManager::getInstance()->getSystem<SpawnSystem>();
    if (spawnSystem)
    {
        ax::Vec2 playerPos = spawnSystem->getPlayerPosition();
        ax::Vec2 slimePos(transform->x, transform->y);
        ax::Vec2 direction = playerPos - slimePos;
        direction.normalize();

        velocity->vx = direction.x * speed->speed;
        velocity->vy = direction.y * speed->speed;

        if (auto animation = animationMgr.getComponent(entity))
        {
            // Tính góc di chuyển từ hướng joystick (độ sang radian)
            // Chuyển đổi vector direction thành hệ góc (-180;180)
            float angle = AX_RADIANS_TO_DEGREES(atan2(direction.y, direction.x));
            if (angle < 0)
                angle += 360.0f;  // Nếu là góc âm chuyển thành hệ góc (0;360)

            // Kiểm tra góc cho "moveDown" (255° đến 285°)
            if (angle >= 255.0f && angle <= 285.0f)
                animation->currentState = "moveDown";
            else if (velocity->vx < 0)
                animation->currentState = "moveLeft";
            else if (velocity->vx > 0)
                animation->currentState = "moveRight";
            else  // Mặc định (= 0)
                animation->currentState = "idle";
        }
    }
    
    // Xử lý sub-stepping và đặt vị trí mới
    subSteppingHandle(entity, dt);
}

// Logic di chuyển cho enemy loại ranged enemy
void MovementSystem::moveRangedEnemy(Entity entity, float dt)
{
    auto velocity  = velocityMgr.getComponent(entity);
    auto transform = transformMgr.getComponent(entity);
    auto speed     = speedMgr.getComponent(entity);
    auto animation = animationMgr.getComponent(entity);
    if (!velocity || !transform || !speed)
        return;

    // Re-position nếu quá xa
    isOutOfView(entity);

    // Timer kiểm tra vị trí (lưu trong MovementSystem)
    timers[entity] += (dt * BATCH_COUNT);

    // Lấy vị trí player từ SpawnSystem
    auto spawnSystem = SystemManager::getInstance()->getSystem<SpawnSystem>();
    if (!spawnSystem)
        return;

    ax::Vec2 playerPos = spawnSystem->getPlayerPosition();
    ax::Vec2 enemyPos(transform->x, transform->y);

    // Kiểm tra vị trí mỗi x giây
    if (timers[entity] >= 2.0f)
    {
        float currentDistance = playerPos.distance(enemyPos);

        // Nếu không trong vùng, chọn vị trí mới
        if (currentDistance < distance.min() || currentDistance > distance.max())
        {
            // Chọn khoảng cách ngẫu nhiên trong
            float targetDistance = distance(gen);

            // Tính vector hướng từ enemy đến player
            ax::Vec2 direction = playerPos - enemyPos;
            direction.normalize();

            // Tính vị trí mục tiêu
            ax::Vec2 targetPos = playerPos - direction * targetDistance;

            // Tính hướng di chuyển đến targetPos
            ax::Vec2 moveDirection = targetPos - enemyPos;
            moveDirection.normalize();

            // Cập nhật vận tốc
            velocity->vx = moveDirection.x * speed->speed;
            velocity->vy = moveDirection.y * speed->speed;
        }
        else // Trong vùng, dừng lại
        {   
            velocity->vx = 0.0f;
            velocity->vy = 0.0f;
            if (cooldownMgr.getComponent(entity)->cooldownTimer <= 0)
            {
                SystemManager::getInstance()->getSystem<WeaponSystem>()->createEnemyProjectile("energy_ball", entity);
                cooldownMgr.getComponent(entity)->cooldownTimer = cooldownMgr.getComponent(entity)->cooldownDuration;
            }
            
        }
        timers[entity] = 0.0f;  // Reset timer
    }

    // Xử lý animation
    if (animation)
    {
        if (velocity->vx == 0.0f && velocity->vy == 0.0f)
        {
            animation->currentState = "idle";
        }
        else
        {
            ax::Vec2 moveDirection = ax::Vec2(velocity->vx, velocity->vy);
            moveDirection.normalize();
            float angle = AX_RADIANS_TO_DEGREES(atan2(moveDirection.y, moveDirection.x));
            if (angle < 0)
                angle += 360.0f;

            if (angle >= 255.0f && angle <= 285.0f)
                animation->currentState = "moveDown";
            else if (velocity->vx < 0)
                animation->currentState = "moveLeft";
            else if (velocity->vx > 0)
                animation->currentState = "moveRight";
        }
    }

    //Xử lý sub-stepping và đặt vị trí mới
    subSteppingHandle(entity, dt);
}

// Logic di chuyển cho item
void MovementSystem::moveItem(float dt)
{
    std::vector<Entity> toRemove;
    auto pickupSystem = SystemManager::getInstance()->getSystem<PickupSystem>();
    if (!pickupSystem)
        return;

    for (auto& item : lootedItems)
    {
        auto itemTransform   = transformMgr.getComponent(item);
        auto playerTransform = transformMgr.getComponent(SystemManager::getInstance()->getSystem<SpawnSystem>()->getPlayerEntity());
        if (!itemTransform || !playerTransform)
            continue;

        ax::Vec2 itemPos(itemTransform->x, itemTransform->y);
        ax::Vec2 playerPos(playerTransform->x, playerTransform->y);
        ax::Vec2 direction = (playerPos - itemPos).getNormalized();
        float distance     = itemPos.distance(playerPos);

        const float speed = 250.0f;
        if (distance > 5.0f)  // Ngưỡng dừng
        {
            itemTransform->x += direction.x * speed * dt;
            itemTransform->y += direction.y * speed * dt;
        }

        if (distance <= 5.0f)
        {
            auto identity = identityMgr.getComponent(item);
            if (identity)
            {
                pickupSystem->applyPickupEffect(identity->name);
            }
            SystemManager::getInstance()->getSystem<CleanupSystem>()->destroyItem(item);
            toRemove.push_back(item);
        }
    }

    //Xóa item đã nhặt trong list các item đang nhặt
    for (auto item : toRemove)
    {
        lootedItems.erase(item);
    }
}

//Kiểm tra xem entity có quá xa view không
bool MovementSystem::isOutOfView(Entity entity)
{
    auto transform = transformMgr.getComponent(entity);
    if (!transform)
        return false;

    //Player & enemy position
    ax::Vec2 playerPos = SystemManager::getInstance()->getSystem<SpawnSystem>()->getPlayerPosition();
    ax::Vec2 enemyPos(transform->x, transform->y);

    // Check khoảng cách giữa player & enemy
    float distance = playerPos.distance(enemyPos);
    if (distance > 950.0f)
    {
        // Re-position
        ax::Vec2 newEnemyPos =
            SystemManager::getInstance()->getSystem<SpawnSystem>()->getRandomSpawnPosition(entity, playerPos);
        transform->x = newEnemyPos.x;
        transform->y = newEnemyPos.y;
        return true;
    }
    return false;
}

//Thêm item được nhặt vào danh sách
void MovementSystem::moveItemToPlayer(Entity item)
{
    lootedItems.insert(item);  // Thêm item vào danh sách nhặt
}

// Tạo Batch cho các entity
void MovementSystem::initializeBatches()
{
    for (int i = 0; i < BATCH_COUNT; ++i)
    {
        enemyBatches[i]        = std::vector<Entity>();
        BatchScore* batchScore = new BatchScore(i, 0);
        batchScoreMap[i]       = batchScore;
        batchQueue.push(*batchScore);
    }
}

//Thêm entity vào batch
void MovementSystem::assignEntityToBatch(Entity entity)
{
    auto identity = identityMgr.getComponent(entity);
    if (!identity || identity->type != "enemy")
        return;

    int batchId = getBestBatch();
    enemyBatches[batchId].push_back(entity);
}

int MovementSystem::getBestBatch()
{
    if (batchQueue.empty())
        return 0;

    BatchScore leastLoaded = batchQueue.top();
    batchQueue.pop();
    leastLoaded.score += 1;
    batchQueue.push(leastLoaded);
    return leastLoaded.batchId;
}

void MovementSystem::updateBatchOnDeath(Entity entity)
{
    for (auto& batch : enemyBatches)
    {
        auto it = std::find(batch.second.begin(), batch.second.end(), entity);
        if (it != batch.second.end())
        {
            batch.second.erase(it);
            BatchScore* batchScore = batchScoreMap[batch.first];
            batchScore->score -= 1;

            // Cập nhật batchQueue (tái tạo vì std::priority_queue không hỗ trợ cập nhật trực tiếp)
            std::vector<BatchScore> temp;
            while (!batchQueue.empty())
            {
                temp.push_back(batchQueue.top());
                batchQueue.pop();
            }
            for (const auto& bs : temp)
                batchQueue.push(bs);
            break;
        }
    }
}

void MovementSystem::processBatch(int batchId, float dt)
{
    for (Entity entity : enemyBatches[batchId])
    {
        updateEntityMovement(entity, dt);
    }
}
