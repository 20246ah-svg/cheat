#include "esp.h"

#include "engine.h"
#include "entity.h"

#include <algorithm>
#include <cmath>

namespace ESP {
namespace {
struct Color {
    int red;
    int green;
    int blue;
    int alpha;
};

constexpr Color kFriendlyColor{40, 220, 80, 255};
constexpr Color kEnemyColor{235, 65, 65, 255};
constexpr Color kOutlineColor{8, 8, 8, 210};
constexpr float kPlayerHeight = 72.0F;
constexpr float kMinimumClipW = 0.001F;
constexpr int kMaximumPlayerSlots = 64;

[[nodiscard]] bool Project(const Matrix4x4& matrix,
                           const Vector& world,
                           int screen_width,
                           int screen_height,
                           Vector2& screen) noexcept {
    const Vector4 clip = matrix.TransformPoint(world);
    if (clip.w <= kMinimumClipW) {
        return false;
    }

    const float inverse_w = 1.0F / clip.w;
    const float normalized_x = clip.x * inverse_w;
    const float normalized_y = clip.y * inverse_w;

    screen.x = (static_cast<float>(screen_width) * 0.5F) * (1.0F + normalized_x);
    screen.y = (static_cast<float>(screen_height) * 0.5F) * (1.0F - normalized_y);
    return std::isfinite(screen.x) && std::isfinite(screen.y);
}

void SetColor(const Color& color) noexcept {
    Engine::surface->DrawSetColor(color.red, color.green, color.blue, color.alpha);
}

void DrawPlayerBox(const Vector2& head,
                   const Vector2& feet,
                   int screen_width,
                   int screen_height,
                   const Color& color) noexcept {
    const float box_height = std::fabs(feet.y - head.y);
    if (box_height < 2.0F ||
        box_height > static_cast<float>(screen_height) * 4.0F) {
        return;
    }

    const float box_width = box_height * 0.45F;
    const int left = static_cast<int>(std::lround(feet.x - box_width * 0.5F));
    const int right = static_cast<int>(std::lround(feet.x + box_width * 0.5F));
    const int top = static_cast<int>(std::lround(std::min(head.y, feet.y)));
    const int bottom = static_cast<int>(std::lround(std::max(head.y, feet.y)));

    // A dark one-pixel border keeps the team color readable on bright maps.
    SetColor(kOutlineColor);
    Engine::surface->DrawOutlinedRect(left - 1, top - 1, right + 1, bottom + 1);
    Engine::surface->DrawOutlinedRect(left + 1, top + 1, right - 1, bottom - 1);

    SetColor(color);
    Engine::surface->DrawOutlinedRect(left, top, right, bottom);

    SetColor(kOutlineColor);
    Engine::surface->DrawLine(screen_width / 2 + 1, screen_height,
                              static_cast<int>(std::lround(feet.x)) + 1,
                              static_cast<int>(std::lround(feet.y)));
    SetColor(color);
    Engine::surface->DrawLine(screen_width / 2, screen_height,
                              static_cast<int>(std::lround(feet.x)),
                              static_cast<int>(std::lround(feet.y)));
}
} // namespace

bool WorldToScreen(const Vector& world, Vector2& screen) noexcept {
    if (!Engine::IsReady()) {
        return false;
    }

    const Matrix4x4* const matrix = Engine::GetViewMatrix();
    if (matrix == nullptr) {
        return false;
    }

    int screen_width = 0;
    int screen_height = 0;
    Engine::surface->GetScreenSize(screen_width, screen_height);
    if (screen_width <= 0 || screen_height <= 0) {
        return false;
    }

    return Project(*matrix, world, screen_width, screen_height, screen);
}

void Render() noexcept {
    if (!Engine::IsReady() || !Engine::client_engine->IsInGame()) {
        return;
    }

    CBaseEntity* const local_player = Engine::GetLocalPlayer();
    const Matrix4x4* const matrix = Engine::GetViewMatrix();
    if (local_player == nullptr || matrix == nullptr) {
        return;
    }

    const int local_team = local_player->GetTeam();
    if (local_team <= 0) {
        return;
    }

    int screen_width = 0;
    int screen_height = 0;
    Engine::surface->GetScreenSize(screen_width, screen_height);
    if (screen_width <= 0 || screen_height <= 0) {
        return;
    }

    const int highest_index = std::clamp(
        Engine::entity_list->GetHighestEntityIndex(), 0, kMaximumPlayerSlots);

    for (int index = 1; index <= highest_index; ++index) {
        CBaseEntity* const entity = Engine::GetEntityByIndex(index);
        if (entity == nullptr || entity == local_player || entity->IsDormant()) {
            continue;
        }

        const int health = entity->GetHealth();
        const int team = entity->GetTeam();
        if (health <= 0 || health > 500 || (team != 2 && team != 3)) {
            continue;
        }

        const Vector feet_world = entity->GetOrigin();
        const Vector head_world = feet_world + Vector{0.0F, 0.0F, kPlayerHeight};
        Vector2 feet_screen{};
        Vector2 head_screen{};

        if (!Project(*matrix, feet_world, screen_width, screen_height, feet_screen) ||
            !Project(*matrix, head_world, screen_width, screen_height, head_screen)) {
            continue;
        }

        const Color& color = team == local_team ? kFriendlyColor : kEnemyColor;
        DrawPlayerBox(head_screen, feet_screen, screen_width, screen_height, color);
    }
}
} // namespace ESP
