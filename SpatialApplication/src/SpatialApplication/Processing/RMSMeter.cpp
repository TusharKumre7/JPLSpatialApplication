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

#include "RMSMeter.h"

#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/Math/SIMDMath.h>

#include <algorithm>
#include <concepts>
#include <cmath>
#include <format>
#include <string>

namespace JPL
{
    //==========================================================================
    // TODO: move this somewhere reasonable
    namespace SampleBufferOperations
    {
        static JPL_INLINE float GetMagnitude(const float* data, uint32 numFrames)
        {
            simd magVector = simd::zero();

            uint32 numSimd = GetNumSIMDOps(numFrames);
            while (numSimd--)
            {
                magVector = max(magVector, abs(simd(data)));
                data += simd::size();
            }

            float mag = magVector.reduce_max();
            for (uint32 t = 0; t < GetSIMDTail(numFrames); ++t)
            {
                mag = std::max(mag, data[t]);
            }

            return mag;
        }

        template<template<class> class LayoutType>
        concept CDeinterleavedLayout = std::same_as<LayoutType<float>, choc::buffer::SeparateChannelLayout<float>>
                                    || std::same_as<LayoutType<float>, choc::buffer::MonoLayout<float>>;

        // @param buffer must be either a channel array buffer view or have MonoLayout
        template<template<class> class LayoutType> requires(CDeinterleavedLayout<LayoutType>)
        static float GetMagnitude(choc::buffer::BufferView<const float, LayoutType> buffer, uint32 startFrame, uint32 numFrames)
        {
            JPL_ASSERT((startFrame + numFrames) <= buffer.getNumFrames());

            float mag = 0.0f;
            for (uint i = 0; i < buffer.getNumChannels(); ++i)
            {
                const float* channelData = buffer.getChannel(i).data.data + startFrame;
                mag = std::max(mag, GetMagnitude(channelData, numFrames));
            }

            return mag;
        }
    } // namepace SampleBufferOperations

    //==========================================================================
    RMSMeter::RMSMeter()
        : mNumberOfBins(0)
        , mNumberOfSamplesPerBin(0)
        , mNumberOfSamplesInAllBins(0)
        , mNumberOfBinsToCoverPeakMs(0)
        , mFreezeLoudnessRangeOnSilence(false)
        , mCurrentBlockIsSilent(false)
    {
        JPL_TRACE_TAG("RMSMeter",
                      std::format("The longest possible measurement until a buffer overflow = {}",
                                  std::to_string(INT_MAX / 10. / 3600. / 365.) + " years"));

        // If this class is used without caution and ProcessBlock
        // is called before Prepare, divisions by zero
        // might occure. E.g. if m_NumberOfSamplesInAllBins = 0.
        //
        // To prevent this, Prepare is called here with
        // some arbitrary arguments.
        Prepare(48000.0, 2, 480, 60);
    }

    RMSMeter::~RMSMeter()
    {
    }

