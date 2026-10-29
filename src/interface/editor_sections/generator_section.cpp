/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 * Modified for Corpuscular, 2026.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "generator_section.h"

#include "corpuscular_library.h"
#include "fonts.h"
#include "load_save.h"
#include "preset_generator.h"
#include "preset_mutator.h"
#include "skin.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include "wavetable_generator.h"

namespace {
  constexpr int kGenColumns = 4;
  constexpr int kGenMaxCount = 999;
  const char* kGenAuthor = "Corpuscular";
  const char* kGenOptionTitles[] = { "SCALE GROUP", "SCALE", "MELODY", "STEP", "VOCODER" };
  const char* kGenVocoderValues[] = { "OFF", "ON", "SOME" };
  const char* kGenStepValues[] = { "auto", "1/16", "1/8" };
  constexpr int kGenNumSteps = 3;
  const char* kGenTableTitles[] = { "TABLE TYPE", "FRAMES", "FRAME SIZE", "SMOOTH", "EXPORT AS" };
  const int kGenTableFrames[] = { 8, 16, 32, 64, 128, 256 };
  constexpr int kGenNumTableFrames = 6;
  const int kGenTableSizes[] = { 256, 512, 1024, 2048, 4096 };
  constexpr int kGenNumTableSizes = 5;
  constexpr int kGenHistorySize = 20;

  String genSafeName(const String& name, const String& fallback) {
    String result = File::createLegalFileName(name.trim()).trim();
    return result.isEmpty() ? fallback : result;
  }

  uint32_t genSeed() {
    return static_cast<uint32_t>(Random::getSystemRandom().nextInt());
  }

  int genWrap(int value, int size) {
    return size <= 0 ? 0 : ((value % size) + size) % size;
  }

  // Makes the bank presets on a worker thread while a progress window is shown
  class GenBankBuilder : public ThreadWithProgressWindow {
    public:
      GenBankBuilder(std::vector<corpuscular::gen::PresetRequest> requests) :
          ThreadWithProgressWindow("Corpuscular: making the bank", true, true), requests_(std::move(requests)) { }

      void run() override {
        for (size_t i = 0; i < requests_.size(); ++i) {
          if (threadShouldExit())
            return;
          setProgress(i / (double)requests_.size());
          presets_.push_back(corpuscular::gen::createPreset(requests_[i]));
        }
        setProgress(1.0);
      }

      std::vector<corpuscular::gen::GeneratedPreset>& presets() { return presets_; }

    private:
      std::vector<corpuscular::gen::PresetRequest> requests_;
      std::vector<corpuscular::gen::GeneratedPreset> presets_;
  };
}

GeneratorSection::GeneratorSection() : SynthSection("GEN"), selected_type_(0), random_count_(0),
                                       scale_group_(-1), scale_(0), melody_(0), step_(0), vocoder_(kVocoderOff),
                                       table_type_(0), table_frames_(3), table_size_(3),
                                       table_smooth_(false), table_format_(0), history_index_(-1) {
  types_ = corpuscular::gen::presetTypes();
  bank_counts_.assign(types_.size(), 0);
  newBankName();
  loadHistory();
  setInterceptsMouseClicks(true, false);
}

void GeneratorSection::resized() {
  SynthSection::resized();
}

Rectangle<float> GeneratorSection::contentArea() const {
  float padding = 12.0f * size_ratio_;
  float left = findValue(Skin::kTitleWidth) + padding;
  return Rectangle<float>(left, padding, getWidth() - left - padding, getHeight() - 2.0f * padding);
}

Rectangle<float> GeneratorSection::band(float top, float bottom) const {
  Rectangle<float> content = contentArea();
  return Rectangle<float>(content.getX(), content.getY() + content.getHeight() * top,
                          content.getWidth(), content.getHeight() * (bottom - top));
}

