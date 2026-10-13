#include "PresetManager.h"
#include "PluginProcessor.h"

PresetManager::PresetManager (PolysixAudioProcessor& p) : proc (p)
{
    migrateOldPresets();
    getUserDir().createDirectory();
    loadFavourites();
    refresh();
}

juce::File PresetManager::getUserDir() const
{
   #if JUCE_MAC
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
             .getChildFile ("Library/Audio/Presets/Ticco Ross/Polytwelve");
   #elif JUCE_WINDOWS
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)   // Documents\Ticco Ross\Polytwelve\Presets
             .getChildFile ("Ticco Ross/Polytwelve/Presets");
   #else
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)        // ~/.local/share/Ticco Ross/Polytwelve/Presets
             .getChildFile (".local/share/Ticco Ross/Polytwelve/Presets");
   #endif
}

// First run: bring over presets + favourites saved with the earlier "Polysix Port" build
void PresetManager::migrateOldPresets()
{
   #if JUCE_MAC
    const auto oldDir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                          .getChildFile ("Library/Audio/Presets/DIY/Polysix Port");
   #else
    const auto oldDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile ("DIY/Polysix Port/Presets");
   #endif
    const auto newDir = getUserDir();
    if (newDir.isDirectory() || ! oldDir.isDirectory()) return;
    newDir.createDirectory();
    for (const auto& f : oldDir.findChildFiles (juce::File::findFiles, false, "*"))
        f.copyFileTo (newDir.getChildFile (f.getFileName()));
}

juce::String PresetManager::legalName (const juce::String& name)
{
    return juce::File::createLegalFileName (name.trim()).substring (0, 64);
}

void PresetManager::refresh()
{
    entries.clear();
    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        Entry e; e.factory = true; e.factoryIndex = i; e.name = proc.getProgramName (i); e.key = "F:" + e.name;
        entries.push_back (e);
    }

    auto files = getUserDir().findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension);
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0; });
    for (auto& f : files)
    {
        Entry e; e.file = f; e.name = f.getFileNameWithoutExtension(); e.key = "U:" + e.name;
        entries.push_back (e);
    }
    sendChangeMessage();
}

int PresetManager::indexOfKey (const juce::String& key) const
{
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].key == key) return (int) i;
    return -1;
}

juce::String PresetManager::getCurrentName() const
{
    return currentKey.fromFirstOccurrenceOf (":", false, false);
}

void PresetManager::setCurrentKey (const juce::String& key)
{
    currentKey = key.isNotEmpty() ? key : juce::String ("F:Init");
    sendChangeMessage();
}

bool PresetManager::load (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) entries.size())) return false;
    const auto e = entries[(size_t) index];
    if (e.factory)
    {
        proc.setCurrentProgram (e.factoryIndex); // updates currentKey through the processor
        currentKey = e.key;
        sendChangeMessage();
        return true;
    }
    if (! loadFile (e.file)) return false;
    currentKey = e.key;
    sendChangeMessage();
    return true;
}

bool PresetManager::loadFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("PolysixPreset")) return false;
    auto* stateXml = xml->getChildByName (proc.apvts.state.getType());
    if (stateXml == nullptr) return false;
    proc.restoreState (juce::ValueTree::fromXml (*stateXml));

    // a file opened from elsewhere is shown by its name (save it to keep it in the list)
    currentKey = "U:" + file.getFileNameWithoutExtension();
    sendChangeMessage();
    return true;
}

bool PresetManager::userPresetExists (const juce::String& name) const
{
    return getUserDir().getChildFile (legalName (name) + extension).existsAsFile();
}

bool PresetManager::save (const juce::String& name)
{
    const auto clean = legalName (name);
    if (clean.isEmpty()) return false;
    getUserDir().createDirectory();

    juce::XmlElement root ("PolysixPreset");
    root.setAttribute ("name", clean);
    root.setAttribute ("version", 1);
    if (auto state = proc.captureState().createXml()) root.addChildElement (state.release());

    const auto file = getUserDir().getChildFile (clean + extension);
    if (! root.writeTo (file)) return false;
    currentKey = "U:" + clean;
    refresh();
    return true;
}

bool PresetManager::deleteCurrent()
{
    const int idx = indexOfKey (currentKey);
    if (idx < 0 || entries[(size_t) idx].factory) return false;
    if (! entries[(size_t) idx].file.deleteFile()) return false;
    if (favourites.contains (currentKey)) { favourites.removeString (currentKey); saveFavourites(); }
    currentKey = "F:Init";
    refresh();
    return true;
}

void PresetManager::toggleFavourite (const juce::String& key)
{
    if (favourites.contains (key)) favourites.removeString (key);
    else favourites.add (key);
    saveFavourites();
    sendChangeMessage();
}

void PresetManager::step (int delta, bool favouritesOnly)
{
    std::vector<int> pool;
    for (size_t i = 0; i < entries.size(); ++i)
        if (! favouritesOnly || isFavourite (entries[i].key)) pool.push_back ((int) i);
    if (pool.empty()) return;

    const int cur = indexOfKey (currentKey);
    int pos = -1;
    for (size_t i = 0; i < pool.size(); ++i) if (pool[i] == cur) pos = (int) i;

    const int n = (int) pool.size();
    const int next = pos < 0 ? (delta > 0 ? 0 : n - 1) : ((pos + delta) % n + n) % n;
    load (pool[(size_t) next]);
}

void PresetManager::loadFavourites()
{
    favourites.clear();
    if (favouritesFile().existsAsFile())
    {
        favourites.addLines (favouritesFile().loadFileAsString());
        favourites.removeEmptyStrings();
        favourites.removeDuplicates (false);
    }
}

void PresetManager::saveFavourites() const
{
    getUserDir().createDirectory();
    favouritesFile().replaceWithText (favourites.joinIntoString ("\n"));
}
