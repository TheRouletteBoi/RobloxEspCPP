#pragma once

#include "roblox.hpp"
#include "esp_controller.hpp"

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace games {

struct Target {
    roblox::Instance character;
    roblox::Vector3 aim_position;       // World position to aim at
    roblox::Vector3 screen_position;    // Screen position (x, y, depth)
    roblox::Vector3 velocity;
    roblox::Vector3 predicted_position;;
    float distance_3d;                  // World distance from local player
    float delta_distance;               // Distance from crosshair ray (studs)
    float screen_distance;
    float health;
    std::string name;
    bool is_teammate;
    bool is_valid;

    vm_address_t custom_aim_part = 0;
};

enum class AimStyle { Silent, Legit, Snap };
enum class TargetSelection { ClosestToCrosshair, ClosestDistance, LowestHealth, ClosestToMouse };
enum class AimPart { Head, Torso, Custom };
enum class AimKey { LeftMouse, RightMouse, KeyE };

struct AimSettings {
    bool enabled = true;
    float max_delta_dist = 30.0f;
    float max_distance = 400.0f;
    float smoothing = 5.0f;
    const float min_studs = 1.0f;
    const float max_studs = 50.0f;

    bool prediction_enabled = false;
    float bullet_speed = 3000.0f;
    float gravity = 196.2f;
    bool predict_gravity = true;

    AimStyle style = AimStyle::Silent;
    TargetSelection selection = TargetSelection::ClosestToCrosshair;
    AimPart aim_part = AimPart::Torso;
    std::string custom_part_name = "HumanoidRootPart";
    AimKey aim_key = AimKey::LeftMouse;
};

class GameProfile {
public:
    virtual ~GameProfile() = default;

    virtual std::string name() const = 0;
    virtual uint64_t place_id() const = 0;

    virtual bool detect(const roblox::GameContext& game) const {
        return game.place_id() == place_id();
    }

    // Override in subclass if needed
    virtual void initialize(roblox::GameContext& game) {

    }

    // Override in subclass if needed - Called every frame for game specific state updates
    virtual void update(roblox::GameContext& game) {

    }

    virtual std::vector<Target> find_targets(roblox::GameContext& game, const roblox::CFrame& camera_cf, float fov_radians) = 0;

    virtual ESPColor get_target_color(const Target& target, bool is_aim_target) const {
        if (is_aim_target) {
            return ESPColor{1.0f, 0.0f, 1.0f, 1.0f};  // Magenta for aim target
        }
        if (target.is_teammate) {
            return ESPColor{0.0f, 1.0f, 0.0f, 1.0f};  // Green for teammates
        }
        return ESPColor{1.0f, 0.0f, 0.0f, 1.0f};      // Red for enemies
    }

    virtual float get_target_border_width(const Target& target, bool is_aim_target) const {
        return is_aim_target ? 3.0f : 2.0f;
    }

    virtual AimKey default_aim_key() const { return AimKey::LeftMouse; }

    // Override in subclass if needed
    virtual void set_aim_part(const std::string& part_name) {

    }

    void set_screen_size(float width, float height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    float screen_width() const { return m_screenWidth; }
    float screen_height() const { return m_screenHeight; }

    void set_mouse_position(float x, float y) {
        m_mouse_x = x;
        m_mouse_y = y;
    }

    virtual void apply_aim(const Target& target, roblox::GameContext& game, const roblox::CFrame& camera_cf, const AimSettings& settings)
    {
        auto camera = game.camera();
        if (!camera)
            return;

        roblox::Vector3 camera_pos = camera_cf.position;
        roblox::Vector3 aim_target = target.aim_position;

        if (settings.prediction_enabled) {
            auto my_hrp = game.my_hrp();
            if (my_hrp) {
                roblox::Vector3 shooter_pos = my_hrp.position();

                if (settings.predict_gravity) {
                    aim_target = predict_position_with_gravity(
                        target.aim_position,
                        target.velocity,
                        shooter_pos,
                        settings.bullet_speed,
                        settings.gravity
                    );
                } else {
                    aim_target = predict_position(
                        target.aim_position,
                        target.velocity,
                        shooter_pos,
                        settings.bullet_speed,
                        0.0f
                    );
                }
            }
        }

        roblox::Vector3 target_direction = (aim_target - camera_pos).normalized();

        roblox::Vector3 new_look;

        switch (settings.style) {
            case AimStyle::Silent:
            case AimStyle::Snap: {
                // Instant lock
                new_look = target_direction;
                break;
            }

            case AimStyle::Legit: {
                // Smooth interpolation toward target
                roblox::Vector3 current_look = camera_cf.look_vector();
                float alpha = 1.0f / settings.smoothing;
                new_look = current_look.lerp(target_direction, alpha).normalized();
                break;
            }
        }

        // Apply new look direction to camera
        roblox::CFrame new_cf = camera_cf;
        new_cf.r20 = -new_look.x;
        new_cf.r21 = -new_look.y;
        new_cf.r22 = -new_look.z;
        camera.set_cframe(new_cf);
    }

    virtual void apply_aim_mouse(const Target& target, roblox::GameContext& game,
                             const roblox::CFrame& camera_cf, const AimSettings& settings,
                             ESPController& esp) {
        // Convert predicted aim position to screen coordinates
        roblox::Vector3 aim_target = target.aim_position;

        if (settings.prediction_enabled) {
            auto my_hrp = game.my_hrp();
            if (my_hrp) {
                roblox::Vector3 shooter_pos = my_hrp.position();
                if (settings.predict_gravity) {
                    aim_target = predict_position_with_gravity(
                        target.aim_position, target.velocity, shooter_pos,
                        settings.bullet_speed, settings.gravity
                    );
                } else {
                    aim_target = predict_position(
                        target.aim_position, target.velocity, shooter_pos,
                        settings.bullet_speed, 0.0f
                    );
                }
            }
        }

        std::println("[MOUSE_AIM] Target world pos: ({:.1f}, {:.1f}, {:.1f})",
                    aim_target.x, aim_target.y, aim_target.z);

        // Get camera FOV
        auto camera = game.camera();
        if (!camera) {
            std::println("[MOUSE_AIM] ERROR: No camera!");
            return;
        }

        // FIX: Get FOV correctly - it's already in radians!
        float fov_radians = camera.field_of_view();

        // If it's suspiciously small (< 0.1), it might be in degrees
        if (fov_radians < 0.1f) {
            // Convert from degrees to radians
            fov_radians = fov_radians * (3.14159f / 180.0f);
        }

        // If it's still too small or too large, use a default
        if (fov_radians < 0.5f || fov_radians > 2.0f) {
            std::println("[MOUSE_AIM] WARNING: FOV out of range ({:.3f}), using default 70deg", fov_radians);
            fov_radians = 70.0f * (3.14159f / 180.0f); // Default to 70 degrees
        }

        std::println("[MOUSE_AIM] Camera FOV: {:.3f} radians ({:.1f} degrees)",
                    fov_radians, fov_radians * 180.0f / 3.14159f);
        std::println("[MOUSE_AIM] Screen size: {:.0f}x{:.0f}", screen_width(), screen_height());

        // Convert to screen space
        roblox::Vector3 screen_pos = world_to_screen(
            aim_target, camera_cf, fov_radians,
            screen_width(), screen_height()
        );

        std::println("[MOUSE_AIM] Screen pos: ({:.1f}, {:.1f}, depth={:.1f})",
                    screen_pos.x, screen_pos.y, screen_pos.z);

        if (screen_pos.z <= 0) {
            std::println("[MOUSE_AIM] Target behind camera!");
            return;
        }

        // SANITY CHECK: Screen coordinates should be within screen bounds (with some margin)
        if (screen_pos.x < -100 || screen_pos.x > screen_width() + 100 ||
            screen_pos.y < -100 || screen_pos.y > screen_height() + 100) {
            std::println("[MOUSE_AIM] WARNING: Screen pos out of bounds! Skipping.");
            return;
            }

        float target_x = screen_pos.x;
        float target_y = screen_pos.y;

        float current_x = esp.mouse_x();
        float current_y = esp.mouse_y();

        std::println("[MOUSE_AIM] Current mouse: ({:.1f}, {:.1f})", current_x, current_y);
        std::println("[MOUSE_AIM] Target mouse: ({:.1f}, {:.1f})", target_x, target_y);
        std::println("[MOUSE_AIM] Delta: ({:.1f}, {:.1f})",
                    target_x - current_x, target_y - current_y);

        switch (settings.style) {
            case AimStyle::Silent:
            case AimStyle::Snap: {
                // Instant snap to target
                std::println("[MOUSE_AIM] Snap to target");
                esp.move_mouse(target_x, target_y);
                break;
            }

            case AimStyle::Legit: {
                // Smooth interpolation
                float alpha = 1.0f / settings.smoothing;
                float new_x = current_x + (target_x - current_x) * alpha;
                float new_y = current_y + (target_y - current_y) * alpha;

                std::println("[MOUSE_AIM] Legit smooth: alpha={:.3f}, new=({:.1f}, {:.1f})",
                            alpha, new_x, new_y);

                esp.move_mouse(new_x, new_y);
                break;
            }
        }
    }

protected:
    float m_screenWidth = 1920.0f;
    float m_screenHeight = 1080.0f;
    float m_mouse_x = 960.0f;
    float m_mouse_y = 540.0f;
    struct TargetHistory {
        roblox::Vector3 last_position;
        std::chrono::steady_clock::time_point last_time;
        roblox::Vector3 velocity;
    };
    std::unordered_map<vm_address_t, TargetHistory> m_target_history;

    // Calculate distance from crosshair ray
    float calculate_delta_distance(const roblox::CFrame& camera_cf, const roblox::Vector3& target_pos)
    {
        roblox::Vector3 camera_pos = camera_cf.position;
        roblox::Vector3 camera_look = camera_cf.look_vector();
        
        roblox::Vector3 to_target = target_pos - camera_pos;
        float depth = to_target.dot(camera_look);
        
        if (depth <= 0)
            return 99999.0f;  // Behind camera
        
        roblox::Vector3 crosshair_world = camera_pos + camera_look * depth;
        return crosshair_world.distance_to(target_pos);
    }

    float calculate_screen_distance(const roblox::Vector3& screen_pos,
                                    float mouse_x, float mouse_y) const
    {
        float dx = screen_pos.x - mouse_x;
        float dy = screen_pos.y - mouse_y;
        return std::sqrt(dx * dx + dy * dy);
    }

    roblox::Vector3 calculate_velocity(vm_address_t character_addr, const roblox::Vector3& current_pos) {
        auto now = std::chrono::steady_clock::now();

        auto it = m_target_history.find(character_addr);
        if (it == m_target_history.end()) {
            m_target_history[character_addr] = {current_pos, now, roblox::Vector3(0, 0, 0)};
            return roblox::Vector3(0, 0, 0);
        }

        auto& history = it->second;
        float dt = std::chrono::duration<float>(now - history.last_time).count();

        if (dt < 0.001f) {
            return history.velocity; // Too fast
        }

        roblox::Vector3 displacement = current_pos - history.last_position;
        roblox::Vector3 new_velocity = displacement / dt;

        // Smooth velocity with exponential moving average
        float alpha = 0.3f;
        roblox::Vector3 smoothed_velocity = history.velocity.lerp(new_velocity, alpha);

        history.last_position = current_pos;
        history.last_time = now;
        history.velocity = smoothed_velocity;

        return smoothed_velocity;
    }

    roblox::Vector3 predict_position(
        const roblox::Vector3& target_pos,
        const roblox::Vector3& target_velocity,
        const roblox::Vector3& shooter_pos,
        float bullet_speed,
        float bullet_drop = 0.0f,
        int max_iterations = 5) const
    {
        if (bullet_speed <= 0.0f) {
            return target_pos;
        }

        roblox::Vector3 predicted = target_pos;

        for (int i = 0; i < max_iterations; i++) {
            float distance = shooter_pos.distance_to(predicted);
            float time_to_hit = distance / bullet_speed;

            predicted = target_pos + target_velocity * time_to_hit;

            if (bullet_drop > 0.0f) {
                predicted.y -= 0.5f * bullet_drop * time_to_hit * time_to_hit;
            }
        }

        return predicted;
    }

    roblox::Vector3 predict_position_with_gravity(
        const roblox::Vector3& target_pos,
        const roblox::Vector3& target_velocity,
        const roblox::Vector3& shooter_pos,
        float bullet_speed,
        float gravity = 196.2f,
        int max_iterations = 5) const
    {
        if (bullet_speed <= 0.0f) {
            return target_pos;
        }

        roblox::Vector3 predicted = target_pos;

        for (int i = 0; i < max_iterations; i++) {
            float distance = shooter_pos.distance_to(predicted);
            float time_to_hit = distance / bullet_speed;

            // Predict horizontal movement (X, Z)
            predicted.x = target_pos.x + target_velocity.x * time_to_hit;
            predicted.z = target_pos.z + target_velocity.z * time_to_hit;

            // Predict vertical movement with gravity
            // Formula: y = y0 + v0*t - 0.5*g*t^2
            predicted.y = target_pos.y +
                         target_velocity.y * time_to_hit -
                         0.5f * gravity * time_to_hit * time_to_hit;
        }

        return predicted;
    }

    roblox::Vector3 world_to_screen(
        const roblox::Vector3& world_pos,
        const roblox::CFrame& camera_cf,
        float fov_radians,
        float screen_width,
        float screen_height)
    {
        roblox::Vector3 camera_pos = camera_cf.position;
        roblox::Vector3 look = camera_cf.look_vector();
        roblox::Vector3 right = camera_cf.right_vector();
        roblox::Vector3 up = camera_cf.up_vector();

        roblox::Vector3 to_target = world_pos - camera_pos;

        float depth = to_target.dot(look);
        if (depth <= 0) {
            return roblox::Vector3(0, 0, -1);  // Behind camera
        }

        float right_offset = to_target.dot(right);
        float up_offset = to_target.dot(up);

        float tan_half_fov = std::tan(fov_radians * 0.5f);
        float plane_height = 2.0f * depth * tan_half_fov;
        float plane_width = plane_height * (screen_width / screen_height);

        float x = (right_offset / plane_width + 0.5f) * screen_width;
        float y = (0.5f + up_offset / plane_height) * screen_height;  // + not - (screen Y is inverted)

        return roblox::Vector3(x, y, depth);
    }
};

class GameProfileFactory {
public:
    template<typename T>
    void register_profile() {
        m_profiles.push_back(std::make_unique<T>());
    }

    GameProfile* detect_game(const roblox::GameContext& game) {
        for (auto& profile : m_profiles) {
            if (profile->detect(game)) {
                return profile.get();
            }
        }
        return nullptr;  // No profile, use generic
    }

    GameProfile* get_profile(uint64_t place_id) {
        for (auto& profile : m_profiles) {
            if (profile->place_id() == place_id) {
                return profile.get();
            }
        }
        return nullptr;
    }

    const std::vector<std::unique_ptr<GameProfile>>& get_all_profiles() const {
        return m_profiles;
    }
    
private:
    std::vector<std::unique_ptr<GameProfile>> m_profiles;
};

} // namespace games
