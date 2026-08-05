#include "lib.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace hk::hpbar {

// Hollow Knight 1.4.3.2b (0100633007D48800 v262144).
// RVAs were recovered from main + global-metadata.dat.
constexpr std::ptrdiff_t kHealthManagerTakeDamageRva = 0x163F50;
constexpr std::ptrdiff_t kHealthManagerApplyExtraDamageRva = 0x1656B0;
constexpr std::ptrdiff_t kHealthManagerOnDisableRva = 0x1630C0;
constexpr std::ptrdiff_t kHeroControllerDieRva = 0x172200;
constexpr std::ptrdiff_t kHeroControllerDieFromHazardRva = 0x172270;
constexpr std::ptrdiff_t kHealthManagerHpOffset = 0xE8;
constexpr std::ptrdiff_t kInputHandlerOnGuiRva = 0xD21E0;
constexpr std::ptrdiff_t kGuiDrawTextureRva = 0xB01230;
constexpr std::ptrdiff_t kGuiGetColorRva = 0xAFF190;
constexpr std::ptrdiff_t kGuiSetColorRva = 0xAFF2C0;
constexpr std::ptrdiff_t kGuiLabelRva = 0xB00AB0;
constexpr std::ptrdiff_t kTexture2DGetWhiteTextureRva = 0xBE4B80;
constexpr std::ptrdiff_t kScreenGetWidthRva = 0xBDC3D0;
constexpr std::ptrdiff_t kScreenGetHeightRva = 0xBDC440;
constexpr std::ptrdiff_t kComponentGetTransformRva = 0x981290;
constexpr std::ptrdiff_t kComponentGetGameObjectRva = 0x981300;
constexpr std::ptrdiff_t kObjectGetNameRva = 0x9984A0;
constexpr std::ptrdiff_t kObjectGetInstanceIdRva = 0x99B450;
constexpr std::ptrdiff_t kTransformGetPositionRva = 0xBE6F00;
constexpr std::ptrdiff_t kStringCreateFromUtf8Rva = 0x8C9ED0;
constexpr std::ptrdiff_t kFileWriteAllTextRva = 0x913500;
constexpr std::ptrdiff_t kApplicationPersistentDataPathRva = 0x97A690;
constexpr std::ptrdiff_t kCameraWorldToScreenPointRva = 0x97ECB0;
constexpr std::ptrdiff_t kCameraGetMainRva = 0x97F320;
constexpr std::ptrdiff_t kObjectOpImplicitRva = 0x98CC10;
constexpr int kMinimumInitialHpForBar = 50;
constexpr std::ptrdiff_t kHealthManagerEnemyTypeOffset = 0xEC;
constexpr const char* kTelemetryFileName = "/hk_hpbar_telemetry.csv";

