#pragma once

namespace ImGui
{
    // Combo box — works for any enum, no boilerplate needed.
    template <typename EnumT>
    std::optional<EnumT> EnumCombo(const char* label, EnumT* value)
    {
        static_assert(std::is_enum_v<EnumT>, "EnumCombo requires an enum type");

        constexpr auto entries = magic_enum::enum_entries<EnumT>();
        bool changed = false;
        EnumT newValue;

        const char* previewName = magic_enum::enum_name(*value).data();

        if (ImGui::BeginCombo(label, previewName))
        {
            for (auto [enumValue, name] : entries)
            {
                bool selected = (*value == enumValue);
                if (ImGui::Selectable(name.data(), selected))
                {
                    newValue = enumValue;
                    changed = true;
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (changed) {
            return newValue;
        }
        else {
            return std::nullopt;
        }
   
    }


}