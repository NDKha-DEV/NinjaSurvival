#ifndef __WEAPON_MOVEMENT_SYSTEM_H__
#define __WEAPON_MOVEMENT_SYSTEM_H__

#include "System.h"
#include "components/ComponentManager.h"
#include "entities/EntityManager.h"

class WeaponMovementSystem : public System
{
public:
    WeaponMovementSystem(EntityManager& em,
                         ComponentManager<IdentityComponent>& im,
                         ComponentManager<TransformComponent>& tm,
                         ComponentManager<VelocityComponent>& vm,
                         ComponentManager<AnimationComponent>& am,
                         ComponentManager<HitboxComponent>& hm,
                         ComponentManager<SpeedComponent>& spm);

    void update(float dt) override;
    void init() override;

    // Hàm tính lại góc cho tất cả shuriken
    void recalculateShurikenAngles(const std::vector<Entity>& shurikenList);
    void moveSpinnerWeapon(Entity entity, float dt);
    void moveLightningScrollWeapon(Entity entity, float dt);

private:
    EntityManager& entityManager;
    ComponentManager<IdentityComponent>& identityMgr;
    ComponentManager<TransformComponent>& transformMgr;
    ComponentManager<VelocityComponent>& velocityMgr;
    ComponentManager<AnimationComponent>& animationMgr;
    ComponentManager<HitboxComponent>& hitboxMgr;
    ComponentManager<SpeedComponent>& speedMgr;

    // Định nghĩa một kiểu hàm MoveFunc nhận Entity và float, trả về void
    // std::function khai báo kiểu hàm với tên bất kỳ có 2 tham số và kiểu trả về là void
    using MoveFunc = std::function<void(Entity, float)>;
    // Một map lưu cặp key(kiểu string entity type) và value(kiểu MoveFunc)
    std::unordered_map<std::string, MoveFunc> movementStrategies;

    // Gọi update với key type
    void updateEntityMovement(Entity entity, float dt);

    // Xử lý sub-stepping
    ax::Vec2 subSteppingHandle(Entity entity, float dt);

    // Các hàm di chuyển của weapon
    void moveEnemyProjectile(Entity entity, float dt);

    void moveSwordWeapon(Entity entity, float dt);
    int swordCountInFrame = 0;  // Biến tạm để xử lý hướng sword

    void moveShurikenWeapon(Entity entity, float dt);

    void moveKunaiWeapon(Entity entity, float dt);

    void moveBigKunaiWeapon(Entity entity, float dt);

    void moveExplosionKunaiWeapon(Entity entity, float dt);

    void moveNinjutsuSpellWeapon(Entity entity, float dt);
};

#endif  // __WEAPON_MOVEMENT_SYSTEM_H__
