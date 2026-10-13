#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PolysixAudioProcessor;

// Factory + user presets, favourites, prev/next. Message-thread only.
// User presets (*.p6preset):
//   macOS    ~/Library/Audio/Presets/Ticco Ross/Polytwelve
//   Windows  Documents\Ticco Ross\Polytwelve\Presets
//   Linux    ~/.local/share/Ticco Ross/Polytwelve/Presets
class PresetManager : public juce::ChangeBroadcaster
{
public:
    struct Entry
    {
        juce::String key;      // "F:<name>" factory, "U:<file name>" user
        juce::String name;
        bool factory = false;
        int factoryIndex = -1;
        juce::File file;
    };

    static constexpr const char* extension = ".p6preset";

    explicit PresetManager (PolysixAudioProcessor&);

    juce::File getUserDir() const;
    void refresh();

    const std::vector<Entry>& getEntries() const { return entries; }
    int indexOfKey (const juce::String& key) const;

    juce::String getCurrentKey() const  { return currentKey; }
    juce::String getCurrentName() const;
    bool currentIsUser() const          { return currentKey.startsWith ("U:"); }
    void setCurrentKey (const juce::String& key); // used when restoring the plugin state

    bool load (int index);
    bool loadFile (const juce::File& file);              // any .p6preset (e.g. from Finder)
    bool save (const juce::String& name);                // creates or overwrites a user preset
    bool userPresetExists (const juce::String& name) const;
    bool deleteCurrent();                                // user presets only

    bool isFavourite (const juce::String& key) const { return favourites.contains (key); }
    void toggleFavourite (const juce::String& key);
    bool hasFavourites() const { return ! favourites.isEmpty(); }

    void step (int delta, bool favouritesOnly);

private:
    PolysixAudioProcessor& proc;
    std::vector<Entry> entries;
    juce::StringArray favourites;
    juce::String currentKey { "F:Init" };

    juce::File favouritesFile() const { return getUserDir().getChildFile ("favourites.txt"); }
    void migrateOldPresets();
    void loadFavourites();
    void saveFavourites() const;
    static juce::String legalName (const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
