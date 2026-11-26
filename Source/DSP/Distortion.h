#pragma once

// Eleven distortion algorithms with a shared drive/tone/mix stage.
//
// The expensive part of a saturator is the nonlinearity itself; everything else
// here is a one-pole tilt and two multiplies. Optional 2x subdivision removes
// the worst aliasing from the hard shapers and the bitcrusher.
//
// Sample-and-hold algorithms keep their phase per channel: a shared counter
// would make one side starve whenever the hold length is even.

#include "DSP/MkCommon.h"
#include "DSP/Params.h"

namespace mk
{

class DistortionUnit
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        toneCoeff = 1.0f - std::exp (-kTwoPi * 1800.0f / (float) sr);
        reset();
    }

    void reset() noexcept
    {
        toneState[0] = toneState[1] = 0.0f;
        prevInput[0] = prevInput[1] = 0.0f;
        holdValue[0] = holdValue[1] = 0.0f;
        crushPhase[0] = crushPhase[1] = 0.0f;
        holdCounter[0] = holdCounter[1] = 0;
    }

    void setParameters (int distortionType, float driveAmount, float toneAmount, float mixAmount,
                        float outputDb, bool oversample) noexcept
    {
        type = distortionType;
        drive = clampf (driveAmount, 0.0f, 1.0f);
        tone = clampf (toneAmount, -1.0f, 1.0f);
        wet = clampf (mixAmount, 0.0f, 1.0f);
        preGain = 1.0f + drive * 22.0f;
        postGain = dbToGain (outputDb) * (1.0f / (1.0f + drive * 2.4f));
        oversample2x = oversample;
        crushStep = 1.0f + drive * 63.0f;
        downFactor = (int) (1.0f + drive * 39.0f);
        active = type != DistOff && wet > 0.0005f;
    }

    inline void process (float& left, float& right) noexcept
    {
        if (! active)
            return;

        const float dryL = left, dryR = right;

        if (oversample2x)
        {
            const float inL = left, inR = right;
            const float midL = lerp (prevInput[0], inL, 0.5f);
            const float midR = lerp (prevInput[1], inR, 0.5f);
            const float aL = saturate (midL * preGain, 0);
            const float aR = saturate (midR * preGain, 1);
            const float bL = saturate (inL * preGain, 0);
            const float bR = saturate (inR * preGain, 1);
            prevInput[0] = inL;
            prevInput[1] = inR;
            left = 0.5f * (aL + bL);
            right = 0.5f * (aR + bR);
        }
        else
        {
            left = saturate (left * preGain, 0);
            right = saturate (right * preGain, 1);
        }

        left = tilt (left * postGain, 0);
        right = tilt (right * postGain, 1);

        left = dryL + (left - dryL) * wet;
        right = dryR + (right - dryR) * wet;
    }

private:
    inline float saturate (float x, int channel) noexcept
    {
        switch (type)
        {
            case DistSoftClip:
                return fastTanh (x);
            case DistHardClip:
                return clampf (x, -1.0f, 1.0f);
            case DistTube:
            {
                const float biased = x + 0.15f;
                return (fastTanh (biased * 1.4f) - 0.2f) * 0.85f;
            }
            case DistDiode:
            {
                // Asymmetric exponential knee.
                const float pos = 1.0f - std::exp (-std::max (x, 0.0f) * 1.8f);
                const float neg = -(1.0f - std::exp (-std::max (-x, 0.0f) * 1.2f)) * 0.8f;
                return x > 0.0f ? pos : neg;
            }
            case DistFoldback:
            {
                float v = x;
                while (v > 1.0f || v < -1.0f)
                    v = std::fabs (std::fabs (std::fmod (v - 1.0f, 4.0f)) - 2.0f) - 1.0f;
                return v;
            }
            case DistWavefold:
                return std::sin (clampf (x, -3.0f, 3.0f) * kPi * 0.5f);
            case DistBitcrush:
            {
                float& phase = crushPhase[channel];
                phase += 1.0f;
                if (phase >= crushStep)
                {
                    phase = 0.0f;
                    holdValue[channel] = std::round (clampf (x, -1.0f, 1.0f) * 16.0f) * (1.0f / 16.0f);
                }
                return holdValue[channel];
            }
            case DistDownsample:
            {
                int& counter = holdCounter[channel];
                if (++counter >= downFactor)
                {
                    counter = 0;
                    holdValue[channel] = clampf (x, -1.0f, 1.0f);
                }
                return holdValue[channel];
            }
            case DistDigital:
            {
                const float q = std::round (clampf (x, -1.0f, 1.0f) * 64.0f) * (1.0f / 64.0f);
                return clampf (q * 1.3f - q * q * q * 0.3f, -1.0f, 1.0f);
            }
            case DistAnalog:
            {
                const float drift = fastTanh (x * 1.1f + 0.05f);
                return drift - drift * drift * drift * 0.22f;
            }
            case DistMultistage:
            {
                float v = fastTanh (x * 0.9f);
                v = fastTanh (v * 1.8f);
                v = fastTanh (v * 3.0f) * 0.7f;
                return v;
            }
            case DistOff:
            default:
                return x;
        }
    }

    inline float tilt (float x, int channel) noexcept
    {
        float& lp = toneState[channel];
        lp += (x - lp) * toneCoeff;
        const float bright = 0.5f + 0.5f * tone;
        const float dark = 0.5f - 0.5f * tone;
        return lp * dark + (x - lp) * bright + lp * 0.5f * bright;
    }

    double sr = 48000.0;
    float toneCoeff = 0.2f;
    float toneState[2] {};
    float prevInput[2] {};
    float holdValue[2] {};
    float crushPhase[2] {};
    int holdCounter[2] {};
    float drive = 0.0f, tone = 0.0f, wet = 1.0f;
    float preGain = 1.0f, postGain = 1.0f;
    float crushStep = 1.0f;
    int downFactor = 1;
    int type = DistOff;
    bool oversample2x = false;
    bool active = false;
};

} // namespace mk
