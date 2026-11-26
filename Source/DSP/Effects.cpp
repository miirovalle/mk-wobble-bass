#include "DSP/Effects.h"

namespace mk
{

// Modulation and gain coefficients are refreshed once per 32 samples: cheap
// enough for an iPad, inaudible for anything slower than a click.
constexpr int kControlChunk = 32;

void MasterEffects::prepare (double sampleRate, bool ecoMode)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;

    chorusLine.assign (2048, 0.0f);
    flangerLine.assign (1024, 0.0f);
    chorusWrite = flangerWrite = 0;

    // Eco halves the delay memory, High keeps a full 1.6 s stereo delay.
    delayMax = clampValue ((int) (sr * (ecoMode ? 0.8 : 1.6)), 1024, 262144);
    delayLine.assign ((size_t) delayMax, 0.0f);
    delayWrite = 0;

    static const int combTuning[kCombs] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    static const int allpassTuning[kAllpasses] = { 556, 441, 341, 225 };

    for (int i = 0; i < kCombs; ++i)
        combBuffer[i].assign ((size_t) (combTuning[i] * (ecoMode ? 1 : 1)), 0.0f);
    for (int i = 0; i < kAllpasses; ++i)
        allpassBuffer[i].assign ((size_t) allpassTuning[i], 0.0f);

    xoverCoeffLo = 1.0f - std::exp (-kTwoPi * 120.0f / (float) sr);
    xoverCoeffHi = 1.0f - std::exp (-kTwoPi * 3000.0f / (float) sr);
    ottAttackCoeff = 1.0f - std::exp (-1.0f / (0.0015f * (float) sr));
    ottReleaseCoeff = 1.0f - std::exp (-1.0f / (0.08f * (float) sr));

    reset();
}

void MasterEffects::reset()
{
    std::fill (chorusLine.begin(), chorusLine.end(), 0.0f);
    std::fill (flangerLine.begin(), flangerLine.end(), 0.0f);
    std::fill (delayLine.begin(), delayLine.end(), 0.0f);
    for (auto& b : combBuffer)
        std::fill (b.begin(), b.end(), 0.0f);
    for (auto& b : allpassBuffer)
        std::fill (b.begin(), b.end(), 0.0f);

    for (int i = 0; i < kCombs; ++i)
        combIndex[i] = 0;
    for (int i = 0; i < kAllpasses; ++i)
        allpassIndex[i] = 0;

    std::fill (std::begin (combStore), std::end (combStore), 0.0f);
    std::fill (std::begin (allpassStore), std::end (allpassStore), 0.0f);
    std::fill (std::begin (phaserZ[0]), std::end (phaserZ[0]), 0.0f);
    std::fill (std::begin (phaserZ[1]), std::end (phaserZ[1]), 0.0f);

    chorusPhase = flangerPhase = phaserPhase = 0.0f;
    chorusLp[0] = chorusLp[1] = flangerLp[0] = flangerLp[1] = 0.0f;
    delayDamp[0] = delayDamp[1] = 0.0f;
    reverbLp[0] = reverbLp[1] = 0.0f;
    compEnv = 0.0f;
    bandLow[0] = bandLow[1] = bandHigh[0] = bandHigh[1] = 0.0f;
    ottLow[0] = ottLow[1] = ottHigh[0] = ottHigh[1] = 0.0f;
    for (auto& band : ottEnv)
        band[0] = band[1] = 0.0f;
    dcState[0] = dcState[1] = 0.0f;
}

void MasterEffects::setSettings (const EffectSettings& next) noexcept
{
    settings = next;
    compAttackCoeff = 1.0f - std::exp (-1.0f / (std::max (next.compAttackMs, 0.1f) * 0.001f * (float) sr));
    compReleaseCoeff = 1.0f - std::exp (-1.0f / (std::max (next.compReleaseMs, 1.0f) * 0.001f * (float) sr));
}

void MasterEffects::process (float* left, float* right, int numSamples) noexcept
{
    if (settings.chorusOn)
        processChorus (left, right, numSamples);
    if (settings.flangerOn)
        processFlanger (left, right, numSamples);
    if (settings.phaserOn)
        processPhaser (left, right, numSamples);
    if (settings.delayOn && settings.delayMix > 0.0005f)
        processDelay (left, right, numSamples);
    if (settings.reverbOn && settings.reverbMix > 0.0005f)
        processReverb (left, right, numSamples);
    if (settings.multibandOn && settings.multibandMix > 0.0005f)
        processMultiband (left, right, numSamples);
    if (settings.ottOn && settings.ottMix > 0.0005f)
        processOtt (left, right, numSamples);
    if (settings.compressorOn)
        processCompressor (left, right, numSamples);
}

