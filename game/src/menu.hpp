void mainMenu(flecs::world& ecs) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 windowPos = viewport->Pos;
    ImVec2 windowSize = viewport->Size;

    // Fullscreen window
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::SetNextWindowBgAlpha(0.5f);

    ImGui::Begin("Main Menu", nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar
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
    if (CenteredButton("New Game", 0.0f)) {

        ecs.lookup("newGame").get<Callback>().callbackFunction();
    }
    if (CenteredButton("Load Game", buttonHeight + spacing)) {

        ecs.lookup("loadGame").get<Callback>().callbackFunction();
    }
    if (CenteredButton("Options", 2 * (buttonHeight + spacing))) {

        ecs.lookup("gameOptions").get<Callback>().callbackFunction();
    }
    if (CenteredButton("Exit", 3 * (buttonHeight + spacing))) {
        
        ecs.lookup("Exit").get<Callback>().callbackFunction();

    }

    ImGui::End();
}
