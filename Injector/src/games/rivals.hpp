#pragma once

#include "games/generic.hpp"

namespace games {

    class RivalsProfile : public GenericProfile {
    public:
        std::string name() const override { return "Rivals"; }
        uint64_t place_id() const override { return 17625359962; }

        bool detect(const roblox::GameContext& game) const override {
            return game.place_id() == place_id();
        }

        void debug_player_data(roblox::Player player) {
            if (!player) return;

            auto name = player.name().value_or("Unknown");
            std::println("[DEBUG] Scanning Player: {}", name);

            // 1. Check for standard TeamColor (often used when Team is null)
            // Offset for TeamColor is usually near Team
            // std::println("  - TeamColor: {}", player.team_color());

            // 2. Scan Children for "Value" objects (StringValue,IntValue)
            for (const auto& child : player.children()) {
                auto child_name = child.name().value_or("???");
                auto class_name = child.class_name().value_or("???");
                std::println("  - Child: {} (Class: {})", child_name, class_name);

                if (child_name == "leaderstats") {
                    for (const auto& stat : child.children()) {
                        std::println("    - Stat: {}", stat.name().value_or("???"));
                    }
                }
            }
        }

        void debug_workspace_teams(roblox::GameContext& game) {
            auto ws = game.workspace();
            std::println("[DEBUG] Scanning Workspace: ");
            for (const auto& child : ws.children()) {
                auto name = child.name().value_or("");
                // Look for folders that might contain player models
                std::println("  - Child: {} (Class: {})", name, child.class_name().value_or("????"));
                if (name == "Red" || name == "Blue" || name == "T" || name == "CT") {
                    std::println("[RIVALS] Potential Team Folder found: {}", name);
                }
            }
        }

        std::vector<Target> find_targets(roblox::GameContext& game, const roblox::CFrame& camera_cf, float fov_radians) override
        {
            // For now, show ALL players and let user decide
            // TODO: Investigate how teams work in this game
            auto targets = GenericProfile::find_targets(game, camera_cf, fov_radians);

            // Debug: Log team info once
            static bool logged = false;
            if (!logged && !targets.empty()) {
                logged = true;
                auto local = game.local_player();

                debug_workspace_teams(game);

                debug_player_data(local);

                auto my_team = local.team();
                std::println("[RIVALS] My team: {:#x}", my_team ? my_team.address() : 0);

                auto teams = game.teams();
                if (teams) {
                    std::println("[RIVALS] Teams service found, children:");
                    for (const auto& child : teams.children()) {
                        auto name = child.name().value_or("???");
                        std::println("[RIVALS]   - {} ({:#x})", name, child.address());
                    }
                } else {
                    std::println("[RIVALS] No Teams service found");
                }
            }

            return targets;
        }
    };

} // namespace games
