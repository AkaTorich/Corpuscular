/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "arp_section.h"

#include "ArpGen.h"
#include "ArpTurnaround.h"
#include "ArpsyGen.h"
#include "BassGen.h"
#include "Generators.h"
#include "MusicData.h"
#include "fonts.h"
#include "full_interface.h"
#include "load_save.h"
#include "skin.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

namespace {
  const char* kArpModeNames[] = { "PSY ARP", "ARP", "ARP STYLED", "BASS", "PAD" };

  // Per mode: titles of the four mode settings and their defaults (from Melody Atoms)
  const char* kArpParamTitles[ArpSection::kNumModes][4] = {
    { "COLOR NOTES", "BARS", "OCTAVES", "STRUCTURE" },
    { "NOTES", "BARS", "REPEATS", "ENDING" },
    { "OCTAVES", "BARS", "REPEATS", "GENRE" },
    { "NOTES", "BARS", "REPEATS", "PATTERN" },
    { "CHORDS", "LENGTH", "", "" },
  };
  const int kArpParamDefaults[ArpSection::kNumModes][4] = {
    { 5, 4, 2, 0 }, { 4, 4, 1, 0 }, { 2, 4, 1, 0 }, { 4, 4, 1, 0 }, { 4, 4, 0, 0 },
  };
  const int kArpDefaultKey = 21;   // Am

  // Share of the row each control takes; row A is the generator, row B the player and the roll tools
  const float kArpRowA[] = { 0.055f, 0.065f, 0.10f, 0.065f, 0.12f, 0.15f, 0.07f, 0.065f, 0.07f, 0.13f, 0.11f };
  const float kArpRowB[] = { 0.08f, 0.08f, 0.08f, 0.075f, 0.055f, 0.055f, 0.06f, 0.06f, 0.045f, 0.06f, 0.06f, 0.06f,
                             0.055f, 0.065f, 0.11f };
  // Clicking the middle of a list control opens the list, its sides step through it
  constexpr float kArpListSide = 0.22f;
  constexpr int kArpRowACount = 11;

  mg::MusicData& arpMusicData() {
    static mg::MusicData data;
    return data;
  }

  int arpWrap(int value, int size) {
    return size <= 0 ? 0 : ((value % size) + size) % size;
  }
}

ArpSection::ArpSection() : SynthSection("ARP"), loaded_(false), mode_(kPsyArp), key_(kArpDefaultKey),
                           scale_group_(0), scale_(1), midi_pressed_(false), midi_drop_hover_(false) {
  for (int i = 0; i < 4; ++i)
    params_[i] = kArpParamDefaults[kPsyArp][i];

  // Natural minor to start with, wherever the library sorts it
  const Array<mg::Category>& categories = arpMusicData().categories();
  for (int c = 0; c < categories.size(); ++c) {
    const Array<mg::Scale>& scales = categories.getReference(c).scales;
    for (int s = 0; s < scales.size(); ++s) {
      if (scales.getReference(s).name.startsWithIgnoreCase("Natural minor")) {
        scale_group_ = c;
        scale_ = s;
      }
    }
  }

  piano_roll_.setListener(this);
  addAndMakeVisible(piano_roll_);
  addOpenGlComponent(piano_roll_.getImageComponent());
  setInterceptsMouseClicks(true, true);
}

ArpSequencer* ArpSection::sequencer() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || parent->getSynth() == nullptr)
    return nullptr;
  return &parent->getSynth()->getArpSequencer();
}

// Settings ----------------------------------------------------------------------------------------------------

void ArpSection::loadFromSequencer() {
  ArpSequencer* seq = sequencer();
  if (seq == nullptr)
    return;

  json ui = seq->getUiState();
  const Array<mg::Category>& categories = arpMusicData().categories();
  if (ui.is_object() && ui.count("mode")) {
    mode_ = jlimit(0, kNumModes - 1, ui["mode"].get<int>());
    key_ = jlimit(0, mg::MusicData::tonalities().size() - 1, ui.count("key") ? ui["key"].get<int>() : kArpDefaultKey);
    scale_group_ = jlimit(0, std::max(0, categories.size() - 1), ui.count("group") ? ui["group"].get<int>() : 0);
    int num_scales = categories.isEmpty() ? 1 : categories.getReference(scale_group_).scales.size();
    scale_ = jlimit(0, std::max(0, num_scales - 1), ui.count("scale") ? ui["scale"].get<int>() : 0);
    if (ui.count("params") && ui["params"].is_array()) {
      for (int i = 0; i < 4 && i < (int)ui["params"].size(); ++i)
        params_[i] = jlimit(paramMin(i), std::max(paramMin(i), paramMax(i)), ui["params"][i].get<int>());
    }
    info_ = ui.count("info") ? String(ui["info"].get<std::string>()) : String();
  }

  bool first_load = !loaded_;
  loaded_ = true;
  piano_roll_.setPattern(seq->getNotes(), seq->getLength(), false);
  if (first_load)
    piano_roll_.fitToPattern();
  updateScaleHighlight();
  repaintBackground();
}

