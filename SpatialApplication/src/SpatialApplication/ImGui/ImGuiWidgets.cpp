//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright Jaroslav Pevno, JPL Spatial Application is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "ImGuiWidgets.h"

#include "Walnut/Application.h"

#include <JPLSpatial/Core.h>

namespace JPL::ImGuiEx
{
    bool PropertyCheckbox(const char* label, Property<bool>& property)
    {
        bool bValue = property.Get();
        ScopedItemOutline outline(label, OutlineFlags_NoOutlineInactive, ImColor(60, 60, 60));
        if (ImGui::Checkbox(label, &bValue))
        {
            property.Set(bValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax, ImVec2 uv0, ImVec2 uv1)
    {
        auto* drawList = ImGui::GetWindowDrawList();
        if (ImGui::IsItemActive())
        {
            drawList->AddImage(imagePressed.GetDescriptorSet(), rectMin, rectMax, uv0, uv1, tintPressed);
        }
        else if (ImGui::IsItemHovered())
        {
            drawList->AddImage(imageHovered.GetDescriptorSet(), rectMin, rectMax, uv0, uv1, tintHovered);
        }
        else
        {
            drawList->AddImage(imageNormal.GetDescriptorSet(), rectMin, rectMax, uv0, uv1, tintNormal);
        }
    }

    bool PathButton(const char* label, ImVec2 size)
    {
#if defined(JPL_PLATFORM_WINDOWS)
        static constexpr std::string_view separator = "\\";
#elif defined(JPL_PLATFORM_LINUX) or defined(JPL_PLATFORM_FREEBSD) or defined(JPL_PLATFORM_MACOS) or defined(JPL_PLATFORM_WASM)
        static constexpr std::string_view separator = "/";
#endif
        ScopedColourStack buttonColours(
            ImGuiCol_Button, IM_COL32_BLACK_TRANS,
            ImGuiCol_Border, IM_COL32_BLACK_TRANS
        );
        Conditional<ScopedColour> separatorColor(
            std::string_view(label) == separator,
            ImGuiCol_Text, GUI::Colours::Theme::TextDarker
        );
        ScopedStyle innerPadding(
            ImGuiStyleVar_FramePadding, ImVec2(2.0f, ImGui::GetStyle().FramePadding.y)
        );
        return ImGui::Button(label, size);
    }

	void DrawMainWindowButtons(ImVec2 titlebarMin, ImVec2 titlebarMax)
	{
		JPL::ImGuiEx::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		const bool bIsMaximized = Walnut::Application::Get().IsMaximized();

		const float buttonWidth = 36.0f;
		const float buttonHeight = 26.0f;
		const float buttonsStartX = titlebarMax.x - buttonWidth * 3.0f;
		const float titlebarVerticalOffset = bIsMaximized ? 6.0f : 0.0f;

		ImGui::SetCursorScreenPos(ImVec2(buttonsStartX, titlebarMin.y + titlebarVerticalOffset));

		const float iconPadding = 13.0f;

		auto getFillColour = []
		{
			return ImGui::IsItemActive()
				? IM_COL32(255, 255, 255, 10)
				: ImGui::IsItemHovered()
					? IM_COL32(255, 255, 255, 20)
					: IM_COL32_BLACK_TRANS;
		};

		auto getIconColour = []
		{
			return ImGui::IsItemActive()
				? JPL::GUI::Colours::Theme::TextDarker
				: (ImU32)JPL::Colour(JPL::GUI::Colours::Theme::Text).WithMultipliedValue(0.9f);
		};

		
		auto* drawList = ImGui::GetForegroundDrawList();

		// Minimize Button
		{
			if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
			{
				Walnut::Application::Get().Minimize();
			}
			const ImU32 fillColour = getFillColour();
			const ImU32 iconColour = getIconColour();

			auto rect = JPL::ImGuiEx::GetItemRect();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 lineP1 = rect.Min + ImVec2(iconPadding, rect.GetSize().y * 0.5f);
			const ImVec2 lineP2 = lineP1 + ImVec2(rect.GetSize().x - iconPadding * 2.0f, 0.0f);
			drawList->AddLine(lineP1, lineP2, iconColour, 1.0f);
		}

		// Maximize Button
		{
			if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
			{
				Walnut::Application::Get().ToggleMaximize();
			}
			const ImU32 fillColour = getFillColour();
			const ImU32 iconColour = getIconColour();

			auto rect = JPL::ImGuiEx::GetItemRect();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 center = rect.GetCenter();
			const float radius = (rect.GetWidth() * 0.5f - iconPadding) * 0.8f;
			ImRect square{ center - ImVec2(radius, radius), center + ImVec2(radius, radius) };

			if (bIsMaximized)
			{
				// translate for the first square
				square.Translate(ImVec2(1.0f, -1.0f));
				drawList->AddRect(square.Min, square.Max, iconColour, 0.0f, 0, 1.0f);

				// translate for the second square that overlaps the first one
				square.Translate(ImVec2(-2.0f, 2.0f));

				// fill the suqare on top with opaque titlebar colour
				const ImU32 iconFill = JPL::GUI::Colours::Theme::Titlebar;
				drawList->AddRectFilled(square.Min, square.Max, iconFill, 0.0f, 0);
				// add the hover effect to our opaque fill
				drawList->AddRectFilled(square.Min, square.Max, fillColour, 0.0f, 0);
				// add bright outline
				drawList->AddRect(square.Min, square.Max, iconColour, 0.0f, 0, 1.0f);
			}
			else
			{
				square.Expand(1.0f);
				drawList->AddRect(square.Min, square.Max, iconColour, 0.0f, 0, 1.0f);
			}
		}

		// Close Button
		{
			if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
			{
				Walnut::Application::Get().Close();
			}

			auto rect = JPL::ImGuiEx::GetItemRect();

			// Close button is the only one uses different fill colour (red)
			const ImU32 fillColour = ImGui::IsItemActive()
									? IM_COL32(255, 50, 50, 150)
									: ImGui::IsItemHovered()
										? IM_COL32(235, 50, 50, 255)
										: IM_COL32_BLACK_TRANS;

			// Draw icon dark, to make sure it's visible on the red background fill
			const ImU32 iconColour = ImGui::IsItemActive() || ImGui::IsItemHovered()
						? JPL::GUI::Colours::Theme::Titlebar
						: getIconColour();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 center = rect.GetCenter();
			const float radius = (rect.GetWidth() * 0.5f - iconPadding);
			const ImRect square{ center - ImVec2(radius, radius), center + ImVec2(radius, radius) };

			drawList->AddLine(square.GetTL(), square.GetBR() - ImVec2(0.5f, 0.5f), iconColour, 1.5f);
			drawList->AddLine(square.GetTR() - ImVec2(0.5f, 0.5f), square.GetBL() - ImVec2(0.0f, 1.0f), iconColour, 1.5f);
		}
	}

	bool IconButton(const char* label, const IconButtonStyle& style)
	{
		const ImVec2 position = ImGui::GetCursorScreenPos();

		ImVec2 buttonSize = ImGui::CalcTextSize(label) + ImGui::GetStyle().FramePadding * 2.0f;

		if (style.ButtonSize.x != 0.0f)
		{
			buttonSize.x = ImMax(buttonSize.x, style.ButtonSize.x);
		}

		if (style.ButtonSize.y != 0.0f)
		{
			buttonSize.y = ImMax(buttonSize.y, style.ButtonSize.y);
		}

		// Mouse interaction handled by invisible button
		const bool bPressed = ImGui::InvisibleButton(label, buttonSize);
		
		// Visuals we draw outselves...

		const ImU32 backgroundColour =
			ImGui::IsItemActive() ? style.BgColourPressed :
			ImGui::IsItemHovered() ? style.BgColourHovered :
			style.BgColourNormal;

		const ImU32 iconColour =
			ImGui::IsItemActive() ? style.ColourPressed :
			ImGui::IsItemHovered() ? style.ColourHovered :
			style.ColourNormal;

		// Draw background and border
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), backgroundColour, ImGui::GetStyle().FrameRounding);
		if (style.bDrawBorder)
		{
			drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), style.BorderColour, ImGui::GetStyle().FrameRounding);
		}
		
		// Draw icon text label
		ImGui::SetCursorScreenPos(position);
		ImGui::AlignTextToFramePadding();
		ShiftCursorX(ImGui::GetStyle().FramePadding.x);

		ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(iconColour), label);

		return bPressed;
	}

	/*IconButtonStyle IconButtonStyle::Make(ImU32 colourNormal, ImU32 colourHovered, ImU32 colourPressed)
	{
		return IconButtonStyle{
			.ColourNormal = colourNormal,
			.ColourHovered = colourHovered,
			.ColourPressed = colourPressed,
			.BgColourNormal = IM_COL32_BLACK_TRANS,
			.BgColourHovered = IM_COL32_BLACK_TRANS,
			.BgColourPressed = IM_COL32_BLACK_TRANS
		};
	}

	IconButtonStyle IconButtonStyle::Default()
	{
		return IconButtonStyle();
	}*/

} // namespace JPL::ImGuiEx