void MasterEffects::processChorus (float* l, float* r, int n) noexcept
{
    const float rate = clampf (settings.chorusRate, 0.01f, 10.0f);
    const float inc = rate / (float) sr;
    const float depth = clampf (settings.chorusDepth, 0.0f, 1.0f);
    const float mix = clampf (settings.chorusMix, 0.0f, 1.0f);
    const float width = clampf (settings.chorusWidth, 0.0f, 1.0f);
    const float base = 0.012f * (float) sr;
    const float swing = 0.006f * (float) sr * depth;
    const int size = (int) chorusLine.size();
    const int mask = size - 1;

    for (int i = 0; i < n; ++i)
    {
        chorusPhase += inc;
        if (chorusPhase >= 1.0f)
            chorusPhase -= 1.0f;

        const float modL = SineTable::instance().lookup (chorusPhase);
        const float modR = SineTable::instance().lookup (chorusPhase + 0.25f);

        chorusLine[(size_t) chorusWrite] = (l[i] + r[i]) * 0.5f;

        auto read = [&] (float offset)
        {
            float pos = (float) chorusWrite - (base + offset);
            while (pos < 0.0f)
                pos += (float) size;
            const int i0 = (int) pos & mask;
            const int i1 = (i0 + 1) & mask;
            const float f = pos - std::floor (pos);
            return lerp (chorusLine[(size_t) i0], chorusLine[(size_t) i1], f);
        };

        const float wetL = read (modL * swing);
        const float wetR = read (modR * swing);

        chorusLp[0] += (wetL - chorusLp[0]) * 0.5f;
        chorusLp[1] += (wetR - chorusLp[1]) * 0.5f;

        const float midL = l[i] + (chorusLp[0] - l[i]) * mix;
        const float midR = r[i] + (chorusLp[1] - r[i]) * mix;
        const float side = (midL - midR) * width;

        l[i] = midL - side * 0.5f;
        r[i] = midR + side * 0.5f;

        if (++chorusWrite >= size)
            chorusWrite = 0;
    }
}

void MasterEffects::processFlanger (float* l, float* r, int n) noexcept
{
    const float inc = clampf (settings.flangerRate, 0.01f, 10.0f) / (float) sr;
    const float depth = clampf (settings.flangerDepth, 0.0f, 1.0f);
    const float fb = clampf (settings.flangerFb, 0.0f, 0.95f);
    const float mix = clampf (settings.flangerMix, 0.0f, 1.0f);
    const float base = clampf (settings.flangerDelay, 0.0002f, 0.02f) * (float) sr;
    const float swing = base * 0.8f * depth;
    const int size = (int) flangerLine.size();
    const int mask = size - 1;

    for (int i = 0; i < n; ++i)
    {
        flangerPhase += inc;
        if (flangerPhase >= 1.0f)
            flangerPhase -= 1.0f;

        const float mod = SineTable::instance().lookup (flangerPhase);
        const float pos = (float) flangerWrite - (base + mod * swing);
        float wrapped = pos;
        while (wrapped < 0.0f)
            wrapped += (float) size;

        const int i0 = (int) wrapped & mask;
        const int i1 = (i0 + 1) & mask;
        const float wet = lerp (flangerLine[(size_t) i0], flangerLine[(size_t) i1],
                                wrapped - std::floor (wrapped));

        flangerLp[0] += (wet - flangerLp[0]) * 0.35f;
        flangerLine[(size_t) flangerWrite] = flushDenormal ((l[i] + r[i]) * 0.5f
                                                            + flangerLp[0] * fb);

        l[i] += (wet - l[i]) * mix * 0.8f;
        r[i] += (wet - r[i]) * mix * 0.8f;

        if (++flangerWrite >= size)
            flangerWrite = 0;
    }
}

void MasterEffects::processPhaser (float* l, float* r, int n) noexcept
{
    const float inc = clampf (settings.phaserRate, 0.01f, 10.0f) / (float) sr;
    const float depth = clampf (settings.phaserDepth, 0.0f, 1.0f);
    const float fb = clampf (settings.phaserFb, 0.0f, 0.95f);
    const float mix = clampf (settings.phaserMix, 0.0f, 1.0f);
    const int stages = clampValue (settings.phaserStages, 2, 12);

    // Coefficients only change at control rate: two tan() calls per 32 samples
    // instead of per sample keeps this cheap on an iPad.
    int done = 0;

    while (done < n)
    {
        const int chunk = std::min (kControlChunk, n - done);

        phaserPhase += inc * (float) chunk;
        if (phaserPhase >= 1.0f)
            phaserPhase -= std::floor (phaserPhase);

        const float modL = 0.5f + 0.5f * SineTable::instance().lookup (phaserPhase);
        const float modR = 0.5f + 0.5f * SineTable::instance().lookup (phaserPhase + 0.25f);
        const float freqL = 200.0f * std::exp2 (modL * depth * 5.0f);
        const float freqR = 200.0f * std::exp2 (modR * depth * 5.0f);

        const float tanL = fastTan (kPi * std::min (freqL, (float) sr * 0.45f) / (float) sr);
        const float tanR = fastTan (kPi * std::min (freqR, (float) sr * 0.45f) / (float) sr);
        const float coeffL = (1.0f - tanL) / (1.0f + tanL);
        const float coeffR = (1.0f - tanR) / (1.0f + tanR);

        for (int i = done; i < done + chunk; ++i)
        {
            float xL = l[i] + phaserZ[0][stages - 1] * fb;
            float xR = r[i] + phaserZ[1][stages - 1] * fb;

            for (int s = 0; s < stages; ++s)
            {
                const float yL = coeffL * xL + phaserZ[0][s];
                phaserZ[0][s] = xL - coeffL * yL;
                xL = yL;

                const float yR = coeffR * xR + phaserZ[1][s];
                phaserZ[1][s] = xR - coeffR * yR;
                xR = yR;
            }

            l[i] += (xL - l[i]) * mix;
            r[i] += (xR - r[i]) * mix;
        }

        done += chunk;
    }
}