void ArpSection::saveUiState() {
  ArpSequencer* seq = sequencer();
  if (seq == nullptr)
    return;
  json ui;
  ui["mode"] = mode_;
  ui["key"] = key_;
  ui["group"] = scale_group_;
  ui["scale"] = scale_;
  ui["params"] = { params_[0], params_[1], params_[2], params_[3] };
  ui["info"] = info_.toStdString();
  seq->setUiState(ui);
}

void ArpSection::pushPattern() {
  ArpSequencer* seq = sequencer();
  if (seq)
    seq->setPattern(piano_roll_.getNotes(), piano_roll_.getLength(), seq->getRoot());
}

void ArpSection::patternEdited() {
  pushPattern();
  repaintBackground();
}

void ArpSection::updateScaleHighlight() {
  const Array<mg::Category>& categories = arpMusicData().categories();
  Array<int> steps;
  if (scale_group_ < categories.size() && scale_ < categories.getReference(scale_group_).scales.size())
    steps = categories.getReference(scale_group_).scales.getReference(scale_).steps;
  piano_roll_.setScale(key_ % 12, steps);
}

void ArpSection::setModeDefaults() {
  for (int i = 0; i < 4; ++i)
    params_[i] = kArpParamDefaults[mode_][i];
}

StringArray ArpSection::paramNames(int param) const {
  if (param != 3)
    return {};
  if (mode_ == kPsyArp)
    return mg::arpsyStructureNames();
  if (mode_ == kArpRiff)
    return mg::arpEndingNames();
  if (mode_ == kArpStyled)
    return mg::arpGenreNames();
  if (mode_ == kBass)
    return mg::bassPatternNames();
  return {};
}

int ArpSection::paramMin(int param) const {
  if (String(kArpParamTitles[mode_][param]).isEmpty())
    return 0;
  if (param == 3)
    return 0;
  if (mode_ == kPsyArp && param == 0)
    return 0;
  if (param == 2 && mode_ != kPsyArp)
    return 0;
  return 1;
}

int ArpSection::paramMax(int param) const {
  if (String(kArpParamTitles[mode_][param]).isEmpty())
    return 0;
  if (param == 3)
    return std::max(0, paramNames(3).size() - 1);
  if (mode_ == kPsyArp)
    return param == 0 ? 12 : param == 1 ? 16 : 3;
  if (mode_ == kArpStyled && param == 0)
    return 3;
  if (mode_ == kPad)
    return param == 0 ? 8 : 16;
  if (param == 2)
    return 15;
  return 16;
}

// Generation ---------------------------------------------------------------------------------------------------

