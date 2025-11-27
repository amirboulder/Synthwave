
namespace Menu {

    const ImGuiWindowFlags menuFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    void mainMenuDraw(flecs::world& ecs) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 windowPos = viewport->Pos;
        ImVec2 windowSize = viewport->Size;

        // Fullscreen window
        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(windowSize);
        ImGui::SetNextWindowBgAlpha(0.8f);

        ImGui::Begin("Main Menu", nullptr,
            menuFlags
        );

        // --- Dynamic layout settings ---
        const float baseWidth = 1920.0f;
        const float baseHeight = 1080.0f;
        const float scaleX = windowSize.x / baseWidth;
        const float scaleY = windowSize.y / baseHeight;
        const float scale = std::min(scaleX, scaleY); // keep uniform scale

        const float buttonWidth = 300.0f * scale;
        const float buttonHeight = 70.0f * scale;
        const float spacing = 30.0f * scale;

        ImVec2 screenCenter(windowSize.x * 0.5f, windowSize.y * 0.5f);
        float totalHeight = 4 * buttonHeight + 3 * spacing;
        float startY = screenCenter.y - totalHeight * 0.5f;

        // Helper lambda for consistent centering
        auto CenteredButton = [&](const char* label, float yOffset) {
            ImGui::SetCursorPos(ImVec2(
                (windowSize.x - buttonWidth) * 0.5f,
                startY + yOffset
            ));
            return ImGui::Button(label, ImVec2(buttonWidth, buttonHeight));
        };

        // --- Buttons --- 
        //Button clicks emitt commands  which will be processed next frame 
        if (CenteredButton("New Game", 0.0f)) {

            ecs.entity().set<UICommand>({ UICommandType::NewGame });
        }
        if (CenteredButton("Load Game", buttonHeight + spacing)) {

            ecs.entity().set<UICommand>({ UICommandType::LoadGame });
        }
        if (CenteredButton("Options", 2 * (buttonHeight + spacing))) {

            ecs.entity().set<UICommand>({ UICommandType::GameOptions });
        }
        if (CenteredButton("Exit", 3 * (buttonHeight + spacing))) {

            ecs.entity().set<UICommand>({ UICommandType::ExitGame });
        }

        ImGui::End();
    }


    void pauseMenuDraw(flecs::world& ecs) {

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 windowPos = viewport->Pos;
        ImVec2 windowSize = viewport->Size;

        // Fullscreen window
        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(windowSize);
        ImGui::SetNextWindowBgAlpha(0.5f);

        ImGui::Begin("Pause Menu", nullptr,
            menuFlags
        );

        // --- Dynamic layout settings ---
        const float baseWidth = 1920.0f;
        const float baseHeight = 1080.0f;
        const float scaleX = windowSize.x / baseWidth;
        const float scaleY = windowSize.y / baseHeight;
        const float scale = std::min(scaleX, scaleY); // keep uniform scale

        const float buttonWidth = 300.0f * scale;
        const float buttonHeight = 70.0f * scale;
        const float spacing = 30.0f * scale;

        //TODO find a more dyamic way than windowSize.y * 0.35f
        ImVec2 screenCenter(windowSize.x * 0.5f, windowSize.y * 0.35f);
        float totalHeight = 4 * buttonHeight + 3 * spacing;
        float startY = screenCenter.y - totalHeight * 0.5f;

        // Helper lambda for consistent centering
        auto CenteredButton = [&](const char* label, float yOffset) {
            ImGui::SetCursorPos(ImVec2(
                (windowSize.x - buttonWidth) * 0.5f,
                startY + yOffset
            ));
            return ImGui::Button(label, ImVec2(buttonWidth, buttonHeight));
        };

        // --- Buttons ---
        if (CenteredButton("Resume", 0.0f)) {

            ecs.entity().set<UICommand>({ UICommandType::ResumeGame });
        }
        if (CenteredButton("Restart Level", buttonHeight + spacing)) {

            ecs.entity().set<UICommand>({ UICommandType::RestartLevel });
        }
        if (CenteredButton("Save Game", 2 * (buttonHeight + spacing))) {

            ecs.entity().set<UICommand>({ UICommandType::SaveGame });
        }
        if (CenteredButton("Load Game", 3 * (buttonHeight + spacing))) {

            ecs.entity().set<UICommand>({ UICommandType::LoadGame });
        }
        if (CenteredButton("Options", 4 * (buttonHeight + spacing))) {

            ecs.entity().set<UICommand>({ UICommandType::GameOptions });
        }
        if (CenteredButton("Main Menu", 5 * (buttonHeight + spacing))) {

            ecs.entity().set<UICommand>({ UICommandType::MainMenu });
        }
        if (CenteredButton("Exit", 6 * (buttonHeight + spacing))) {

            ecs.entity().set<UICommand>({ UICommandType::ExitGame });
        }

        ImGui::End();

    }


}