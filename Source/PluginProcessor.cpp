#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <limits>

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout AuroraAudioProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    using Range = NormalisableRange<float>;

    auto f = [&] (const String& id, const String& name, float lo, float hi, float def,
                  const String& unit = {}, float skew = 1.f, float step = 0.f)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                           Range (lo, hi, step, skew), def,
                                                           AudioParameterFloatAttributes().withLabel (unit)));
    };

    // I/O
    f ("io_in",  "Gain In",  -24.f, 24.f, 0.f, "dB");
    f ("io_out", "Gain Out", -24.f, 24.f, 0.f, "dB");
    f ("io_mix", "Dry/Wet",  0.f, 1.f, 1.f);

    // EQ
    f ("eq_lowG",  "Low",    -18.f, 18.f, 0.f, "dB");
    f ("eq_lowF",  "Low Hz", 20.f, 500.f, 100.f, "Hz", 0.5f);
    f ("eq_midG",  "Mid",    -18.f, 18.f, 0.f, "dB");
    f ("eq_midF",  "Mid Hz", 200.f, 8000.f, 1000.f, "Hz", 0.4f);
    f ("eq_midQ",  "Mid Q",  0.2f, 10.f, 0.7f, {}, 0.5f);
    f ("eq_highG", "High",   -18.f, 18.f, 0.f, "dB");
    f ("eq_highF", "High Hz", 2000.f, 18000.f, 8000.f, "Hz", 0.5f);

    // Saturation
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "sat_type", 1 }, "Type",
                                                        StringArray { "Tanh", "Soft", "Hard", "Fold" }, 0));
    f ("sat_drive", "Drive", 0.f, 36.f, 6.f, "dB");
    f ("sat_mix",   "Mix",   0.f, 1.f, 0.f);

    // Chorus
    f ("cho_rate",  "Rate",  0.1f, 5.f, 0.8f, "Hz", 0.5f);
    f ("cho_depth", "Depth", 0.f, 1.f, 0.4f);
    f ("cho_mix",   "Mix",   0.f, 1.f, 0.f);

    // Delay
    f ("dly_time", "Time",     1.f, 1500.f, 375.f, "ms", 0.5f);
    f ("dly_fb",   "Feedback", 0.f, 0.95f, 0.35f);
    f ("dly_tone", "Tone",     500.f, 16000.f, 6000.f, "Hz", 0.4f);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "dly_pp", 1 }, "Ping-Pong", true));
    f ("dly_mix",  "Mix",      0.f, 1.f, 0.f);

    // Reverb
    f ("rev_size",  "Size",  0.f, 1.f, 0.5f);
    f ("rev_damp",  "Damp",  0.f, 1.f, 0.5f);
    f ("rev_width", "Width", 0.f, 1.f, 1.f);
    f ("rev_mix",   "Mix",   0.f, 1.f, 0.f);

    // Compresseur
    f ("cmp_thresh",  "Thresh",  -60.f, 0.f, 0.f, "dB");
    f ("cmp_ratio",   "Ratio",   1.f, 20.f, 1.f, ":1", 0.5f);
    f ("cmp_attack",  "Attack",  1.f, 200.f, 10.f, "ms", 0.4f);
    f ("cmp_release", "Release", 10.f, 1000.f, 120.f, "ms", 0.4f);
    f ("cmp_makeup",  "Makeup",  0.f, 24.f, 0.f, "dB");

    // Auto-pan / largeur
    f ("pan_rate",  "Rate",  0.05f, 10.f, 1.f, "Hz", 0.5f);
    f ("pan_depth", "Depth", 0.f, 1.f, 0.f);
    f ("pan_width", "Width", 0.f, 2.f, 1.f);

    return layout;
}