void MasterEffects::processDelay (float* l, float* r, int n) noexcept
{
    // The line is interleaved L/R, so the delay length is forced even.
    const int delaySamples = 2 * clampValue ((int) (settings.delaySeconds * (float) sr * 0.5f),
                                             4, (delayMax - 2) / 2);
    const float fb = clampf (settings.delayFeedback, 0.0f, 0.95f);
    const float mix = clampf (settings.delayMix, 0.0f, 1.0f);
    const float dampCoeff = 0.02f + clampf (settings.delayTone, 0.0f, 1.0f) * 0.7f;
    const bool ping = settings.delayPingPong;

    for (int i = 0; i < n; ++i)
    {
        int readPos = delayWrite - delaySamples;
        if (readPos < 0)
            readPos += delayMax;

        const float wetL = delayLine[(size_t) readPos];
        const float wetR = delayLine[(size_t) (readPos + 1 < delayMax ? readPos + 1 : 0)];

        delayDamp[0] += (wetL - delayDamp[0]) * dampCoeff;
        delayDamp[1] += (wetR - delayDamp[1]) * dampCoeff;

        const float inL = ping ? l[i] + delayDamp[1] * fb : l[i] + delayDamp[0] * fb;
        const float inR = ping ? r[i] + delayDamp[0] * fb : r[i] + delayDamp[1] * fb;

        delayLine[(size_t) delayWrite] = flushDenormal (inL);
        const int secondWrite = delayWrite + 1 < delayMax ? delayWrite + 1 : 0;
        delayLine[(size_t) secondWrite] = flushDenormal (inR);

        l[i] += (delayDamp[0] - l[i]) * mix;
        r[i] += (delayDamp[1] - r[i]) * mix;

        delayWrite += 2;
        if (delayWrite >= delayMax)
            delayWrite -= delayMax;
    }
}

void MasterEffects::processReverb (float* l, float* r, int n) noexcept
{
    const float sizeScale = 0.5f + clampf (settings.reverbSize, 0.0f, 1.0f) * 0.9f;
    const float decay = 0.72f + clampf (settings.reverbDecay, 0.0f, 1.0f) * 0.26f;
    const float damp = clampf (settings.reverbDamp, 0.0f, 1.0f) * 0.6f;
    const float width = clampf (settings.reverbWidth, 0.0f, 1.0f);
    const float mix = clampf (settings.reverbMix, 0.0f, 1.0f);
    static const float spread[kCombs] = { 0.0f, 0.13f, 0.29f, 0.41f, 0.53f, 0.67f, 0.79f, 0.91f };

    // Comb lengths depend only on the size control, so resolve them once per
    // block instead of eight times per sample.
    int combLen[kCombs] {};
    for (int c = 0; c < kCombs; ++c)
        combLen[c] = clampValue ((int) ((float) combBuffer[c].size() * sizeScale
                                        * (1.0f + spread[c] * 0.2f)),
                                 16, (int) combBuffer[c].size() - 1);

    for (int i = 0; i < n; ++i)
    {
        const float input = (l[i] + r[i]) * 0.5f;
        float sum = 0.0f;

        for (int c = 0; c < kCombs; ++c)
        {
            auto& buffer = combBuffer[c];
            const int len = combLen[c];
            int& index = combIndex[c];
            if (index >= len)
                index = 0;

            const float delayed = buffer[(size_t) index];
            combStore[c] = flushDenormal (delayed * (1.0f - damp) + combStore[c] * damp);
            buffer[(size_t) index] = input + combStore[c] * decay;
            if (++index >= len)
                index = 0;
            sum += delayed;
        }

        for (int a = 0; a < kAllpasses; ++a)
        {
            auto& buffer = allpassBuffer[a];
            const int len = (int) buffer.size();
            int& index = allpassIndex[a];

            const float delayed = buffer[(size_t) index];
            const float out = -sum + delayed;
            buffer[(size_t) index] = flushDenormal (sum + delayed * 0.5f);
            if (++index >= len)
                index = 0;
            allpassStore[a] = out;
            sum = out;
        }

        reverbLp[0] += (sum - reverbLp[0]) * 0.35f;
        const float wet = reverbLp[0] * 0.6f;

        l[i] += (wet * (0.5f + 0.5f * width) - l[i]) * mix;
        r[i] += (wet * (0.5f + 0.5f * (1.0f - width)) - r[i]) * mix;
    }
}