void ArpSection::generate() {
  const Array<mg::Category>& categories = arpMusicData().categories();
  if (scale_group_ >= categories.size() || scale_ >= categories.getReference(scale_group_).scales.size())
    return;

  const mg::Scale& scale = categories.getReference(scale_group_).scales.getReference(scale_);
  String tonality = mg::MusicData::tonalities()[key_];
  Random random(Random::getSystemRandom().nextInt64());
  StringArray info;
  mg::Clip clip;
  int bars = std::max(1, params_[1]);
  int repeats = jlimit(0, bars - 1, params_[2]);

  if (mode_ == kPsyArp)
    clip = mg::generateArpsy(tonality, scale.steps, params_[0], bars, jlimit(1, 3, params_[2]), params_[3], random, info);
  else if (mode_ == kArpRiff)
    clip = mg::generateArpRiff(tonality, scale.steps, params_[0], 2, bars, repeats, random, info, params_[3]);
  else if (mode_ == kArpStyled) {
    String genre = mg::arpGenreNames()[params_[3]];
    clip = mg::generateArpStyled(tonality, scale.name, scale.steps, genre, jlimit(1, 3, params_[0]), bars, repeats,
                                 random, info);
  }
  else if (mode_ == kBass) {
    String pattern = mg::bassPatternNames()[params_[3]];
    clip = mg::generateBass(tonality, scale.name, scale.steps, pattern, params_[0], bars - repeats, repeats,
                            random, info);
  }
  else {
    mg::Scale pad_scale = scale;
    info = mg::generateProgression(tonality, pad_scale, params_[0], params_[1], random);
    clip = mg::renderClip(mg::Mode::Pad, info);
  }

  std::vector<ArpSequencer::Note> notes;
  int end = 0;
  for (const mg::NoteEvent& event : clip) {
    notes.push_back({ event.midi, event.startSixteenth, std::max(1, event.lengthSixteenth), 100 });
    end = std::max(end, event.startSixteenth + event.lengthSixteenth);
  }
  if (notes.empty())
    return;

  int length = std::max(16, ((end + 15) / 16) * 16);
  int root = 60 + key_ % 12;
  info_ = info.joinIntoString("  ");
  midi_status_.clear();

  piano_roll_.setPattern(notes, length, true);
  piano_roll_.fitToPattern();
  ArpSequencer* seq = sequencer();
  if (seq)
    seq->setPattern(notes, length, root);
  saveUiState();
  repaintBackground();
}

// Controls -------------------------------------------------------------------------------------------------------

Rectangle<float> ArpSection::contentArea() const {
  float padding = 10.0f * size_ratio_;
  float left = findValue(Skin::kTitleWidth) + padding;
  return Rectangle<float>(left, padding, getWidth() - left - padding, getHeight() - 2.0f * padding);
}

Rectangle<float> ArpSection::controlBounds(int control) const {
  Rectangle<float> content = contentArea();
  bool row_a = control < kArpRowACount;
  float top = content.getY() + content.getHeight() * (row_a ? 0.0f : 0.125f);
  float height = content.getHeight() * (row_a ? 0.115f : 0.10f);
  const float* weights = row_a ? kArpRowA : kArpRowB;
  int index = row_a ? control : control - kArpRowACount;
  float x = content.getX();
  for (int i = 0; i < index; ++i)
    x += weights[i] * content.getWidth();
  return Rectangle<float>(x, top, weights[index] * content.getWidth(), height).reduced(3.0f * size_ratio_);
}

Rectangle<float> ArpSection::rollBounds() const {
  Rectangle<float> content = contentArea();
  return Rectangle<float>(content.getX(), content.getY() + content.getHeight() * 0.235f,
                          content.getWidth(), content.getHeight() * 0.715f);
}

Rectangle<float> ArpSection::infoBounds() const {
  Rectangle<float> content = contentArea();
  return content.withTop(content.getY() + content.getHeight() * 0.955f);
}

bool ArpSection::isButton(int control) const {
  return control == kOn || control == kLatch || control == kGenerate ||
         (control >= kZoomTimeOut && control != kSavedArp);
}

bool ArpSection::isLit(int control) const {
  ArpSequencer* seq = const_cast<ArpSection*>(this)->sequencer();
  if (control == kOn)
    return seq && seq->isEnabled();
  if (control == kLatch)
    return seq && seq->isLatched();
  if (control == kGenerate)
    return true;
  if (control == kUndo)
    return piano_roll_.canUndo();
  if (control == kRedo)
    return piano_roll_.canRedo();
  return false;
}

String ArpSection::controlTitle(int control) const {
  switch (control) {
    case kMode: return "MODE";
    case kKey: return "KEY";
    case kScaleGroup: return "SCALE GROUP";
    case kScale: return "SCALE";
    case kParam1: case kParam2: case kParam3: case kParam4: return kArpParamTitles[mode_][control - kParam1];
    case kRate: return "RATE";
    case kOctave: return "OCTAVE";
    case kGate: return "GATE";
    case kBars: return "LENGTH";
    case kSavedArp: return "SAVED ARP";
    default: return {};
  }
}

