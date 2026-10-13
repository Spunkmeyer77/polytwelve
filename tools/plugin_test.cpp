// End-to-end check of the plugin wrapper: params, programs, state, MIDI rendering, editor snapshot.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir = argc > 1 ? juce::File (argv[1]) : juce::File::getCurrentWorkingDirectory();

    PolysixAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    std::printf ("params: %d  programs: %d\n", (int) proc.getParameters().size(), proc.getNumPrograms());

    for (int prog = 0; prog < proc.getNumPrograms(); ++prog)
    {
        proc.setCurrentProgram (prog);
        proc.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buf (2, 512);
        juce::AudioBuffer<float> all (2, 48000 * 3);
        float peak = 0; int bad = 0;
        for (int blk = 0; blk * 512 < all.getNumSamples(); ++blk)
        {
            juce::MidiBuffer midi;
            const double t = blk * 512 / 48000.0;
            if (blk == 0) for (int n : { 57, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 10);
            if (std::abs (t - 2.0) < 0.006) for (int n : { 57, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
            proc.processBlock (buf, midi);
            const int n = juce::jmin (512, all.getNumSamples() - blk * 512);
            for (int c = 0; c < 2; ++c)
            {
                all.copyFrom (c, blk * 512, buf, c, 0, n);
                for (int i = 0; i < n; ++i)
                {
                    const float v = buf.getSample (c, i);
                    if (! std::isfinite (v)) ++bad;
                    peak = juce::jmax (peak, std::abs (v));
                }
            }
        }
        std::printf ("program %-13s peak %.3f nonfinite %d\n", proc.getProgramName (prog).toRawUTF8(), peak, bad);
        juce::WavAudioFormat wav;
        auto file = outDir.getChildFile ("prog_" + juce::String (prog) + "_" + proc.getProgramName (prog).replaceCharacter (' ', '_') + ".wav");
        file.deleteFile();
        if (auto os = std::unique_ptr<juce::OutputStream> (file.createOutputStream()))
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), 48000.0, 2, 16, {}, 0)))
            { os.release(); w->writeFromAudioSampleBuffer (all, 0, all.getNumSamples()); }
    }

    // state round trip
    proc.setCurrentProgram (5);
    juce::MemoryBlock state; proc.getStateInformation (state);
    proc.setCurrentProgram (0);
    proc.setStateInformation (state.getData(), (int) state.getSize());
    std::printf ("state restore: resonance=%.2f arpOn=%.0f (expect 0.75, 1)\n",
                 proc.apvts.getRawParameterValue ("resonance")->load(), proc.apvts.getRawParameterValue ("arpOn")->load());

    // preset manager
    {
        auto& pm = proc.getPresetManager();
        auto* res = proc.apvts.getParameter ("resonance");
        proc.setCurrentProgram (2);
        res->setValueNotifyingHost (0.33f);
        const bool saved = pm.save ("Test Pad");
        res->setValueNotifyingHost (0.9f);
        const int idx = pm.indexOfKey ("U:Test Pad");
        pm.load (idx);
        std::printf ("presets: saved=%d index=%d resonance after load=%.2f (expect 0.33) current=%s\n",
                     (int) saved, idx, res->getValue(), pm.getCurrentKey().toRawUTF8());
        pm.toggleFavourite ("U:Test Pad");
        pm.toggleFavourite ("F:Bass");
        pm.load (0);
        pm.step (1, true);  std::printf ("fav step 1: %s\n", pm.getCurrentKey().toRawUTF8());
        pm.step (1, true);  std::printf ("fav step 2: %s\n", pm.getCurrentKey().toRawUTF8());
        pm.step (1, true);  std::printf ("fav step 3: %s\n", pm.getCurrentKey().toRawUTF8());
        pm.step (-1, false); std::printf ("all step -1: %s\n", pm.getCurrentKey().toRawUTF8());
        pm.load (pm.indexOfKey ("U:Test Pad"));
        juce::MemoryBlock s2; proc.getStateInformation (s2);
        pm.load (0);
        proc.setStateInformation (s2.getData(), (int) s2.getSize());
        std::printf ("session restore keeps preset name: %s\n", pm.getCurrentKey().toRawUTF8());
        std::printf ("favourites file: %s\n", pm.getUserDir().getChildFile ("favourites.txt").loadFileAsString().replace ("\n", " | ").toRawUTF8());
        pm.toggleFavourite ("U:Test Pad");
        pm.deleteCurrent();
        std::printf ("after delete: current=%s user file exists=%d entries=%d\n", pm.getCurrentKey().toRawUTF8(),
                     (int) pm.userPresetExists ("Test Pad"), (int) pm.getEntries().size());
        pm.save ("My Bass");
        pm.toggleFavourite ("U:My Bass");
    }

    // chord memory via MEMORY button path
    proc.setCurrentProgram (1);
    {
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        for (int n : { 57, 60, 64, 67 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
        proc.processBlock (buf, midi);
        proc.requestChordMemory();
        if (auto* vm = proc.apvts.getParameter ("voiceMode")) vm->setValueNotifyingHost (vm->convertTo0to1 (2.0f));
        midi.clear();
        proc.processBlock (buf, midi);
        for (int n : { 57, 60, 64, 67 }) midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
        proc.processBlock (buf, midi);
        int iv[p6::kNumVoices]; int root = 0;
        const int cn = proc.getChord (iv, root);
        std::printf ("chord memory: root %d, %d notes:", root, cn);
        for (int i = 0; i < cn; ++i) std::printf (" %d", iv[i]);
        std::printf ("  (expect root 57: 0 3 7 10)\n");

        // play one key in chord mode -> 4 voices
        midi.clear(); midi.addEvent (juce::MidiMessage::noteOn (1, 50, (juce::uint8) 100), 0);
        proc.processBlock (buf, midi);
        juce::MemoryBlock st; proc.getStateInformation (st);
        auto xml = juce::AudioProcessor::getXmlFromBinary (st.getData(), (int) st.getSize());
        std::printf ("state chordMemory=%s chordRoot=%s\n", xml->getStringAttribute ("chordMemory").toRawUTF8(), xml->getStringAttribute ("chordRoot").toRawUTF8());
    }

    // editor snapshot
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    std::printf ("editor size %d x %d\n", ed->getWidth(), ed->getHeight());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::PNGImageFormat png;
    auto pngFile = outDir.getChildFile ("editor.png");
    pngFile.deleteFile();
    if (auto os = pngFile.createOutputStream()) png.writeImageToStream (img, *os);
    ed.reset();
    return 0;
}