struct Rect {
    float x;
    float y;
    float width;
    float height;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

struct Vector3 {
    float x;
    float y;
    float z;
};

struct HitInstance {
    void* source;
    std::int32_t attackType;
    bool circleDirection;
    std::uint8_t padding0[3];
    std::int32_t damageDealt;
    float direction;
    bool ignoreInvulnerable;
    std::uint8_t padding1[3];
    float magnitudeMultiplier;
    float moveAngle;
    bool moveDirection;
    std::uint8_t padding2[3];
    float multiplier;
    std::int32_t specialType;
    bool isExtraDamage;
};

struct Il2CppString;
using StringCreateFromUtf8Fn = Il2CppString* (*)(void* thisObject,
                                                 const char* value,
                                                 const void* methodInfo);
using FileWriteAllTextFn = void (*)(void* thisObject, Il2CppString* path,
                                    Il2CppString* contents,
                                    const void* methodInfo);
using GetPersistentDataPathFn = Il2CppString* (*)(void* thisObject,
                                                  const void* methodInfo);

struct Il2CppString {
    std::uintptr_t objectHeader[2];
    std::int32_t length;
    char16_t chars[1];
};

using GuiDrawTextureFn = void (*)(void* thisObject, Rect position,
                                  void* texture, const void* methodInfo);
using GuiGetColorFn = Color (*)(void* thisObject, const void* methodInfo);
using GuiSetColorFn = void (*)(void* thisObject, Color value,
                               const void* methodInfo);
using GuiLabelFn = void (*)(void* thisObject, Rect position,
                            Il2CppString* text, const void* methodInfo);
using GetWhiteTextureFn = void* (*)(void* thisObject,
                                    const void* methodInfo);
using ScreenGetDimensionFn = int (*)(void* thisObject,
                                     const void* methodInfo);
using ComponentGetObjectFn = void* (*)(void* self, const void* methodInfo);
using ObjectGetNameFn = Il2CppString* (*)(void* self,
                                         const void* methodInfo);
using ObjectGetInstanceIdFn = int (*)(void* self, const void* methodInfo);
using TransformGetPositionFn = Vector3 (*)(void* self,
                                           const void* methodInfo);
using CameraGetMainFn = void* (*)(void* thisObject, const void* methodInfo);
using CameraWorldToScreenPointFn = Vector3 (*)(void* camera,
                                                Vector3 position,
                                                const void* methodInfo);
using ObjectExistsFn = bool (*)(void* thisObject, void* object,
                                const void* methodInfo);

struct EnemySlot {
    std::uintptr_t manager;
    Vector3 screenPosition;
    int instanceId;
    int hp;
    int maxHp;
    std::uint64_t lastHitTick;
    bool active;
};

struct DamagePopup {
    Vector3 screenPosition;
    Il2CppString* text;
    std::uint64_t startTick;
    bool active;
};

constexpr std::size_t kEnemySlotCount = 32;
constexpr std::uint64_t kSystemTicksPerSecond = 19'200'000ULL;
constexpr std::uint64_t kEnemyBarLifetimeTicks =
    kSystemTicksPerSecond * 6ULL;
constexpr std::size_t kDamagePopupCount = 16;
constexpr std::uint64_t kDamagePopupLifetimeTicks =
    kSystemTicksPerSecond * 14ULL / 10ULL;

std::uintptr_t g_lastHitHealthManager = 0;
int g_hpBeforeHit = 0;
int g_hpAfterHit = 0;
int g_observedMaxHp = 0;
std::uint32_t g_hitCount = 0;
bool g_telemetryReady = false;
int g_telemetryStage = 0;
Result g_telemetryResult = 0;
char g_telemetryPath[512]{};
char g_telemetryBuffer[64 * 1024]{};
std::size_t g_telemetryLength = 0;
Il2CppString* g_lastEnemyName = nullptr;
Il2CppString* g_lastEnemyStats = nullptr;
Il2CppString* g_bossHpText = nullptr;
int g_lastEnemyInstanceId = 0;
int g_lastEnemyType = 0;
EnemySlot g_enemySlots[kEnemySlotCount]{};
DamagePopup g_damagePopups[kDamagePopupCount]{};

void ClearCombatHud() {
    g_lastHitHealthManager = 0;
    g_hpBeforeHit = 0;
    g_hpAfterHit = 0;
    g_observedMaxHp = 0;
    g_hitCount = 0;
    g_lastEnemyName = nullptr;
    g_lastEnemyStats = nullptr;
    g_bossHpText = nullptr;
    g_lastEnemyInstanceId = 0;
    g_lastEnemyType = 0;
    for (auto& slot : g_enemySlots) slot = EnemySlot{};
    for (auto& popup : g_damagePopups) popup = DamagePopup{};
}

template <typename Fn>
Fn MainFunction(std::ptrdiff_t rva) {
    return reinterpret_cast<Fn>(
        exl::util::modules::GetTargetOffset(rva)
    );
}

void CopyManagedName(Il2CppString* value, char* output,
                     std::size_t outputSize) {
    if (outputSize == 0) return;
    output[0] = '\0';
    if (value == nullptr || value->length <= 0) return;

    std::size_t count = static_cast<std::size_t>(value->length);
    if (count >= outputSize) count = outputSize - 1;
    for (std::size_t i = 0; i < count; ++i) {
        const char16_t c = value->chars[i];
        output[i] = (c >= 0x20 && c <= 0x7E && c != ',')
            ? static_cast<char>(c)
            : '_';
    }
    output[count] = '\0';
}

void InitializeTelemetry() {
    g_telemetryReady = true;
    g_telemetryStage = 7;
}

void WriteTelemetry(void* healthManager, int hpBefore, int hpAfter,
                    int reportedDamage, int attackType, int specialType,
                    void* source) {
    if (healthManager == nullptr) return;

    auto getGameObject = MainFunction<ComponentGetObjectFn>(
        kComponentGetGameObjectRva
    );
    auto getName = MainFunction<ObjectGetNameFn>(kObjectGetNameRva);
    auto getInstanceId = MainFunction<ObjectGetInstanceIdFn>(
        kObjectGetInstanceIdRva
    );

    void* gameObject = getGameObject(healthManager, nullptr);
    const auto address = reinterpret_cast<std::uintptr_t>(healthManager);
    g_lastEnemyName = gameObject ? getName(gameObject, nullptr) : nullptr;
    g_lastEnemyInstanceId = gameObject
        ? getInstanceId(gameObject, nullptr) : 0;
    g_lastEnemyType = *reinterpret_cast<volatile int*>(
        address + kHealthManagerEnemyTypeOffset
    );

    char stats[160];
    std::snprintf(stats, sizeof(stats),
                  "ID %d | TYPE %d | HP %d/%d | HIT %d | RAW %d | ATK %d",
                  g_lastEnemyInstanceId, g_lastEnemyType, hpAfter,
                  g_observedMaxHp, hpBefore - hpAfter, reportedDamage,
                  attackType);
    auto createString = MainFunction<StringCreateFromUtf8Fn>(
        kStringCreateFromUtf8Rva);
    g_lastEnemyStats = createString(nullptr, stats, nullptr);
    char hpText[48];
    std::snprintf(hpText, sizeof(hpText), "%d / %d", hpAfter,
                  g_observedMaxHp);
    g_bossHpText = createString(nullptr, hpText, nullptr);
}

void UpdateEnemySlot(void* healthManager, int hpBefore, int hpAfter) {
    if (healthManager == nullptr || g_lastEnemyType != 0) return;
    const auto manager = reinterpret_cast<std::uintptr_t>(healthManager);
    EnemySlot* slot = nullptr;
    EnemySlot* freeSlot = nullptr;
    for (auto& candidate : g_enemySlots) {
        if (candidate.active && candidate.manager == manager) {
            slot = &candidate;
            break;
        }
        if (!candidate.active && freeSlot == nullptr) freeSlot = &candidate;
    }
    if (slot == nullptr) slot = freeSlot ? freeSlot : &g_enemySlots[0];

    auto getTransform = MainFunction<ComponentGetObjectFn>(
        kComponentGetTransformRva);
    auto getPosition = MainFunction<TransformGetPositionFn>(
        kTransformGetPositionRva);
    auto getMainCamera = MainFunction<CameraGetMainFn>(kCameraGetMainRva);
    auto worldToScreen = MainFunction<CameraWorldToScreenPointFn>(
        kCameraWorldToScreenPointRva);
    void* transform = getTransform(healthManager, nullptr);
    void* camera = getMainCamera(nullptr, nullptr);
    if (transform == nullptr || camera == nullptr) return;
    Vector3 world = getPosition(transform, nullptr);
    world.y += 1.15f;
    slot->manager = manager;
    slot->screenPosition = worldToScreen(camera, world, nullptr);
    slot->instanceId = g_lastEnemyInstanceId;
    slot->hp = hpAfter;
    slot->lastHitTick = svcGetSystemTick();
    if (!slot->active) slot->maxHp = hpBefore > hpAfter ? hpBefore : hpAfter;
    if (hpBefore > slot->maxHp) slot->maxHp = hpBefore;
    if (hpAfter > slot->maxHp) slot->maxHp = hpAfter;
    slot->active = hpAfter > 0 && slot->screenPosition.z > 0.0f;
}

void AddDamagePopup(void* healthManager, int effectiveDamage) {
    if (healthManager == nullptr || effectiveDamage <= 0) return;
    auto getTransform = MainFunction<ComponentGetObjectFn>(
        kComponentGetTransformRva);
    auto getPosition = MainFunction<TransformGetPositionFn>(
        kTransformGetPositionRva);
    auto getMainCamera = MainFunction<CameraGetMainFn>(kCameraGetMainRva);
    auto worldToScreen = MainFunction<CameraWorldToScreenPointFn>(
        kCameraWorldToScreenPointRva);
    void* transform = getTransform(healthManager, nullptr);
    void* camera = getMainCamera(nullptr, nullptr);
    if (transform == nullptr || camera == nullptr) return;
    Vector3 world = getPosition(transform, nullptr);
    world.y += 0.45f;
    const Vector3 screen = worldToScreen(camera, world, nullptr);
    if (screen.z <= 0.0f) return;

    DamagePopup* popup = nullptr;
    for (auto& candidate : g_damagePopups) {
        if (!candidate.active) {
            popup = &candidate;
            break;
        }
        if (popup == nullptr || candidate.startTick < popup->startTick) {
            popup = &candidate;
        }
    }
    char damageText[24];
    std::snprintf(damageText, sizeof(damageText), "-%d", effectiveDamage);
    auto createString = MainFunction<StringCreateFromUtf8Fn>(
        kStringCreateFromUtf8Rva);
    popup->screenPosition = screen;
    popup->text = createString(nullptr, damageText, nullptr);
    popup->startTick = svcGetSystemTick();
    popup->active = popup->text != nullptr;
}

int ReadHp(void* healthManager) {
    if (healthManager == nullptr) {
        return 0;
    }

    const auto address = reinterpret_cast<std::uintptr_t>(healthManager);
    return *reinterpret_cast<volatile int*>(address + kHealthManagerHpOffset);
}

void RecordDamage(void* healthManager, int hpBefore, int hpAfter,
                  int reportedDamage, int attackType, int specialType,
                  void* source) {
    if (g_telemetryStage == 0) {
        InitializeTelemetry();
    }

    const auto manager = reinterpret_cast<std::uintptr_t>(healthManager);
    if (manager != g_lastHitHealthManager) {
        g_lastHitHealthManager = manager;
        g_observedMaxHp = hpBefore > hpAfter ? hpBefore : hpAfter;
    } else if (hpBefore > g_observedMaxHp || hpAfter > g_observedMaxHp) {
        g_observedMaxHp = hpBefore > hpAfter ? hpBefore : hpAfter;
    }

    g_hpBeforeHit = hpBefore;
    g_hpAfterHit = hpAfter;
    ++g_hitCount;
    WriteTelemetry(healthManager, hpBefore, hpAfter, reportedDamage,
                   attackType, specialType, source);
    UpdateEnemySlot(healthManager, hpBefore, hpAfter);
    AddDamagePopup(healthManager, hpBefore - hpAfter);

    Logging.Log(
        "[hk-boss-hp] hit=%u manager=%p hp=%d->%d",
        static_cast<unsigned>(g_hitCount),
        healthManager,
        hpBefore,
        hpAfter
    );
}

HOOK_DEFINE_TRAMPOLINE(HealthManagerTakeDamageHook) {
    // IL2CPP passes the large HitInstance value indirectly on AArch64. The
    // third argument is the hidden MethodInfo pointer.
    static void Callback(void* self, void* hitInstance, const void* methodInfo) {
        const HitInstance hit = hitInstance
            ? *reinterpret_cast<const HitInstance*>(hitInstance)
            : HitInstance{};
        const int hpBefore = ReadHp(self);
        Orig(self, hitInstance, methodInfo);
        const int hpAfter = ReadHp(self);

        RecordDamage(self, hpBefore, hpAfter, hit.damageDealt,
                     hit.attackType, hit.specialType, hit.source);
    }
};

HOOK_DEFINE_TRAMPOLINE(HealthManagerApplyExtraDamageHook) {
    static void Callback(void* self, int damageAmount,
                         const void* methodInfo) {
        const int hpBefore = ReadHp(self);
        Orig(self, damageAmount, methodInfo);
        RecordDamage(self, hpBefore, ReadHp(self), damageAmount,
                     -1, -1, nullptr);
    }
};

HOOK_DEFINE_TRAMPOLINE(HealthManagerOnDisableHook) {
    static void Callback(void* self, const void* methodInfo) {
        Orig(self, methodInfo);
        if (reinterpret_cast<std::uintptr_t>(self) ==
            g_lastHitHealthManager) {
            g_lastHitHealthManager = 0;
            g_lastEnemyName = nullptr;
            g_lastEnemyStats = nullptr;
            g_bossHpText = nullptr;
            g_lastEnemyInstanceId = 0;
            g_lastEnemyType = 0;
            g_hpBeforeHit = 0;
            g_hpAfterHit = 0;
            g_observedMaxHp = 0;
        }
        const auto manager = reinterpret_cast<std::uintptr_t>(self);
        for (auto& slot : g_enemySlots) {
            if (slot.active && slot.manager == manager) slot = EnemySlot{};
        }
    }
};

HOOK_DEFINE_TRAMPOLINE(HeroControllerDieHook) {
    static void* Callback(void* self, const void* methodInfo) {
        ClearCombatHud();
        return Orig(self, methodInfo);
    }
};

HOOK_DEFINE_TRAMPOLINE(HeroControllerDieFromHazardHook) {
    static void* Callback(void* self, int hazardType, float angle,
                          const void* methodInfo) {
        ClearCombatHud();
        return Orig(self, hazardType, angle, methodInfo);
    }
};

HOOK_DEFINE_TRAMPOLINE(InputHandlerOnGuiHook) {
    static void Callback(void* self, const void* methodInfo) {
        Orig(self, methodInfo);

        auto drawTexture = reinterpret_cast<GuiDrawTextureFn>(
            exl::util::modules::GetTargetOffset(kGuiDrawTextureRva)
        );
        auto getColor = reinterpret_cast<GuiGetColorFn>(
            exl::util::modules::GetTargetOffset(kGuiGetColorRva)
        );
        auto setColor = reinterpret_cast<GuiSetColorFn>(
            exl::util::modules::GetTargetOffset(kGuiSetColorRva)
        );
        auto drawLabel = reinterpret_cast<GuiLabelFn>(
            exl::util::modules::GetTargetOffset(kGuiLabelRva)
        );
        auto getWhiteTexture = reinterpret_cast<GetWhiteTextureFn>(
            exl::util::modules::GetTargetOffset(kTexture2DGetWhiteTextureRva)
        );
        auto getScreenWidth = reinterpret_cast<ScreenGetDimensionFn>(
            exl::util::modules::GetTargetOffset(kScreenGetWidthRva)
        );
        auto getScreenHeight = reinterpret_cast<ScreenGetDimensionFn>(
            exl::util::modules::GetTargetOffset(kScreenGetHeightRva)
        );

        const float screenWidth = static_cast<float>(
            getScreenWidth(nullptr, nullptr)
        );
        const float screenHeight = static_cast<float>(
            getScreenHeight(nullptr, nullptr)
        );
        const float barWidth = screenWidth * 0.62f;
        const float barHeight = screenHeight * 0.028f;
        const float barX = (screenWidth - barWidth) * 0.5f;
        const float barY = screenHeight - barHeight - screenHeight * 0.07f;
        float ratio = static_cast<float>(g_hpAfterHit) /
            static_cast<float>(g_observedMaxHp);
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;

        void* whiteTexture = getWhiteTexture(nullptr, nullptr);
        if (whiteTexture == nullptr) {
            return;
        }

        const Color previousColor = getColor(nullptr, nullptr);

        auto objectExists = MainFunction<ObjectExistsFn>(kObjectOpImplicitRva);
        auto getTransform = MainFunction<ComponentGetObjectFn>(
            kComponentGetTransformRva);
        auto getPosition = MainFunction<TransformGetPositionFn>(
            kTransformGetPositionRva);
        auto getMainCamera = MainFunction<CameraGetMainFn>(kCameraGetMainRva);
        auto worldToScreen = MainFunction<CameraWorldToScreenPointFn>(
            kCameraWorldToScreenPointRva);
        void* camera = getMainCamera(nullptr, nullptr);
        if (camera != nullptr) {
            constexpr float smallWidth = 72.0f;
            constexpr float smallHeight = 9.0f;
            const std::uint64_t now = svcGetSystemTick();
            for (auto& slot : g_enemySlots) {
                if (!slot.active || slot.hp <= 0 || slot.maxHp <= 0) continue;
                if (now - slot.lastHitTick > kEnemyBarLifetimeTicks) {
                    slot = EnemySlot{};
                    continue;
                }
                void* manager = reinterpret_cast<void*>(slot.manager);
                if (!objectExists(nullptr, manager, nullptr)) {
                    slot = EnemySlot{};
                    continue;
                }
                void* transform = getTransform(manager, nullptr);
                if (!objectExists(nullptr, transform, nullptr)) {
                    slot = EnemySlot{};
                    continue;
                }
                Vector3 world = getPosition(transform, nullptr);
                world.y += 1.15f;
                const Vector3 screen = worldToScreen(camera, world, nullptr);
                if (screen.z <= 0.0f) continue;
                const float x = screen.x - smallWidth * 0.5f;
                const float y = screenHeight - screen.y;
                if (x + smallWidth < 0.0f || x > screenWidth ||
                    y + smallHeight < 0.0f || y > screenHeight) continue;
                float smallRatio = static_cast<float>(slot.hp) /
                    static_cast<float>(slot.maxHp);
                if (smallRatio < 0.0f) smallRatio = 0.0f;
                if (smallRatio > 1.0f) smallRatio = 1.0f;
                setColor(nullptr, {0.02f, 0.02f, 0.025f, 0.92f}, nullptr);
                drawTexture(nullptr,
                            {x - 2.0f, y - 2.0f,
                             smallWidth + 4.0f, smallHeight + 4.0f},
                            whiteTexture, nullptr);
                setColor(nullptr, {0.18f, 0.02f, 0.025f, 0.92f}, nullptr);
                drawTexture(nullptr, {x, y, smallWidth, smallHeight},
                            whiteTexture, nullptr);
                setColor(nullptr, {0.92f, 0.03f, 0.04f, 1.0f}, nullptr);
                drawTexture(nullptr,
                            {x, y, smallWidth * smallRatio, smallHeight},
                            whiteTexture, nullptr);
            }
            setColor(nullptr, previousColor, nullptr);
        }

        const std::uint64_t popupNow = svcGetSystemTick();
        for (auto& popup : g_damagePopups) {
            if (!popup.active || popup.text == nullptr) continue;
            const std::uint64_t age = popupNow - popup.startTick;
            if (age >= kDamagePopupLifetimeTicks) {
                popup = DamagePopup{};
                continue;
            }
            const float progress = static_cast<float>(age) /
                static_cast<float>(kDamagePopupLifetimeTicks);
            const float popupX = popup.screenPosition.x - 14.0f;
            const float popupY = screenHeight - popup.screenPosition.y -
                progress * 42.0f;
            const float alpha = 1.0f - progress;
            setColor(nullptr, {0.0f, 0.0f, 0.0f, alpha * 0.8f}, nullptr);
            drawLabel(nullptr, {popupX + 1.0f, popupY + 1.0f, 80.0f, 28.0f},
                      popup.text, nullptr);
            setColor(nullptr, {1.0f, 0.92f, 0.84f, alpha}, nullptr);
            drawLabel(nullptr, {popupX, popupY, 80.0f, 28.0f},
                      popup.text, nullptr);
        }
        setColor(nullptr, previousColor, nullptr);

        if (g_hitCount == 0 || g_hpAfterHit <= 0 ||
            g_lastEnemyType != 1) {
            return;
        }

        // Subtle shadow, borderless dark track and saturated red fill.
        setColor(nullptr, {0.0f, 0.0f, 0.0f, 0.62f}, nullptr);
        drawTexture(
            nullptr,
            {barX + 3.0f, barY + 4.0f, barWidth, barHeight},
            whiteTexture,
            nullptr
        );

        setColor(nullptr, {0.018f, 0.014f, 0.018f, 0.96f}, nullptr);
        drawTexture(nullptr, {barX, barY, barWidth, barHeight},
                    whiteTexture, nullptr);

        constexpr float frame = 1.0f;
        setColor(nullptr, {0.018f, 0.014f, 0.018f, 0.96f}, nullptr);
        drawTexture(
            nullptr,
            {barX + frame, barY + frame, barWidth - frame * 2.0f,
             barHeight - frame * 2.0f},
            whiteTexture,
            nullptr
        );

        constexpr float inset = 4.0f;
        setColor(nullptr, {0.76f, 0.015f, 0.025f, 1.0f}, nullptr);
        drawTexture(
            nullptr,
            {barX + inset, barY + inset,
             (barWidth - inset * 2.0f) * ratio,
            barHeight - inset * 2.0f},
            whiteTexture,
            nullptr
        );
        if (g_lastEnemyName != nullptr) {
            const float estimatedNameWidth =
                static_cast<float>(g_lastEnemyName->length) * 7.0f;
            setColor(nullptr, {0.95f, 0.9f, 0.8f, 1.0f}, nullptr);
            drawLabel(nullptr,
                      {screenWidth * 0.5f - estimatedNameWidth * 0.5f,
                       barY - 24.0f, estimatedNameWidth + 24.0f, 24.0f},
                      g_lastEnemyName, nullptr);
        }
        if (g_bossHpText != nullptr) {
            setColor(nullptr, {1.0f, 1.0f, 1.0f, 1.0f}, nullptr);
            drawLabel(nullptr,
                      {barX + barWidth * 0.5f - 45.0f,
                       barY + barHeight * 0.5f - 8.0f,
                       120.0f, 24.0f},
                      g_bossHpText, nullptr);
        }
        setColor(nullptr, previousColor, nullptr);
    }
};

} // namespace hk::hpbar

extern "C" void exl_main(void*, void*) {
    exl::hook::Initialize();
    hk::hpbar::HealthManagerTakeDamageHook::InstallAtOffset(
        hk::hpbar::kHealthManagerTakeDamageRva
    );
    hk::hpbar::HealthManagerApplyExtraDamageHook::InstallAtOffset(
        hk::hpbar::kHealthManagerApplyExtraDamageRva
    );
    hk::hpbar::HealthManagerOnDisableHook::InstallAtOffset(
        hk::hpbar::kHealthManagerOnDisableRva
    );
    hk::hpbar::HeroControllerDieHook::InstallAtOffset(
        hk::hpbar::kHeroControllerDieRva
    );
    hk::hpbar::HeroControllerDieFromHazardHook::InstallAtOffset(
        hk::hpbar::kHeroControllerDieFromHazardRva
    );
    hk::hpbar::InputHandlerOnGuiHook::InstallAtOffset(
        hk::hpbar::kInputHandlerOnGuiRva
    );
    Logging.Log("[hk-boss-hp] initialized; HP and OnGUI hooks installed");
}

extern "C" NORETURN void exl_exception_entry() {
    EXL_ABORT("hk-boss-hp exception");
}