String ArpSection::controlValue(int control) const {
  const Array<mg::Category>& categories = arpMusicData().categories();
  ArpSequencer* seq = const_cast<ArpSection*>(this)->sequencer();
  switch (control) {
    case kOn: return "ON";
    case kLatch: return "LATCH";
    case kGenerate: return "GENERATE";
    case kZoomTimeOut: return "TIME -";
    case kZoomTimeIn: return "TIME +";
    case kZoomNotesOut: return "NOTES -";
    case kZoomNotesIn: return "NOTES +";
    case kFit: return "FIT";
    case kUndo: return "UNDO";
    case kRedo: return "REDO";
    case kClear: return "CLEAR";
    case kSave: return "SAVE";
    case kMidi: return "DRAG MIDI";
    case kSavedArp: return current_arp_file_ == File() ? String("-") : current_arp_file_.getFileNameWithoutExtension();
    case kMode: return kArpModeNames[mode_];
    case kKey: return mg::MusicData::tonalities()[key_];
    case kScaleGroup:
      return scale_group_ < categories.size() ? categories.getReference(scale_group_).name : String("-");
    case kScale:
      if (scale_group_ < categories.size() && scale_ < categories.getReference(scale_group_).scales.size())
        return categories.getReference(scale_group_).scales.getReference(scale_).name;
      return "-";
    case kParam1: case kParam2: case kParam3: case kParam4: {
      int param = control - kParam1;
      if (controlTitle(control).isEmpty())
        return {};
      StringArray names = paramNames(param);
      if (!names.isEmpty())
        return names[jlimit(0, names.size() - 1, params_[param])];
      return String(params_[param]);
    }
    case kRate: return seq ? ArpSequencer::rateName(seq->getRate()) : String();
    case kOctave: {
      int octave = seq ? seq->getOctave() : 0;
      return octave > 0 ? "+" + String(octave) : String(octave);
    }
    case kGate: return seq ? String(roundToInt(seq->getGate() * 100.0f)) + "%" : String();
    case kBars: {
      int length = piano_roll_.getLength();
      return length % 16 == 0 ? String(length / 16) + (length == 16 ? " bar" : " bars") : String(length) + " steps";
    }
    default: return {};
  }
}

void ArpSection::stepControl(int control, int delta) {
  const Array<mg::Category>& categories = arpMusicData().categories();
  ArpSequencer* seq = sequencer();
  switch (control) {
    case kMode:
      mode_ = arpWrap(mode_ + delta, kNumModes);
      setModeDefaults();
      break;
    case kKey:
      key_ = arpWrap(key_ + delta, mg::MusicData::tonalities().size());
      updateScaleHighlight();
      break;
    case kScaleGroup:
      scale_group_ = arpWrap(scale_group_ + delta, categories.size());
      scale_ = 0;
      updateScaleHighlight();
      break;
    case kScale:
      if (scale_group_ < categories.size())
        scale_ = arpWrap(scale_ + delta, categories.getReference(scale_group_).scales.size());
      updateScaleHighlight();
      break;
    case kParam1: case kParam2: case kParam3: case kParam4: {
      int param = control - kParam1;
      int low = paramMin(param), high = paramMax(param);
      if (high >= low)
        params_[param] = low + arpWrap(params_[param] - low + delta, high - low + 1);
      break;
    }
    case kRate:
      if (seq)
        seq->setRate(jlimit(0, ArpSequencer::kNumRates - 1, seq->getRate() + delta));
      break;
    case kOctave:
      if (seq)
        seq->setOctave(seq->getOctave() + delta);
      break;
    case kGate:
      if (seq)
        seq->setGate(seq->getGate() + delta * 0.05f);
      break;
    case kBars: {
      int length = piano_roll_.getLength();
      int bars = length % 16 == 0 ? length / 16 + delta : (delta > 0 ? length / 16 + 1 : length / 16);
      piano_roll_.setLength(jlimit(1, ArpSequencer::kMaxSteps / 16, bars) * 16);
      break;
    }
    default:
      break;
  }
  saveUiState();
}

void ArpSection::clickButton(int control) {
  ArpSequencer* seq = sequencer();
  switch (control) {
    case kOn:
      if (seq)
        seq->setEnabled(!seq->isEnabled());
      break;
    case kLatch:
      if (seq)
        seq->setLatch(!seq->isLatched());
      break;
    case kGenerate:
      generate();
      return;
    case kZoomTimeOut: piano_roll_.zoomTime(1.5f); break;
    case kZoomTimeIn: piano_roll_.zoomTime(1.0f / 1.5f); break;
    case kZoomNotesOut: piano_roll_.zoomPitch(1.3f); break;
    case kZoomNotesIn: piano_roll_.zoomPitch(1.0f / 1.3f); break;
    case kFit: piano_roll_.fitToPattern(); break;
    case kUndo: piano_roll_.undo(); break;
    case kRedo: piano_roll_.redo(); break;
    case kClear: piano_roll_.clearNotes(); break;
    case kSave: saveArp(); return;
    default: break;
  }
}

