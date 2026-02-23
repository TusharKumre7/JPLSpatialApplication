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

#include "WaveformRendering.h"

#include "Controller/AudioPlayer.h"
#include "Utility/Timer.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/Containers/StaticArray.h>
#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/Math/SIMDMath.h>

#define STB_VORBIS_HEADER_ONLY
#include "miniaudio/extras/stb_vorbis.c"
#include <miniaudio/miniaudio.h>

#undef min
#undef max

#include <algorithm>
#include <vector>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <implot.h>

namespace JPL::GUI
{
	void Waveform::SetFile(const std::filesystem::path& file)
	{
		if (file == mSelectedFile)
		{
			return;
		}

		mSelectedFile = file;

		// If we are in the middle of processing last waveform data,
		// we need to wait for it to finish.
		if (mFutureChannelData.valid())
		{
			mFutureChannelData.wait();
		}

		mSampleData.clear();
		
		if (AudioPlayer::IsValidAudioFile(mSelectedFile))
		{
			auto loadAndParseFileAsync = [this] (std::filesystem::path file, uint64_t waveformPx) -> std::vector<ChannelData>
			{
				std::vector<ChannelData> outChannelData;

				ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);

				uint64 numFramesRead = 0;
				void* framesPtr = nullptr;
				const ma_result result = ma_decode_file(file.string().c_str(), &config, &numFramesRead, &framesPtr);

				if (not JPL_ENSURE(result == MA_SUCCESS) || numFramesRead == 0 || framesPtr == nullptr)
				{
					if (framesPtr != nullptr)
					{
						ma_free(framesPtr, nullptr);
					}
					return outChannelData;
				}
			

				ma_uint16 channels = config.channels;
				ma_uint32 sampleRate = config.sampleRate;
				double duration = (double)numFramesRead / (double)sampleRate;

				//auto dataSize = wav.dataChunkDataSize;
				//auto pos = wav.dataChunkDataPos;
				//auto fileSize = dataSize + pos;
				////auto sizestr = Utils::BytesToString(fileSize);
				//fileInfo = { duration, sampleRate, bitDepth, channels, wav.totalPCMFrameCount, fileSize };

				mSampleData.resize(numFramesRead * channels);

				JPL::StaticArray<void*, MA_MAX_CHANNELS> channelPtrs(channels);
				for (uint32_t i = 0; i < channels; ++i)
				{
					channelPtrs[i] = &mSampleData[i * numFramesRead];
				}

				ma_deinterleave_pcm_frames(ma_format_f32, channels, numFramesRead, framesPtr, (void**)channelPtrs.data());
			
				// miniaudio doesn't deallocated allocated frame storage,
				// we need to do it ourselves
				ma_free(framesPtr, nullptr);

				outChannelData = GenerateMinMaxValues(mSampleData, numFramesRead, waveformPx);
				return outChannelData;
			};

			mFutureChannelData = std::async(std::launch::async, loadAndParseFileAsync, mSelectedFile, mWaveformWidthPx);
		}
	}

	void Waveform::Draw()
	{
		// Check if we're processing a new waveform data if the results ar available
		if (mFutureChannelData.valid() && mFutureChannelData.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			mChannelData = mFutureChannelData.get();
		}
			
		const uint32_t numChannels = mChannelData.size();
		
		// When we have multiple subplots, ImGui adds a scrollbard, even uf item spacing set to 0
		const ImVec2 plotSpace = ImGui::GetContentRegionAvail() - ImVec2(ImGui::GetStyle().ChildBorderSize * 2.0f, float(numChannels > 1));

		// TODO: we may want to update the waveform when the display width changes
		mWaveformWidthPx = static_cast<int>(plotSpace.x);
		
		if (mChannelData.empty())
		{
			return;
		}

#if 0	// Enable to play around with waveform colour
		JPL::ImGuiEx::Window("Waveform Col", [&]
		{
			ImGui::ColorPicker4("Waveform Fill", &mWaveformFillColour.Value.x);
			ImGui::ColorPicker4("Waveform Outline", &mWaveformLineColour.Value.x);
		});
#endif

		JPL::ImGuiEx::ScopedColour frameBg(ImGuiCol_FrameBg, IM_COL32(14, 14, 14, 255));
		JPL::ImGuiEx::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.0f, 0.0f));
		const ImPlotSpec spec(
			ImPlotProp_LineColor, mWaveformLineColour,
			ImPlotProp_FillColor, mWaveformFillColour,
			ImPlotProp_LineWeight, mLineThickness
		);

		std::vector<float> indices; // TODO: this technically should be time position and based on time position it asks for a min-max values?
		indices.resize(mChannelData[0].Min.size());
		for (size_t i = 0; i < indices.size(); ++i)
			indices[i] = (float)i;

		const auto drawChannel = [&](const char* plotId, const std::vector<float>& min, const std::vector<float>& max, float height, bool drawTimeline)
		{
			if (ImPlot::BeginPlot(plotId, ImVec2(0, 0), ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame/* | ImPlotFlags_NoChild*/))
			{
				ImPlot::SetupAxis(ImAxis_X1, nullptr,
								  (drawTimeline ? 0 : ImPlotAxisFlags_NoTickLabels)
								  | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks
								  | ImPlotAxisFlags_NoHighlight
								  | ImPlotAxisFlags_AutoFit);

				ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight);
				ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImPlotCond_Always);

				ImPlot::PlotShaded("fill", indices.data(), min.data(), max.data(), indices.size(), spec);

				ImPlot::PlotLine("min", min.data(), min.size(), 1.0f, 0.0, spec);
				ImPlot::PlotLine("max", max.data(), max.size(), 1.0f, 0.0, spec);

				ImPlot::EndPlot();
			}
		};

		const ImPlotSubplotFlags flags
			= ImPlotSubplotFlags_NoResize
			| ImPlotSubplotFlags_LinkRows
			| ImPlotSubplotFlags_LinkAllX
			| ImPlotSubplotFlags_LinkAllY
			| ImPlotSubplotFlags_NoLegend
			| ImPlotSubplotFlags_ShareItems;


		if (indices.size() && ImPlot::BeginSubplots("##Waveform Plot", numChannels, 1, plotSpace, flags))
		{
			const float channelHeight = ImGui::GetContentRegionAvail().y / numChannels;
			char id = 'a';

			for (const ChannelData& channelData : mChannelData)
			{
				drawChannel(&id, channelData.Min, channelData.Max, channelHeight, false);
				id++;
			}

			ImPlot::EndSubplots();
		}

		ImPlot::PopStyleVar(); // ImPlotStyleVar_PlotPadding
	}

	std::vector<Waveform::ChannelData> Waveform::GenerateMinMaxValues(const std::vector<float>& sampleData, uint64_t numFrames, int frameWidth)
	{
		std::vector<ChannelData> outChannelData;
		
		const uint32_t numChannels = sampleData.size() / numFrames;

		if (numChannels == 0 || frameWidth < 1 || numFrames < 1 || sampleData.empty())
		{
			return outChannelData;
		}

		outChannelData.resize(numChannels);

		for (ChannelData& channelData : outChannelData)
		{
			channelData.Min.clear();
			channelData.Max.clear();
		}

		const uint64 resolutionWidth = std::min(static_cast<uint64>(frameWidth), numFrames);
		const float timeStep = 1.0f / static_cast<float>(resolutionWidth);
		const int64 itemCount = numFrames - 1;

		for (uint32_t channelIndex = 0; channelIndex < outChannelData.size(); ++channelIndex)
		{
			ChannelData& channelData = outChannelData[channelIndex];

			// Get pointer to channel sample data in our contiguous buffer
			const float* data = &sampleData[channelIndex * numFrames];

			//! Note: we can pass in value offset to display a shorter window of the sample data

			//float v0 = data[0 /* valueOffset */];
			float t0 = 0.0f;

			uint64 v0_idx = 0; /* valueOffset */

#if 1		// Slightly vectorized version of the algo below

			channelData.Min.reserve(resolutionWidth);
			channelData.Max.reserve(resolutionWidth);

			for (uint64 s = 0; s < resolutionWidth; ++s)
			{
				const float t1 = std::min(1.0f, t0 + timeStep);

				const auto v1_idx = static_cast<uint64>(t1 * itemCount + 0.5f);
				JPL_ASSERT(v1_idx < numFrames);

				const float v1 = data[/* valueOffset +*/ (v1_idx + 1) % numFrames];

				simd minV = v1, maxV = v1;

				const uint64 numValuesInItem = v1_idx - v0_idx;
				const uint64 toSIMD = JPL::FloorToSIMDSize(numValuesInItem);

				uint32_t si = 0;
				for (; si < toSIMD; si += simd::size())
				{
					JPL_ASSERT((v0_idx + si + simd::size()) < numFrames);

					const simd v(&data[v0_idx + si]);
					minV = min(v, minV);
					maxV = max(v, maxV);
				}

				float minS = minV.reduce_min(), maxS = maxV.reduce_max();

				for (; si < numValuesInItem; ++si)
				{
					const float v = data[(v0_idx + si) % numFrames];

					if (v < minS)
						minS = v;
					if (v > maxS)
						maxS = v;
				}

				channelData.Min.push_back(minS);
				channelData.Max.push_back(maxS);

				t0 = t1;
				v0_idx = v1_idx;
			}
#else
			channelData.Min.reserve(resolutionWidth);
			channelData.Max.reserve(resolutionWidth);

			for (uint64 s = 0; s < resolutionWidth; ++s)
			{
				const float t1 = std::min(1.0f, t0 + timeStep);

				const auto v1_idx = static_cast<uint64>(t1 * itemCount + 0.5f);
				JPL_ASSERT(v1_idx < numFrames);

				const float v1 = data[/* valueOffset +*/ (v1_idx + 1) % numFrames];

				float min = v1, max = v1;
				for (uint32_t s = 0; s < v1_idx - v0_idx; ++s)
				{
					const float v = data[/* valueOffset +*/ (v0_idx + s) % numFrames];

					if (v < min)
						min = v;
					if (v > max)
						max = v;
				}

				channelData.Min.push_back(min);
				channelData.Max.push_back(max);

				t0 = t1;
				v0_idx = v1_idx;
			}
#endif
		}

		return outChannelData;
	}
} // namespace JPL::GUI
