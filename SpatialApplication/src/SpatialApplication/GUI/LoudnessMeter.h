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

#include <span>
#include <vector>

namespace JPL
{
	class LoudnessMeter
	{
	public:
		LoudnessMeter() = default;
		~LoudnessMeter() = default;

		struct Levels
		{
			float Peak;
			float RMS;
		};

		void SetNumberOfChannels(uint32_t numberOfChannels);
		void SubmitLevels(std::span<const Levels> levelsNormalized);
		void SubmitLevels(std::span<const float> peak, std::span<const float> rms);
		void OnRender(float deltaTime);

	private:
		uint32_t mNumberOfChannels = 0;
		std::vector<Levels> mLevels;
		std::vector<Levels> mPrevLevels;

		const float mFallOffTime = 2.0f;
		const float mPeakHoldTime = 1.0f;
		std::vector<float> mPeakHoldTicks;
		std::vector<float> m_PeakHoldLevels;

		const float mMinusInfinity = -100.0f;
	};
} // namespace JPL