void ArpSection::mouseDown(const MouseEvent& e) {
  for (int control = 0; control < kNumControls; ++control) {
    Rectangle<float> bounds = controlBounds(control);
    if (!bounds.contains(e.position))
      continue;

    float position = (e.position.x - bounds.getX()) / bounds.getWidth();
    int side = position < kArpListSide ? -1 : position > 1.0f - kArpListSide ? 1 : 0;
    // DRAG MIDI decides on mouse up (click) or on the first drag (drag to the DAW)
    if (control == kMidi) {
      midi_pressed_ = true;
      return;
    }
    if (isButton(control))
      clickButton(control);
    else if (control == kSavedArp) {
      if (side == 0)
        browseArps();
      else
        stepSavedArp(side);
    }
    else if (hasList(control) && side == 0) {
      showList(control);
      return;
    }
    else if (!controlTitle(control).isEmpty())
      stepControl(control, e.position.x < bounds.getCentreX() ? -1 : 1);
    repaintBackground();
    return;
  }
}

void ArpSection::mouseDrag(const MouseEvent& e) {
  static constexpr int kDragStartDistance = 6;
  if (midi_pressed_ && e.getDistanceFromDragStart() > kDragStartDistance * size_ratio_) {
    midi_pressed_ = false;
    dragMidiClip();
  }
}

void ArpSection::mouseUp(const MouseEvent& e) {
  if (!midi_pressed_)
    return;

  midi_pressed_ = false;
  File file = writeMidiClip();
  midi_status_ = file == File() ? String("Can't write the MIDI file") : "MIDI saved: " + file.getFullPathName();
  repaintBackground();
}

// MIDI clips -----------------------------------------------------------------------------------------------------

File ArpSection::writeMidiClip() {
  ArpSequencer* seq = sequencer();
  if (seq == nullptr)
    return File();

  File folder = arpFolder().getChildFile("MIDI");
  if (!folder.isDirectory() && !folder.createDirectory())
    return File();

  // Named after the saved arp, otherwise after the key and the mode; a new file each time,
  // so a clip already in the DAW never changes under it
  String name = current_arp_file_ != File() ? current_arp_file_.getFileNameWithoutExtension()
                                            : String("Arp ") + controlValue(kKey) + " " + kArpModeNames[mode_];
  name = File::createLegalFileName(name);
  File file = folder.getNonexistentChildFile(name, ".mid", false);
  return seq->writeMidiFile(file, name) ? file : File();
}

void ArpSection::dragMidiClip() {
  File file = writeMidiClip();
  if (file == File()) {
    midi_status_ = "Can't write the MIDI file";
    repaintBackground();
    return;
  }

  midi_status_ = "MIDI clip: " + file.getFileName();
  repaintBackground();
  DragAndDropContainer::performExternalDragDropOfFiles(StringArray(file.getFullPathName()), false, this);
}

namespace {
  bool arpIsMidiFile(const String& path) {
    return path.endsWithIgnoreCase(".mid") || path.endsWithIgnoreCase(".midi");
  }
}

bool ArpSection::isInterestedInFileDrag(const StringArray& files) {
  for (const String& file : files) {
    if (arpIsMidiFile(file))
      return true;
  }
  return false;
}

void ArpSection::fileDragEnter(const StringArray& files, int x, int y) {
  midi_drop_hover_ = true;
  repaintBackground();
}

void ArpSection::fileDragExit(const StringArray& files) {
  midi_drop_hover_ = false;
  repaintBackground();
}

void ArpSection::filesDropped(const StringArray& files, int x, int y) {
  midi_drop_hover_ = false;
  for (const String& path : files) {
    if (arpIsMidiFile(path)) {
      importMidiClip(File(path));
      break;
    }
  }
  repaintBackground();
}