int GeneratorSection::numTypeRows() const {
  return std::max<int>(1, ((int)types_.size() + kGenColumns - 1) / kGenColumns);
}

Rectangle<float> GeneratorSection::typeCell(int index) const {
  Rectangle<float> grid = band(0.0f, 0.28f);
  float row_height = grid.getHeight() / numTypeRows();
  float column_width = grid.getWidth() / kGenColumns;
  int row = index / kGenColumns;
  int column = index % kGenColumns;
  return Rectangle<float>(grid.getX() + column * column_width, grid.getY() + row * row_height,
                          column_width, row_height).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::optionBox(int option) const {
  Rectangle<float> row = band(0.29f, 0.40f);
  float width = row.getWidth() / kNumOptions;
  return row.withX(row.getX() + option * width).withWidth(width).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::generateButton() const {
  Rectangle<float> row = band(0.41f, 0.49f);
  return row.withWidth(row.getWidth() * 0.34f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::comboButton() const {
  Rectangle<float> row = band(0.41f, 0.49f);
  return row.withX(row.getX() + row.getWidth() * 0.34f).withWidth(row.getWidth() * 0.40f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::similarButton() const {
  Rectangle<float> row = band(0.41f, 0.49f);
  return row.withLeft(row.getX() + row.getWidth() * 0.74f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::historyBackButton() const {
  Rectangle<float> row = band(0.50f, 0.57f);
  return row.withWidth(row.getWidth() * 0.15f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::historyNameBox() const {
  Rectangle<float> row = band(0.50f, 0.57f);
  return row.withX(row.getX() + row.getWidth() * 0.15f).withWidth(row.getWidth() * 0.70f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::historyForwardButton() const {
  Rectangle<float> row = band(0.50f, 0.57f);
  return row.withLeft(row.getX() + row.getWidth() * 0.85f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::randomCountBox() const {
  Rectangle<float> row = band(0.58f, 0.69f);
  return row.withWidth(row.getWidth() * 0.25f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::bankNameBox() const {
  Rectangle<float> row = band(0.58f, 0.69f);
  return row.withX(row.getX() + row.getWidth() * 0.25f).withWidth(row.getWidth() * 0.45f).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::makeBankButton() const {
  Rectangle<float> row = band(0.58f, 0.69f);
  Rectangle<float> box = row.withX(row.getX() + row.getWidth() * 0.7f).withWidth(row.getWidth() * 0.3f);
  box.removeFromTop(box.getHeight() * 0.38f);
  return box.reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::tableOptionBox(int option) const {
  Rectangle<float> row = band(0.73f, 0.83f);
  float width = row.getWidth() / kNumTableOptions;
  return row.withX(row.getX() + option * width).withWidth(width).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::tableTargetButton(int index) const {
  Rectangle<float> row = band(0.84f, 0.91f);
  float width = row.getWidth() / (kNumWavetableTargets + 1);
  return row.withX(row.getX() + index * width).withWidth(width).reduced(3.0f * size_ratio_);
}

Rectangle<float> GeneratorSection::tableExportButton() const {
  return tableTargetButton(kNumWavetableTargets);
}

Rectangle<float> GeneratorSection::statusArea() const {
  return band(0.92f, 1.0f).reduced(3.0f * size_ratio_, 0.0f);
}

String GeneratorSection::optionValue(int option) const {
  if (option == kScaleGroup) {
    if (scale_group_ < 0)
      return "PSY MIX";
    return corpuscular::gen::scaleCategories()[scale_group_];
  }
  if (option == kScale) {
    if (scale_group_ < 0)
      return "-";
    std::vector<std::string> names = corpuscular::gen::scaleNames(corpuscular::gen::scaleCategories()[scale_group_]);
    if (names.empty())
      return "-";
    return names[std::min<int>(scale_, (int)names.size() - 1)];
  }
  if (option == kMelody)
    return corpuscular::gen::phraseKinds()[melody_];
  if (option == kVocoder)
    return kGenVocoderValues[vocoder_];
  return kGenStepValues[step_];
}

void GeneratorSection::stepOption(int option, int delta) {
  if (option == kScaleGroup) {
    int num_groups = (int)corpuscular::gen::scaleCategories().size();
    scale_group_ = genWrap(scale_group_ + 1 + delta, num_groups + 1) - 1;
    scale_ = 0;
  }
  else if (option == kScale && scale_group_ >= 0) {
    int num_scales = (int)corpuscular::gen::scaleNames(corpuscular::gen::scaleCategories()[scale_group_]).size();
    scale_ = genWrap(scale_ + delta, num_scales);
  }
  else if (option == kMelody)
    melody_ = genWrap(melody_ + delta, (int)corpuscular::gen::phraseKinds().size());
  else if (option == kStep)
    step_ = genWrap(step_ + delta, kGenNumSteps);
  else if (option == kVocoder)
    vocoder_ = genWrap(vocoder_ + delta, kNumVocoderChoices);
}

void GeneratorSection::maybeAddVocoder(nlohmann::json& preset, Random& random) const {
  if (vocoder_ == kVocoderOn || (vocoder_ == kVocoderSome && random.nextInt(4) == 0))
    corpuscular::addVocoder(preset, random);
}

String GeneratorSection::tableOptionValue(int option) const {
  if (option == kTableType)
    return corpuscular::gen::wavetableTypes()[table_type_];
  if (option == kTableFrames)
    return String(kGenTableFrames[table_frames_]);
  if (option == kTableSize)
    return String(kGenTableSizes[table_size_]);
  if (option == kTableSmooth)
    return table_smooth_ ? "ON" : "OFF";
  return corpuscular::gen::wavetableFormatNames()[table_format_];
}

void GeneratorSection::stepTableOption(int option, int delta) {
  if (option == kTableType)
    table_type_ = genWrap(table_type_ + delta, (int)corpuscular::gen::wavetableTypes().size());
  else if (option == kTableFrames)
    table_frames_ = genWrap(table_frames_ + delta, kGenNumTableFrames);
  else if (option == kTableSize)
    table_size_ = genWrap(table_size_ + delta, kGenNumTableSizes);
  else if (option == kTableSmooth)
    table_smooth_ = !table_smooth_;
  else if (option == kTableFormat)
    table_format_ = genWrap(table_format_ + delta, (int)corpuscular::gen::wavetableFormatNames().size());
}

void GeneratorSection::newBankName() {
  bank_name_ = corpuscular::gen::loremWords(genSeed(), 3);
}

void GeneratorSection::setStatus(const String& text) {
  status_ = text;
  repaintBackground();
}

bool GeneratorSection::loadPreset(const File& file) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return false;

  std::string error;
  if (!parent->getSynth()->loadFromFile(file, error)) {
    setStatus("Error: " + String(error));
    return false;
  }
  parent->externalPresetLoaded(file);
  return true;
}

corpuscular::gen::GeneratedWavetable GeneratorSection::makeWavetable() const {
  corpuscular::gen::WavetableRequest request;
  request.type = corpuscular::gen::wavetableTypes()[table_type_];
  request.frames = kGenTableFrames[table_frames_];
  request.frame_size = kGenTableSizes[table_size_];
  request.smooth = table_smooth_;
  request.seed = genSeed();
  return corpuscular::gen::createWavetable(request);
}

// Writes the table into the wavetable library (oscillator browsers list it); renames it to the file name
File GeneratorSection::saveWavetableToLibrary(corpuscular::gen::GeneratedWavetable& table) {
  File folder = LoadSave::getUserWavetableDirectory().getChildFile("Generated");
  if (!folder.isDirectory() && !folder.createDirectory())
    return File();

  File file = folder.getNonexistentChildFile(genSafeName(table.name, "Wavetable"),
                                             String(".") + vital::kWavetableExtension, false);
  table.name = file.getFileNameWithoutExtension().toStdString();
  if (!file.replaceWithText(corpuscular::gen::wavetableToJson(table, kGenAuthor).dump()))
    return File();
  return file;
}

void GeneratorSection::generateOne(bool with_wavetables) {
  if (types_.empty())
    return;

  corpuscular::gen::PresetRequest request;
  request.type = types_[selected_type_];
  request.seed = genSeed();
  request.author = kGenAuthor;
  if (scale_group_ >= 0) {
    request.scale_category = corpuscular::gen::scaleCategories()[scale_group_];
    request.scale_name = optionValue(kScale).toStdString();
  }
  request.phrase_kind = corpuscular::gen::phraseKinds()[melody_];
  request.step_tempo = kGenStepValues[step_];

  corpuscular::gen::GeneratedPreset preset = corpuscular::gen::createPreset(request);
  Random random(request.seed);
  maybeAddVocoder(preset.data, random);

  // Combo: every oscillator the preset uses gets a freshly made wavetable
  String table_names;
  if (with_wavetables) {
    json& settings = preset.data["settings"];
    for (int i = 0; i < GeneratorSection::kNumWavetableTargets; ++i) {
      std::string on_key = "osc_" + std::to_string(i + 1) + "_on";
      if (!settings.count(on_key) || settings[on_key].get<double>() == 0.0)
        continue;

      corpuscular::gen::GeneratedWavetable table = makeWavetable();
      saveWavetableToLibrary(table);
      settings["wavetables"][i] = corpuscular::gen::wavetableToJson(table, kGenAuthor);
      table_names += (table_names.isEmpty() ? "" : ", ") + String("OSC ") + String(i + 1) + " " + String(table.name);
    }
  }

  File folder = LoadSave::getUserPresetDirectory().getChildFile("Generated").getChildFile(genSafeName(preset.type, "Presets"));
  if (!folder.isDirectory() && !folder.createDirectory()) {
    setStatus("Can't create " + folder.getFullPathName());
    return;
  }

  File file = folder.getNonexistentChildFile(genSafeName(preset.name, "Preset"), String(".") + vital::kPresetExtension, false);
  preset.data["preset_name"] = file.getFileNameWithoutExtension().toStdString();
  if (!file.replaceWithText(preset.data.dump())) {
    setStatus("Can't write " + file.getFullPathName());
    return;
  }

  if (loadPreset(file)) {
    addToHistory(file);
    String status = "Loaded preset " + file.getFileNameWithoutExtension();
    if (with_wavetables)
      status += table_names.isEmpty() ? String(" (no oscillators on)") : " with " + table_names;
    setStatus(status);
  }
}

void GeneratorSection::makeSimilar() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  json current = synth->getStateJson();
  uint32_t seed = genSeed();
  json similar = corpuscular::gen::mutatePreset(current, seed);

  // "Name ~ Word": a new word for every relative of the same preset
  String base = synth->getPresetName().upToFirstOccurrenceOf(" ~ ", false, false).trim();
  if (base.isEmpty())
    base = "Preset";
  String name = base + " ~ " + String(corpuscular::gen::loremWords(seed, 1));

  File folder = LoadSave::getUserPresetDirectory().getChildFile("Generated").getChildFile("Similar");
  if (!folder.isDirectory() && !folder.createDirectory()) {
    setStatus("Can't create " + folder.getFullPathName());
    return;
  }

  File file = folder.getNonexistentChildFile(genSafeName(name, "Similar"), String(".") + vital::kPresetExtension, false);
  similar["preset_name"] = file.getFileNameWithoutExtension().toStdString();
  if (!file.replaceWithText(similar.dump())) {
    setStatus("Can't write " + file.getFullPathName());
    return;
  }

  if (loadPreset(file)) {
    addToHistory(file);
    setStatus("Loaded " + file.getFileNameWithoutExtension() + " (similar to " + base + ")");
  }
}

File GeneratorSection::historyFile() const {
  return LoadSave::getUserDirectory().getChildFile("generator_history.txt");
}

void GeneratorSection::loadHistory() {
  history_.clear();
  StringArray lines;
  historyFile().readLines(lines);
  for (const String& line : lines) {
    File file(line.trim());
    if (line.trim().isNotEmpty() && file.existsAsFile())
      history_.push_back(file);
  }
  history_index_ = (int)history_.size() - 1;
}

void GeneratorSection::saveHistory() {
  StringArray lines;
  for (const File& file : history_)
    lines.add(file.getFullPathName());
  historyFile().replaceWithText(lines.joinIntoString("\n"));
}

void GeneratorSection::addToHistory(const File& preset) {
  history_.push_back(preset);
  if ((int)history_.size() > kGenHistorySize)
    history_.erase(history_.begin());
  history_index_ = (int)history_.size() - 1;
  saveHistory();
}

void GeneratorSection::stepHistory(int delta) {
  // Files deleted since are skipped
  int index = history_index_ + delta;
  while (index >= 0 && index < (int)history_.size() && !history_[index].existsAsFile())
    index += delta;
  if (index < 0 || index >= (int)history_.size())
    return;

  history_index_ = index;
  if (loadPreset(history_[index]))
    setStatus("Back to " + history_[index].getFileNameWithoutExtension());
}

void GeneratorSection::makeBank() {
  std::vector<std::string> plan;
  for (size_t i = 0; i < types_.size(); ++i) {
    for (int n = 0; n < bank_counts_[i]; ++n)
      plan.push_back(types_[i]);
  }
  Random random(genSeed());
  for (int n = 0; n < random_count_ && !types_.empty(); ++n)
    plan.push_back(types_[random.nextInt((int)types_.size())]);

  if (plan.empty()) {
    setStatus("The bank is empty");
    return;
  }

  // Catalog order, like the standalone generator
  std::stable_sort(plan.begin(), plan.end(), [this](const std::string& a, const std::string& b) {
    return std::find(types_.begin(), types_.end(), a) < std::find(types_.begin(), types_.end(), b);
  });

  std::vector<corpuscular::gen::PresetRequest> requests;
  for (const std::string& type : plan) {
    corpuscular::gen::PresetRequest request;
    request.type = type;
    request.seed = static_cast<uint32_t>(random.nextInt());
    request.author = kGenAuthor;
    if (scale_group_ >= 0) {
      request.scale_category = corpuscular::gen::scaleCategories()[scale_group_];
      request.scale_name = optionValue(kScale).toStdString();
    }
    request.phrase_kind = corpuscular::gen::phraseKinds()[melody_];
    request.step_tempo = kGenStepValues[step_];
    requests.push_back(request);
  }

  GenBankBuilder builder(requests);
  if (!builder.runThread() || builder.presets().size() != requests.size()) {
    setStatus("Bank cancelled");
    return;
  }

  // The bank goes into the data folder like an imported bank, so the preset browser lists it
  File data_directory = LoadSave::getDataDirectory();
  File bank_folder = data_directory.getChildFile(genSafeName(bank_name_, "Corpuscular Bank")).getNonexistentSibling(true);
  File presets_folder = bank_folder.getChildFile(LoadSave::kPresetFolderName);
  if (!presets_folder.createDirectory()) {
    setStatus("Can't create " + presets_folder.getFullPathName());
    return;
  }

  File first_preset;
  std::set<String> used;
  for (corpuscular::gen::GeneratedPreset& preset : builder.presets()) {
    maybeAddVocoder(preset.data, random);
    String folder_name = genSafeName(preset.type, "Presets");
    String base = genSafeName(preset.name, "Preset");
    String name = base;
    for (int n = 2; !used.insert(folder_name.toLowerCase() + "/" + name.toLowerCase()).second; ++n)
      name = base + " " + String(n);

    File folder = presets_folder.getChildFile(folder_name);
    folder.createDirectory();
    File file = folder.getChildFile(name + "." + vital::kPresetExtension);
    preset.data["preset_name"] = name.toStdString();
    if (!file.replaceWithText(preset.data.dump())) {
      setStatus("Can't write " + file.getFullPathName());
      return;
    }
    if (first_preset == File())
      first_preset = file;
  }

  if (loadPreset(first_preset))
    addToHistory(first_preset);
  setStatus("Bank " + bank_folder.getFileName() + ": " + String((int)builder.presets().size()) + " presets");
  newBankName();
  repaintBackground();
  if (bank_viewer_)
    bank_viewer_();
}

void GeneratorSection::wavetableToOscillator(int index) {
  corpuscular::gen::GeneratedWavetable table = makeWavetable();

  // Saved into the wavetable library first, so the oscillator browser can step through it later
  File file = saveWavetableToLibrary(table);
  if (file == File()) {
    setStatus("Can't write to " + LoadSave::getUserWavetableDirectory().getFullPathName());
    return;
  }

  if (wavetable_loader_)
    wavetable_loader_(index, file);
  setStatus("OSC " + String(index + 1) + ": " + file.getFileNameWithoutExtension() + " (" + String(table.type) + ")");
}

void GeneratorSection::exportWavetable() {
  corpuscular::gen::GeneratedWavetable table = makeWavetable();

  auto format = static_cast<corpuscular::gen::WavetableFormat>(table_format_);
  String extension = corpuscular::gen::wavetableFormatExtension(format);
  File desktop = File::getSpecialLocation(File::userDesktopDirectory);
  FileChooser chooser("Export Wavetable", desktop.getChildFile(genSafeName(table.name, "Wavetable") + "." + extension),
                      "*." + extension);
  if (!chooser.browseForFileToSave(true))
    return;

  File file = chooser.getResult().withFileExtension(extension);
  std::vector<uint8_t> bytes = corpuscular::gen::encodeWavetable(table, format, kGenAuthor);
  if (!file.replaceWithData(bytes.data(), bytes.size())) {
    setStatus("Can't write " + file.getFullPathName());
    return;
  }
  setStatus("Exported " + file.getFullPathName());
}

void GeneratorSection::mouseDown(const MouseEvent& e) {
  Point<float> point = e.position;
  int step = e.mods.isShiftDown() ? 10 : 1;

  for (int i = 0; i < (int)types_.size(); ++i) {
    Rectangle<float> cell = typeCell(i);
    if (!cell.contains(point))
      continue;

    Rectangle<float> counter = cell.withTop(cell.getY() + cell.getHeight() * 0.58f);
    if (counter.contains(point)) {
      int delta = point.x < counter.getCentreX() ? -step : step;
      bank_counts_[i] = jlimit(0, kGenMaxCount, bank_counts_[i] + delta);
    }
    else
      selected_type_ = i;
    repaintBackground();
    return;
  }

  for (int option = 0; option < kNumOptions; ++option) {
    Rectangle<float> box = optionBox(option);
    if (box.contains(point)) {
      stepOption(option, point.x < box.getCentreX() ? -1 : 1);
      repaintBackground();
      return;
    }
  }

  if (generateButton().contains(point)) {
    generateOne(false);
    return;
  }

  if (comboButton().contains(point)) {
    generateOne(true);
    return;
  }

  if (similarButton().contains(point)) {
    makeSimilar();
    return;
  }

  if (historyBackButton().contains(point)) {
    stepHistory(-1);
    repaintBackground();
    return;
  }

  if (historyForwardButton().contains(point)) {
    stepHistory(1);
    repaintBackground();
    return;
  }

  Rectangle<float> random_box = randomCountBox();
  if (random_box.contains(point)) {
    random_count_ = jlimit(0, kGenMaxCount, random_count_ + (point.x < random_box.getCentreX() ? -step : step));
    repaintBackground();
    return;
  }

  if (bankNameBox().contains(point)) {
    newBankName();
    repaintBackground();
    return;
  }

  if (makeBankButton().contains(point)) {
    makeBank();
    return;
  }

  for (int option = 0; option < kNumTableOptions; ++option) {
    Rectangle<float> box = tableOptionBox(option);
    if (box.contains(point)) {
      stepTableOption(option, point.x < box.getCentreX() ? -1 : 1);
      repaintBackground();
      return;
    }
  }

  for (int i = 0; i < kNumWavetableTargets; ++i) {
    if (tableTargetButton(i).contains(point)) {
      wavetableToOscillator(i);
      return;
    }
  }

  if (tableExportButton().contains(point))
    exportWavetable();
}

void GeneratorSection::paintButton(Graphics& g, Rectangle<float> bounds, const String& text, bool lit, float font_height) {
  float rounding = findValue(Skin::kLabelBackgroundRounding);
  g.setColour(lit ? findColour(Skin::kRotaryArc, true) : findColour(Skin::kTextComponentBackground, true));
  g.fillRoundedRectangle(bounds, rounding);
  g.setColour(findColour(Skin::kBodyText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height));
  g.drawFittedText(text, bounds.toNearestInt(), Justification::centred, 1, 0.7f);
}

void GeneratorSection::paintStepper(Graphics& g, Rectangle<float> bounds, const String& title, const String& value,
                                    float font_height) {
  float rounding = findValue(Skin::kLabelBackgroundRounding);
  Rectangle<float> title_bounds = bounds.removeFromTop(bounds.getHeight() * 0.38f);
  g.setColour(findColour(Skin::kBodyText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height * 0.8f));
  g.drawText(title, title_bounds, Justification::centred);

  g.setColour(findColour(Skin::kTextComponentBackground, true));
  g.fillRoundedRectangle(bounds, rounding);
  g.setColour(findColour(Skin::kTextComponentText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height));
  Rectangle<float> arrows = bounds.reduced(bounds.getHeight() * 0.3f, 0.0f);
  g.drawText("<", arrows, Justification::centredLeft);
  g.drawText(">", arrows, Justification::centredRight);
  g.drawFittedText(value, arrows.reduced(font_height, 0.0f).toNearestInt(), Justification::centred, 1, 0.6f);
}

void GeneratorSection::paintBackground(Graphics& g) {
  paintContainer(g);
  paintHeadingText(g);

  Colour text = findColour(Skin::kBodyText, true);
  Colour cell_colour = findColour(Skin::kTextComponentBackground, true);
  Colour active = findColour(Skin::kRotaryArc, true);
  float rounding = findValue(Skin::kLabelBackgroundRounding);
  Rectangle<float> content = contentArea();
  float font_height = std::max(9.0f, content.getHeight() * 0.025f);

  for (int i = 0; i < (int)types_.size(); ++i) {
    Rectangle<float> cell = typeCell(i);
    bool selected = i == selected_type_;
    g.setColour(cell_colour);
    g.fillRoundedRectangle(cell, rounding);
    if (selected) {
      g.setColour(active);
      g.drawRoundedRectangle(cell.reduced(1.0f), rounding, std::max(1.5f, 2.0f * size_ratio_));
    }

    Rectangle<float> name_bounds = cell.withHeight(cell.getHeight() * 0.58f);
    Rectangle<float> counter = cell.withTop(name_bounds.getBottom());
    g.setColour(selected ? active : text);
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height));
    g.drawText(String(types_[i]).toUpperCase(), name_bounds, Justification::centred);

    g.setColour(bank_counts_[i] ? active : text.withMultipliedAlpha(0.6f));
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height * 0.9f));
    Rectangle<float> counter_text = counter.reduced(counter.getHeight() * 0.4f, 0.0f);
    g.drawText("-", counter_text, Justification::centredLeft);
    g.drawText("+", counter_text, Justification::centredRight);
    g.drawText(String(bank_counts_[i]), counter_text, Justification::centred);
  }

  for (int option = 0; option < kNumOptions; ++option)
    paintStepper(g, optionBox(option), kGenOptionTitles[option], optionValue(option), font_height);

  String type_name = types_.empty() ? String() : String(types_[selected_type_]).toUpperCase();
  paintButton(g, generateButton(), "NEW " + type_name + " PRESET", true, font_height * 1.1f);
  paintButton(g, comboButton(), "NEW " + type_name + " + WAVETABLES", true, font_height * 1.1f);
  paintButton(g, similarButton(), "SIMILAR", true, font_height * 1.1f);

  bool can_go_back = history_index_ > 0;
  bool can_go_forward = history_index_ >= 0 && history_index_ + 1 < (int)history_.size();
  paintButton(g, historyBackButton(), "<  BACK", can_go_back, font_height);
  paintButton(g, historyForwardButton(), "NEXT  >", can_go_forward, font_height);
  Rectangle<float> history_box = historyNameBox();
  g.setColour(cell_colour);
  g.fillRoundedRectangle(history_box, rounding);
  String history_text = "No presets made yet";
  if (history_index_ >= 0 && history_index_ < (int)history_.size()) {
    history_text = String(history_index_ + 1) + " / " + String((int)history_.size()) + "    " +
                   history_[history_index_].getFileNameWithoutExtension();
  }
  g.setColour(findColour(Skin::kTextComponentText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height));
  g.drawFittedText(history_text, history_box.reduced(font_height, 0.0f).toNearestInt(), Justification::centred, 1, 0.7f);

  paintStepper(g, randomCountBox(), "RANDOM TYPES", String(random_count_), font_height);

  Rectangle<float> name_box = bankNameBox();
  g.setColour(text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height * 0.8f));
  g.drawText("BANK NAME", name_box.removeFromTop(name_box.getHeight() * 0.38f), Justification::centred);
  g.setColour(cell_colour);
  g.fillRoundedRectangle(name_box, rounding);
  g.setColour(findColour(Skin::kTextComponentText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height));
  g.drawFittedText(bank_name_, name_box.reduced(font_height, 0.0f).toNearestInt(), Justification::centred, 1, 0.7f);

  int total = random_count_;
  for (int count : bank_counts_)
    total += count;
  paintButton(g, makeBankButton(), "MAKE BANK (" + String(total) + ")", total > 0, font_height);

  // Wavetables
  Rectangle<float> divider = band(0.70f, 0.73f);
  g.setColour(text.withMultipliedAlpha(0.35f));
  g.fillRect(divider.getX(), divider.getCentreY(), divider.getWidth(), std::max(1.0f, size_ratio_));
  g.setColour(findColour(Skin::kBody, true));
  float label_width = divider.getWidth() * 0.22f;
  Rectangle<float> label(divider.getCentreX() - label_width / 2.0f, divider.getY(), label_width, divider.getHeight());
  g.fillRect(label);
  g.setColour(text);
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height * 0.9f));
  g.drawText("WAVETABLES", label, Justification::centred);

  for (int option = 0; option < kNumTableOptions; ++option)
    paintStepper(g, tableOptionBox(option), kGenTableTitles[option], tableOptionValue(option), font_height);

  for (int i = 0; i < kNumWavetableTargets; ++i)
    paintButton(g, tableTargetButton(i), "NEW TO OSC " + String(i + 1), true, font_height);
  paintButton(g, tableExportButton(), "EXPORT FILE", false, font_height);

  g.setColour(text.withMultipliedAlpha(0.8f));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_height * 0.85f));
  g.drawFittedText(status_, statusArea().toNearestInt(), Justification::centredLeft, 1, 0.6f);
}
