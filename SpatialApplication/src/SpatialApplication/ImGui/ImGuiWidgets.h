//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatial
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

#pragma once

#include "Utility/MVCUtils.h"

#include "ImGui/ImGui.h"

#include <Walnut/Image.h>

namespace JPL::ImGuiEx
{
    bool PropertyCheckbox(const char* label, Property<bool>& property);

	//=========================================================================================
	/// Button Image

	void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed,
						 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax,
						 ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f));

	inline void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max, uv0, uv1);
	};

	inline void DrawButtonImage(const Walnut::Image& image,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImVec2 rectMin, ImVec2 rectMax,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectMin, rectMax, uv0, uv1);
	};

	inline void DrawButtonImage(const Walnut::Image& image,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImRect rectangle,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max, uv0, uv1);
	};


	inline void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), uv0, uv1);
	};

	inline void DrawButtonImage(const Walnut::Image& image,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), uv0, uv1);
	};

	bool PathButton(const char* label, ImVec2 size = ImVec2(0.0f, 0.0f));

	void DrawMainWindowButtons(ImVec2 titlebarMin, ImVec2 titlebarMax);

	struct IconButtonStyle
	{
		ImU32 ColourNormal = GUI::Colours::Theme::Text;
		ImU32 ColourHovered = GUI::Colours::Theme::TextBrighter;
		ImU32 ColourPressed = GUI::Colours::Theme::TextDarker;

		ImU32 BgColourNormal = IM_COL32_BLACK_TRANS;
		ImU32 BgColourHovered = IM_COL32_BLACK_TRANS;
		ImU32 BgColourPressed = IM_COL32_BLACK_TRANS;

		bool bDrawBorder = false;
		ImU32 BorderColour = GUI::Colours::Theme::Border;

		ImVec2 ButtonSize = ImVec2(0.0f, 0.0f);
	};

	bool IconButton(const char* label, const IconButtonStyle& style = {});

} // namespace JPL::ImGuiEx