void ArpSection::importMidiClip(const File& file) {
  ArpSequencer* seq = sequencer();
  std::vector<ArpSequencer::Note> notes;
  int length = 0;
  if (seq == nullptr || !ArpSequencer::readMidiFile(file, seq->getRate(), notes, length)) {
    midi_status_ = "No notes in " + file.getFileName();
    return;
  }

  // Like a generated melody: the notes in the current KEY, UNDO brings the old pattern back
  piano_roll_.setPattern(notes, length, true);
  piano_roll_.fitToPattern();
  seq->setPattern(notes, length, 60 + key_ % 12);
  current_arp_file_ = File();
  info_ = "From MIDI: " + file.getFileNameWithoutExtension();
  midi_status_ = "MIDI loaded: " + file.getFileName() + ", " + String((int)notes.size()) + " notes";
  saveUiState();
}

// Lists ----------------------------------------------------------------------------------------------------------

bool ArpSection::hasList(int control) const {
  return control == kMode || control == kKey || control == kScaleGroup || control == kScale ||
         (control == kParam4 && !paramNames(3).isEmpty());
}

PopupItems ArpSection::listItems(int control) const {
  PopupItems items;
  const Array<mg::Category>& categories = arpMusicData().categories();
  if (control == kMode) {
    for (int i = 0; i < kNumModes; ++i)
      items.addItem(i, kArpModeNames[i], i == mode_);
  }
  else if (control == kKey) {
    const StringArray& keys = mg::MusicData::tonalities();
    for (int i = 0; i < keys.size(); ++i)
      items.addItem(i, keys[i].toStdString(), i == key_);
  }
  else if (control == kScaleGroup) {
    for (int i = 0; i < categories.size(); ++i)
      items.addItem(i, categories.getReference(i).name.toStdString(), i == scale_group_);
  }
  else if (control == kScale && scale_group_ < categories.size()) {
    const Array<mg::Scale>& scales = categories.getReference(scale_group_).scales;
    for (int i = 0; i < scales.size(); ++i)
      items.addItem(i, scales.getReference(i).name.toStdString(), i == scale_);
  }
  else if (control == kParam4) {
    StringArray names = paramNames(3);
    for (int i = 0; i < names.size(); ++i)
      items.addItem(i, names[i].toStdString(), i == params_[3]);
  }
  return items;
}

void ArpSection::selectFromList(int control, int index) {
  if (index < 0)
    return;
  if (control == kMode) {
    mode_ = jlimit(0, kNumModes - 1, index);
    setModeDefaults();
  }
  else if (control == kKey)
    key_ = jlimit(0, mg::MusicData::tonalities().size() - 1, index);
  else if (control == kScaleGroup) {
    scale_group_ = jlimit(0, std::max(0, arpMusicData().categories().size() - 1), index);
    scale_ = 0;
  }
  else if (control == kScale)
    scale_ = index;
  else if (control == kParam4)
    params_[3] = jlimit(paramMin(3), paramMax(3), index);

  updateScaleHighlight();
  saveUiState();
  repaintBackground();
}

void ArpSection::showList(int control) {
  Rectangle<float> bounds = controlBounds(control);
  Point<int> position(roundToInt(bounds.getX()), roundToInt(bounds.getBottom()));
  showPopupSelector(this, position, listItems(control), [this, control](int index) { selectFromList(control, index); });
}

// Saved arps -----------------------------------------------------------------------------------------------------

File ArpSection::arpFolder() const {
  return LoadSave::getDataDirectory().getChildFile(ArpSequencer::kFolderName);
}

void ArpSection::saveArp() {
  ArpSequencer* seq = sequencer();
  FullInterface* full_interface = findParentComponentOfClass<FullInterface>();
  if (seq == nullptr || full_interface == nullptr)
    return;
  File folder = arpFolder();
  if (!folder.isDirectory())
    folder.createDirectory();
  saveUiState();
  full_interface->saveArp(seq->toJson(), folder);
}

void ArpSection::save(File file) {
  // Every save dialog reports here; only arps concern this tab
  if (file.hasFileExtension(ArpSequencer::kExtension)) {
    current_arp_file_ = file;
    repaintBackground();
  }
}

void ArpSection::loadFile(const File& file) {
  ArpSequencer* seq = sequencer();
  if (seq == nullptr || !file.existsAsFile())
    return;

  json data = json::parse(file.loadFileAsString().toStdString(), nullptr, false);
  if (!data.is_object())
    return;

  // A saved arp brings its pattern and settings but leaves ON as it is
  bool enabled = seq->isEnabled();
  seq->fromJson(data);
  seq->setEnabled(enabled);
  current_arp_file_ = file;
  piano_roll_.setPattern(seq->getNotes(), seq->getLength(), true);
  piano_roll_.fitToPattern();
  loadFromSequencer();
  updatePopupBrowser(this);
}

