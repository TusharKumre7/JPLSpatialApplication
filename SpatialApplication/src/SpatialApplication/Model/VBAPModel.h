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

#pragma once

#include "Utility/MVCUtils.h"
#include "Processing/Panner.h"

#include <JPLSpatial/Math/MinimalVec3.h>

namespace JPL
{
	class VBAPModel : public GenericChangeBroadcaster
	{
	public:
		VBAPModel();
		~VBAPModel() = default;

		// Position of the source relative to the listener
		Property<MinimalVec3> SourcePosition{ MinimalVec3(0.0f, 0.0f, -1.0f) };
		Property<float> Spread{ 1.0f };
		Property<float> Focus{ 0.0f };
		Property<float> SourceSize{ 5.0f };

		Property<bool> SpreadFromSourceSize{ true };
		Property<bool> HeightSpread{ true };
		Property<bool> UseSourceOrientation{ false };

		// @returns distance from the source, or length of the source posiiton vector
		float ComputePanUpdateData(bool bWithHeightSpeakers, JPLPanUpdateData& outPanUpdateData);
	};

} // namespace JPL