    void RMSMeter::Prepare(double sampleRate,
                           int numberOfInputChannels,
                           int estimatedSamplesPerBlock,
                           int expectedRequestRate)
    {
        // Modify the expectedRequestRate if needed.
        // It needs to be at least 10 and a multiple of 10 because
        // exactly every 0.1 second a gating block needs to be measured
        // (for the integrated loudness measurement).
        if (expectedRequestRate < 10)
        {
            expectedRequestRate = 10;
        }
        else
        {
            expectedRequestRate = (((expectedRequestRate - 1) / 10) + 1) * 10;
            // examples
            //  19 -> 20
            //  20 -> 20
            //  21 -> 30
        }
        // It also needs to be a divisor of the samplerate for accurate
        // M and S values (the integrated loudness (I value) would not be
        // affected by this inaccuracy.
        while (int(sampleRate) % expectedRequestRate != 0)
        {
            expectedRequestRate += 10;

            if (expectedRequestRate > sampleRate / 2)
            {
                expectedRequestRate = 10;
                JPL_TRACE_TAG("RMSMeter", "Not possible to make expectedRequestRate a multiple of 10 and "
                                            "a divisor of the samplerate.");
                break;
            }
        }

        //JPL_TRACE_TAG("RMSMeter", "Expected request rate = {}", expectedRequestRate);

        // Figure out how many bins are needed.

        // Needed for the short term loudness measurement.
        mNumberOfBins = expectedRequestRate * cTimeOfAccumulationForRMS;
        mNumberOfSamplesPerBin = int(sampleRate / expectedRequestRate);
        mNumberOfSamplesInAllBins = mNumberOfBins * mNumberOfSamplesPerBin;
        mInvNumberOfSamplesInAllBins = 1.0 / mNumberOfSamplesInAllBins;

        mNumberOfBinsToCoverPeakMs = int((1.0 / expectedRequestRate) * expectedRequestRate);
        //JPL_TRACE_TAG("RMSMeter", "Number of bins to cover peak measure time = {}", m_NumberOfBinsToCoverPeakMs);

        mCurrentBin = 0;
        mNumberOfSamplesInTheCurrentBin = 0;
        mCurrentPeakBin = 0;

        // Initialize the bins.
        mBin.assign(numberOfInputChannels, std::vector<double>(mNumberOfBins, 0.0));
        mPeakBin.assign(numberOfInputChannels, std::vector<double>(mNumberOfBinsToCoverPeakMs, 0.0));

        // RMS for the individual channels.
        //! Even though this is not realtime thread, we can safely modify values here
        //! because realtime thread won't start using it until later
        {
            LevelsRealtimeAccess rms(mLastRMSMeasureTime);
            rms->Levels.assign(numberOfInputChannels, 0.0f);
            rms->MaxLevels.assign(numberOfInputChannels, 0.0f);
        }

        mAverageRMSForIndividualChannelsReturn.assign(numberOfInputChannels, sMinimalReturnValue);
        mMaxRMSForIndividualChannelsReturn.assign(numberOfInputChannels, sMinimalReturnValue);

        //! Even though this is not realtime thread, we can safely modify values here
        //! because realtime thread won't start using it until later
        {
            LevelsRealtimeAccess peaks(mLastPeakMeasure);
            peaks->Levels.assign(numberOfInputChannels, sMinimalReturnValue);
            peaks->MaxLevels.assign(numberOfInputChannels, sMinimalReturnValue);
        }

        mPeakForIndividualChannelsReturn.assign(numberOfInputChannels, sMinimalReturnValue);
        mMaxPeakForIndividualChannelsReturn.assign(numberOfInputChannels, sMinimalReturnValue);

        Reset();
    }