void ArpSection::stepSavedArp(int delta) {
  File file = LoadSave::getShiftedFile(ArpSequencer::kFolderName, String("*.") + ArpSequencer::kExtension, "",
                                       current_arp_file_, delta);
  if (file.existsAsFile())
    loadFile(file);
}

void ArpSection::browseArps() {
  File folder = arpFolder();
  if (!folder.isDirectory())
    folder.createDirectory();

  Rectangle<float> anchor = controlBounds(kSavedArp);
  int width = roundToInt(450 * size_ratio_);
  int height = roundToInt(300 * size_ratio_);
  Rectangle<int> bounds(roundToInt(anchor.getRight()) - width, roundToInt(anchor.getBottom()), width, height);
  showPopupBrowser(this, bounds, { folder }, String("*.") + ArpSequencer::kExtension, ArpSequencer::kFolderName, "");
}

// Component ------------------------------------------------------------------------------------------------------

void ArpSection::resized() {
  SynthSection::resized();
  piano_roll_.setBounds(rollBounds().toNearestInt());
}

void ArpSection::reset() {
  SynthSection::reset();
  // A preset was loaded: it may have brought its own pattern
  if (loaded_)
    loadFromSequencer();
}

void ArpSection::visibilityChanged() {
  SynthSection::visibilityChanged();
  if (isShowing()) {
    loadFromSequencer();
    startTimerHz(30);
  }
  else
    stopTimer();
}

void ArpSection::timerCallback() {
  ArpSequencer* seq = sequencer();
  if (seq)
    piano_roll_.setPlayhead(seq->getPlayhead());
}

void ArpSection::paintBackground(Graphics& g) {
  paintContainer(g);
  paintHeadingText(g);

  float rounding = findValue(Skin::kLabelBackgroundRounding);
  Colour text = findColour(Skin::kBodyText, true);
  Colour cell = findColour(Skin::kTextComponentBackground, true);
  Colour value_text = findColour(Skin::kTextComponentText, true);
  Colour active = findColour(Skin::kRotaryArc, true);
  float font_height = std::max(9.0f, contentArea().getHeight() * 0.022f);

  for (int control = 0; control < kNumControls; ++control) {
    Rectangle<float> bounds = controlBounds(control);
    String title = controlTitle(control);
    String value = controlValue(control);
    if (!isButton(control) && title.isEmpty())
      continue;

    // Every control keeps a title line so the rows line up
    Rectangle<float> title_bounds = bounds.removeFromTop(bounds.getHeight() * 0.36f);
    g.setColour(text);
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height * 0.8f));
    g.drawFittedText(title, title_bounds.toNearestInt(), Justification::centred, 1, 0.7f);

    if (isButton(control)) {
      bool lit = isLit(control);
      g.setColour(lit ? active : cell);
      g.fillRoundedRectangle(bounds, rounding);
      g.setColour(lit ? text : value_text);
      g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height));
      g.drawFittedText(value, bounds.toNearestInt(), Justification::centred, 1, 0.6f);
      continue;
    }

    g.setColour(cell);
    g.fillRoundedRectangle(bounds, rounding);
    g.setColour(value_text);
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height));
    Rectangle<float> arrows = bounds.reduced(bounds.getHeight() * 0.25f, 0.0f);
    g.drawText("<", arrows, Justification::centredLeft);
    g.drawText(">", arrows, Justification::centredRight);
    g.drawFittedText(value, arrows.reduced(font_height * 0.9f, 0.0f).toNearestInt(), Justification::centred, 1, 0.55f);
  }

  g.setColour(text.withMultipliedAlpha(0.75f));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height * 0.85f));
  g.drawFittedText(midi_status_.isEmpty() ? info_ : midi_status_, infoBounds().toNearestInt(), Justification::centredLeft, 1, 0.6f);

  // A MIDI clip is being dragged over the tab: a frame around the notes it will replace
  if (midi_drop_hover_) {
    float thickness = std::max(2.0f, 2.0f * size_ratio_);
    g.setColour(findColour(Skin::kRotaryArc, true));
    g.drawRoundedRectangle(rollBounds().expanded(thickness * 1.5f), findValue(Skin::kLabelBackgroundRounding), thickness);
  }
}
