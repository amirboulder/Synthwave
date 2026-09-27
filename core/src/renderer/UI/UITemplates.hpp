#pragma once

namespace ImGui
{
    template <typename EnumT, typename Filter>
    bool EnumCombo(const char* label, EnumT* value, Filter&& isVisible)
    {
        static_assert(std::is_enum_v<EnumT>, "EnumCombo requires an enum type");
        bool changed = false;
        if (ImGui::BeginCombo(label, magic_enum::enum_name(*value).data())) {
            for (auto [enumValue, name] : magic_enum::enum_entries<EnumT>()) {
                if (!isVisible(enumValue)) continue;
                bool selected = (*value == enumValue);
                if (ImGui::Selectable(name.data(), selected) && !selected) {
                    *value = enumValue;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    template <typename EnumT>
    bool EnumCombo(const char* label, EnumT* value)
    {
        return EnumCombo(label, value, [](EnumT) { return true; });
    }

}