    void RMSMeter::ProcessBlock(const choc::buffer::ChannelArrayView<const float>& buffer)
    {
        if (mFreezeLoudnessRangeOnSilence)
        {
            // Detect if the block is silent.
            // ------------------------------
            static const float silenceThreshold = std::pow(10, 0.1 * -120);
            // -120dB -> approx. 1.0e-12

            const float magnitude = SampleBufferOperations::GetMagnitude(buffer, 0, buffer.getNumFrames());
            mCurrentBlockIsSilent = magnitude < silenceThreshold;
        }

        LevelsRealtimeAccess lastRMSMearue(mLastRMSMeasureTime);

        // Intermezzo: Set the number of channels.
        // ---------------------------------------
        // To prevent bad acces exception when the number of channels in the buffer
        // suddenly changes without calling Prepare() in advance.
        const int numberOfChannels = std::min({ buffer.getNumChannels(),
                                              static_cast<uint32>(mBin.size()),
                                              static_cast<uint32>(mPeakBin.size()),
                                              static_cast<uint32>(lastRMSMearue->Levels.size())});

        JPL_ASSERT(buffer.getNumChannels() == mBin.size());
        JPL_ASSERT(buffer.getNumChannels() == lastRMSMearue->Levels.size());

        // Accumulate the samples and put the sum(s) into the right bin(s).
        // ----------------------------------------------------------------

        // If the new samples from the buffer can all be added to the same bin.
        if (mNumberOfSamplesInTheCurrentBin + buffer.getNumFrames() < mNumberOfSamplesPerBin)
        {
            for (uint32 k = 0; k < numberOfChannels; ++k)
            {
                const float* bufferOfChannelK = buffer.getChannel(k).data.data;
                double& theBinToSumTo = mBin[k][mCurrentBin];

                for (uint32 i = 0; i < buffer.getNumFrames(); ++i)
                {
                    theBinToSumTo += bufferOfChannelK[i] * bufferOfChannelK[i];
                }

                const auto mag = static_cast<double>(SampleBufferOperations::GetMagnitude(buffer.getChannel(k), 0, buffer.getNumFrames()));
                mPeakBin[k][mCurrentPeakBin] = std::max(mPeakBin[k][mCurrentPeakBin], mag);
            }

            mNumberOfSamplesInTheCurrentBin += buffer.getNumFrames();
        }
        else
        {
            // If the new samples are split up between two (or more (which would be a
            // strange setup)) bins.

            int positionInBuffer = 0;
            bool bufferStillContainsSamples = true;

            while (bufferStillContainsSamples)
            {
                // Figure out if the remaining samples in the buffer can all be
                // accumulated to the current bin.
                const int numberOfSamplesLeftInTheBuffer = buffer.getNumFrames() - positionInBuffer;
                int numberOfSamplesToPutIntoTheCurrentBin;

                if (numberOfSamplesLeftInTheBuffer < mNumberOfSamplesPerBin - mNumberOfSamplesInTheCurrentBin)
                {
                    // Case 1: Partially fill a bin (by using all the samples left in the buffer).
                    // ---------------------------------------------------------------------------
                    // If all the samples from the buffer can be added to the
                    // current bin.
                    numberOfSamplesToPutIntoTheCurrentBin = numberOfSamplesLeftInTheBuffer;
                    bufferStillContainsSamples = false;
                }
                else
                {
                    // Case 2: Completely fill a bin (most likely the buffer will still contain some samples for the next bin).
                    // --------------------------------------------------------------------------------------------------------
                    // Accumulate samples to the current bin until it is full.
                    numberOfSamplesToPutIntoTheCurrentBin = mNumberOfSamplesPerBin - mNumberOfSamplesInTheCurrentBin;
                }

                // Add the samples to the bin.
                for (uint32 k = 0; k < numberOfChannels; ++k)
                {
                    const float* bufferOfChannelK = buffer.getChannel(k).data.data;
                    double& theBinToSumTo = mBin[k][mCurrentBin];
                    for (uint32 i = positionInBuffer; i < (positionInBuffer + numberOfSamplesToPutIntoTheCurrentBin); ++i)
                    {
                        theBinToSumTo += bufferOfChannelK[i] * bufferOfChannelK[i];
                    }

					const auto mag = static_cast<double>(SampleBufferOperations::GetMagnitude(buffer.getChannel(k),
																							  positionInBuffer,
																							  numberOfSamplesToPutIntoTheCurrentBin));

                    mPeakBin[k][mCurrentPeakBin] = std::max(mPeakBin[k][mCurrentPeakBin], mag);
                }
                mNumberOfSamplesInTheCurrentBin += numberOfSamplesToPutIntoTheCurrentBin;

                // If there are some samples left in the buffer
                // => A bin has just been completely filled (case 2 above).
                if (bufferStillContainsSamples)
                {
                    positionInBuffer = positionInBuffer + numberOfSamplesToPutIntoTheCurrentBin;

                    // We have completely filled a bin.
                    // This is the moment the larger sums need to be updated.
                    for (uint32 k = 0; k < numberOfChannels; ++k)
                    {
                        // RMS
                        // ====
                        {
                            double sumOfAllBins = 0.0;
                            // which covers the last 1s.

                            for (uint32 b = 0; b < mNumberOfBins; ++b)
                                sumOfAllBins += mBin[k][b];

                            lastRMSMearue->Levels[k] = std::sqrt(sumOfAllBins * mInvNumberOfSamplesInAllBins);

                            // Maximum
                            if (lastRMSMearue->Levels[k] > lastRMSMearue->MaxLevels[k])
                                lastRMSMearue->MaxLevels[k] = lastRMSMearue->Levels[k];
                        }

                        // Peak
                        // ====
                        {
                            double sumOfAllPeakBins = 0.0;
                            for (uint32 b = 0; b < mNumberOfBinsToCoverPeakMs; ++b)
                                sumOfAllPeakBins += mPeakBin[k][b];

                            LevelsRealtimeAccess peakLevels(mLastPeakMeasure);

                            peakLevels->Levels[k] = static_cast<float>(*std::max_element(mPeakBin[k].begin(), mPeakBin[k].end()));

                            // Maximum
                            if (peakLevels->Levels[k] > peakLevels->MaxLevels[k])
                                peakLevels->MaxLevels[k] = peakLevels->Levels[k];
                        }
                    }

                    // Move on to the next bin
                    mCurrentBin = (mCurrentBin + 1) % mNumberOfBins;
                    mCurrentPeakBin = (mCurrentPeakBin + 1) % mNumberOfBinsToCoverPeakMs;

                    // Set it to zero.
                    for (uint32 k = 0; k < numberOfChannels; ++k)
                    {
                        mBin[k][mCurrentBin] = 0.0;
                        mPeakBin[k][mCurrentPeakBin] = 0.0;
                    }
                    mNumberOfSamplesInTheCurrentBin = 0;
                }
            }
        }
    }