void MasterEffects::processCompressor (float* l, float* r, int n) noexcept
{
    const float threshold = dbToGain (settings.compThresholdDb);
    const float ratio = std::max (settings.compRatio, 1.0f);
    const float makeup = dbToGain (settings.compMakeupDb);
    const float slope = 1.0f - 1.0f / ratio;

    float maxReduction = 1.0f;
    int done = 0;

    // One pow() per 32 samples instead of per sample: the dominant saving in
    // this chain on ARM.
    while (done < n)
    {
        const int chunk = std::min (kControlChunk, n - done);

        float blockPeak = 0.0f;
        for (int i = done; i < done + chunk; ++i)
            blockPeak = std::max (blockPeak, std::max (std::fabs (l[i]), std::fabs (r[i])));

        const float coeff = blockPeak > compEnv ? compAttackCoeff : compReleaseCoeff;
        compEnv += (blockPeak - compEnv) * coeff;

        float gain = 1.0f;
        if (compEnv > threshold)
        {
            gain = std::pow (compEnv / threshold, -slope);
            maxReduction = std::min (maxReduction, gain);
        }

        const float g = gain * makeup;
        for (int i = done; i < done + chunk; ++i)
        {
            l[i] *= g;
            r[i] *= g;
        }

        done += chunk;
    }

    lastGainReduction = -20.0f * std::log10 (std::max (maxReduction, 1.0e-4f));
    compEnv = flushDenormal (compEnv);
}

void MasterEffects::processMultiband (float* l, float* r, int n) noexcept
{
    const float drive = 1.0f + clampf (settings.multibandDrive, 0.0f, 1.0f) * 6.0f;
    const float mix = clampf (settings.multibandMix, 0.0f, 1.0f);

    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            float& sample = ch == 0 ? l[i] : r[i];

            bandLow[ch] += (sample - bandLow[ch]) * xoverCoeffLo;
            const float low = bandLow[ch];
            bandHigh[ch] += (sample - bandHigh[ch]) * xoverCoeffHi;
            const float high = sample - bandHigh[ch];
            const float mid = bandHigh[ch] - low;

            const float shaped = low + fastTanh (mid * drive) * (1.0f / (1.0f + 0.4f * drive))
                               + fastTanh (high * drive * 1.3f) * (1.0f / (1.0f + 0.6f * drive));

            sample += (shaped - sample) * mix;
            sample = flushDenormal (sample);
        }
    }
}

void MasterEffects::processOtt (float* l, float* r, int n) noexcept
{
    // OTT-style: split into three bands and compress both above and below the
    // threshold, so quiet detail is pushed up while the peaks are held back.
    // The gain curve is a square root, which is cheap enough for the audio
    // thread and gives roughly 2:1 in both directions.
    const float amount = clampf (settings.ottAmount, 0.0f, 1.0f);
    const float mix = clampf (settings.ottMix, 0.0f, 1.0f);
    const float threshold = 0.18f;
    const float makeup = 1.0f + amount * 0.7f;

    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            float& sample = ch == 0 ? l[i] : r[i];

            ottLow[ch] += (sample - ottLow[ch]) * xoverCoeffLo;
            const float low = ottLow[ch];
            ottHigh[ch] += (sample - ottHigh[ch]) * xoverCoeffHi;
            const float high = sample - ottHigh[ch];
            const float mid = ottHigh[ch] - low;

            const float bands[3] = { low, mid, high };
            float wet = 0.0f;

            for (int band = 0; band < 3; ++band)
            {
                const float x = bands[(size_t) band];
                float& env = ottEnv[(size_t) band][ch];
                const float magnitude = std::fabs (x);
                const float coeff = magnitude > env ? ottAttackCoeff : ottReleaseCoeff;
                env += (magnitude - env) * coeff;

                float gain = std::sqrt (threshold / std::max (env, 1.0e-5f));
                gain = clampf (1.0f + (gain - 1.0f) * amount, 0.2f, 8.0f);
                wet += x * gain;
            }

            wet *= makeup;
            sample += (wet - sample) * mix;
            sample = flushDenormal (sample);
        }
    }
}

} // namespace mk