AuroraAudioProcessor::AuroraAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id); };

    pIn = p ("io_in"); pOut = p ("io_out"); pMix = p ("io_mix");
    pLowG = p ("eq_lowG"); pLowF = p ("eq_lowF"); pMidG = p ("eq_midG"); pMidF = p ("eq_midF");
    pMidQ = p ("eq_midQ"); pHighG = p ("eq_highG"); pHighF = p ("eq_highF");
    pSatType = p ("sat_type"); pDrive = p ("sat_drive"); pSatMix = p ("sat_mix");
    pChoRate = p ("cho_rate"); pChoDepth = p ("cho_depth"); pChoMix = p ("cho_mix");
    pDlyTime = p ("dly_time"); pDlyFb = p ("dly_fb"); pDlyTone = p ("dly_tone");
    pDlyPP = p ("dly_pp"); pDlyMix = p ("dly_mix");
    pRevSize = p ("rev_size"); pRevDamp = p ("rev_damp"); pRevWidth = p ("rev_width"); pRevMix = p ("rev_mix");
    pCmpThresh = p ("cmp_thresh"); pCmpRatio = p ("cmp_ratio"); pCmpAtk = p ("cmp_attack");
    pCmpRel = p ("cmp_release"); pCmpMakeup = p ("cmp_makeup");
    pPanRate = p ("pan_rate"); pPanDepth = p ("pan_depth"); pPanWidth = p ("pan_width");
}

bool AuroraAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void AuroraAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };

    for (auto* g : { &inGain, &makeupGain, &outGain })
    {
        g->prepare (spec);
        g->setRampDurationSeconds (0.05);
    }

    eqLow.prepare (spec); eqMid.prepare (spec); eqHigh.prepare (spec);
    eqCache.fill (std::numeric_limits<float>::quiet_NaN());

    oversampling = std::make_unique<dsp::Oversampling<float>> (
        2, 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) samplesPerBlock);
    oversampling->reset();
    const int latency = (int) std::round (oversampling->getLatencyInSamples());
    setLatencySamples (latency);

    chorus.prepare (spec);
    compressor.prepare (spec);

    maxDelaySamples = (float) (2.0 * sampleRate);
    delayLine.setMaximumDelayInSamples ((int) maxDelaySamples + 2);
    delayLine.prepare (spec);
    delayLine.reset();
    delaySmooth.reset (sampleRate, 0.08);
    delaySmooth.setCurrentAndTargetValue (jlimit (1.f, maxDelaySamples, pDlyTime->load() * 0.001f * (float) sampleRate));
    lpL = lpR = 0.f;

    reverb.prepare (spec);
    reverb.reset();

    limiter.prepare (spec);
    limiter.setThreshold (-0.3f);
    limiter.setRelease (60.f);

    mixer.prepare (spec);
    mixer.setWetLatency ((float) latency);

    panPhase = 0.f;
}

void AuroraAudioProcessor::updateEq()
{
    const std::array<float, 7> cur { pLowG->load(), pLowF->load(), pMidG->load(), pMidF->load(),
                                     pMidQ->load(), pHighG->load(), pHighF->load() };
    if (cur == eqCache)   // NaN au premier appel => toujours different
        return;
    eqCache = cur;

    const double sr = getSampleRate();
    const auto maxF = (float) (sr * 0.45);

    *eqLow.state  = *Coef::makeLowShelf  (sr, jmin (cur[1], maxF), 0.707f, Decibels::decibelsToGain (cur[0]));
    *eqMid.state  = *Coef::makePeakFilter (sr, jmin (cur[3], maxF), cur[4], Decibels::decibelsToGain (cur[2]));
    *eqHigh.state = *Coef::makeHighShelf (sr, jmin (cur[6], maxF), 0.707f, Decibels::decibelsToGain (cur[5]));
}

void AuroraAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 2)
        return;

    const int n = buffer.getNumSamples();
    const double sr = getSampleRate();

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> ctx (block);

    // Dry/Wet global
    mixer.setWetMixProportion (pMix->load());
    mixer.pushDrySamples (block);

    // Gain d'entree
    inGain.setGainDecibels (pIn->load());
    inGain.process (ctx);

    // EQ 3 bandes
    updateEq();
    eqLow.process (ctx);
    eqMid.process (ctx);
    eqHigh.process (ctx);

    // Saturation (sur-echantillonnage x2)
    {
        auto osBlock = oversampling->processSamplesUp (block);
        const float driveDb = pDrive->load();
        const float drive = Decibels::decibelsToGain (driveDb);
        const float comp = Decibels::decibelsToGain (-driveDb * 0.6f);
        const float m = pSatMix->load();
        const int type = (int) pSatType->load();

        for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
        {
            auto* d = osBlock.getChannelPointer (ch);
            for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
            {
                const float x = d[i];
                const float y = x * drive;
                float s;
                switch (type)
                {
                    case 0:  s = std::tanh (y); break;
                    case 1:  s = y / (1.f + std::abs (y)); break;
                    case 2:  s = jlimit (-1.f, 1.f, y); break;
                    default: s = std::sin (y * 1.5707963f); break;
                }
                d[i] = x * (1.f - m) + s * comp * m;
            }
        }
        oversampling->processSamplesDown (block);
    }

    // Chorus
    chorus.setRate (pChoRate->load());
    chorus.setDepth (pChoDepth->load());
    chorus.setCentreDelay (7.f);
    chorus.setFeedback (0.f);
    chorus.setMix (pChoMix->load());
    chorus.process (ctx);

    // Compresseur + makeup
    compressor.setThreshold (pCmpThresh->load());
    compressor.setRatio (pCmpRatio->load());
    compressor.setAttack (pCmpAtk->load());
    compressor.setRelease (pCmpRel->load());
    compressor.process (ctx);
    makeupGain.setGainDecibels (pCmpMakeup->load());
    makeupGain.process (ctx);

    // Delay stereo / ping-pong avec filtre dans la boucle de feedback
    {
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);
        const float fb = jlimit (0.f, 0.95f, pDlyFb->load());
        const float mix = pDlyMix->load();
        const bool pp = pDlyPP->load() > 0.5f;
        const float a = 1.f - std::exp (-MathConstants<float>::twoPi * pDlyTone->load() / (float) sr);
        delaySmooth.setTargetValue (jlimit (1.f, maxDelaySamples, pDlyTime->load() * 0.001f * (float) sr));

        for (int i = 0; i < n; ++i)
        {
            const float d = delaySmooth.getNextValue();
            const float dl = delayLine.popSample (0, d, true);
            const float dr = delayLine.popSample (1, d, true);
            lpL += a * (dl - lpL);
            lpR += a * (dr - lpR);

            const float inL = l[i], inR = r[i];
            if (pp)
            {
                delayLine.pushSample (0, (inL + inR) * 0.5f + lpR * fb);
                delayLine.pushSample (1, lpL * fb);
            }
            else
            {
                delayLine.pushSample (0, inL + lpL * fb);
                delayLine.pushSample (1, inR + lpR * fb);
            }

            l[i] = inL + dl * mix;
            r[i] = inR + dr * mix;
        }
    }

    // Reverb
    {
        const float m = pRevMix->load();
        dsp::Reverb::Parameters rp;
        rp.roomSize = pRevSize->load();
        rp.damping = pRevDamp->load();
        rp.width = pRevWidth->load();
        rp.wetLevel = m * 0.33f;
        rp.dryLevel = 1.f - m;
        rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        reverb.process (ctx);
    }

    // Auto-pan + largeur M/S
    {
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);
        const float depth = pPanDepth->load();
        const float width = pPanWidth->load();
        const float inc = MathConstants<float>::twoPi * pPanRate->load() / (float) sr;
        float ph = panPhase;

        for (int i = 0; i < n; ++i)
        {
            const float s = std::sin (ph);
            ph += inc;
            if (ph > MathConstants<float>::twoPi) ph -= MathConstants<float>::twoPi;

            const float gl = 1.f - depth * 0.5f * (1.f + s);
            const float gr = 1.f - depth * 0.5f * (1.f - s);
            const float mid = (l[i] + r[i]) * 0.5f;
            const float side = (l[i] - r[i]) * 0.5f * width;
            l[i] = (mid + side) * gl;
            r[i] = (mid - side) * gr;
        }
        panPhase = ph;
    }

    // Gain de sortie + limiteur
    outGain.setGainDecibels (pOut->load());
    outGain.process (ctx);
    limiter.process (ctx);

    mixer.mixWetSamples (block);
}

AudioProcessorEditor* AuroraAudioProcessor::createEditor()
{
    return new AuroraAudioProcessorEditor (*this);
}

void AuroraAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AuroraAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuroraAudioProcessor();
}