    const std::vector<float>& RMSMeter::GetRMSForIndividualChannels()
    {
        LevelsNonRealtimeAccess rmsLevels(mLastRMSMeasureTime);

        for (uint32 i = 0; i < rmsLevels->Levels.size(); ++i)
        {
            mAverageRMSForIndividualChannelsReturn[i] = std::max(rmsLevels->Levels[i], sMinimalReturnValue);
        }

        return mAverageRMSForIndividualChannelsReturn;
    }

    const std::vector<float>& RMSMeter::GetMaxRMSForIndividualChannels()
    {
        LevelsNonRealtimeAccess rmsLevels(mLastRMSMeasureTime);

        for (uint32 i = 0; i < rmsLevels->MaxLevels.size(); ++i)
        {
            mMaxRMSForIndividualChannelsReturn[i] = std::max(rmsLevels->MaxLevels[i], sMinimalReturnValue);
        }

        return mMaxRMSForIndividualChannelsReturn;
    }

    void RMSMeter::SetFreezeLoudnessRangeOnSilence(bool freeze)
    {
        mFreezeLoudnessRangeOnSilence = freeze;
    }

    const std::vector<float>& RMSMeter::GetPeakForIndividualChannels()
    {
        LevelsNonRealtimeAccess peakLevels(mLastPeakMeasure);

        for (uint32 i = 0; i < peakLevels->Levels.size(); ++i)
        {
            mPeakForIndividualChannelsReturn[i] = peakLevels->Levels[i];
        }

        return mPeakForIndividualChannelsReturn;
    }

    const std::vector<float>& RMSMeter::GetMaxPeakForIndividualChannels()
    {
        LevelsNonRealtimeAccess peakLevels(mLastPeakMeasure);

        for (uint32 i = 0; i < peakLevels->MaxLevels.size(); ++i)
        {
            mMaxPeakForIndividualChannelsReturn[i] = peakLevels->MaxLevels[i];
        }

        return mMaxPeakForIndividualChannelsReturn;
    }

    void RMSMeter::Reset()
    {
        // the bins
        // It is important to use assign() (replace all values) and not
        // resize() (only set new elements to the provided value).
        mBin.assign(mBin.size(), std::vector<double>(mNumberOfBins, 0.0));
        mPeakBin.assign(mPeakBin.size(), std::vector<double>(mNumberOfBinsToCoverPeakMs, 0.0));

        // To ensure the returned peak and RMS are at its 
        // minimum, even if no audio is processed at the moment.

        //! We must not be running process callback when we call reset!
        {
            LevelsRealtimeAccess rmsLevels(mLastRMSMeasureTime);
            rmsLevels->Levels.assign(rmsLevels->Levels.size(), 0.0f);
            rmsLevels->MaxLevels.assign(rmsLevels->MaxLevels.size(), 0.0f);
        }
        {
            LevelsRealtimeAccess peakLevels(mLastPeakMeasure);
            peakLevels->Levels.assign(peakLevels->Levels.size(), sMinimalReturnValue);
            peakLevels->MaxLevels.assign(peakLevels->MaxLevels.size(), sMinimalReturnValue);
        }

        mAverageRMSForIndividualChannelsReturn.assign(mAverageRMSForIndividualChannelsReturn.size(), sMinimalReturnValue);
        mMaxRMSForIndividualChannelsReturn.assign(mMaxRMSForIndividualChannelsReturn.size(), sMinimalReturnValue);

        mPeakForIndividualChannelsReturn.assign(mPeakForIndividualChannelsReturn.size(), sMinimalReturnValue);
        mMaxPeakForIndividualChannelsReturn.assign(mMaxPeakForIndividualChannelsReturn.size(), sMinimalReturnValue);
    }
} // namespace JPL
