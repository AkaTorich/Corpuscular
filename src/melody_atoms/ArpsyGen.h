/* Corpuscular — synthesizer based on Vital by Matt Tytel (GPLv3).
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "MusicData.h"

// PSY ARP — арп по рецепту, вычисленному из 515 петель Sonicspore ARPSY (tools/ArpAnalyzer/report.txt):
//   • 16-е одной длины и громкости (в ~22% петель есть паузы, в ~11% — такт из двух одинаковых половин);
//   • основа — тоника, прыгающая по октавам (≈65% нот, 80% петель начинаются с неё);
//   • «цветные» ступени лада вкраплены по одной, чаще к концу такта (больше всего — 15-я шестнадцатая);
//     частота ступеней как в ARPSY: ♭2, 5, 4, ♭7, ♭3, 3…;
//   • такт — цельная фраза из 16 шагов;
//   • 4 такта по схеме (ABAC 47%, ABCD 36%…): «ответы» меняют 1–10 нот, чаще в конце такта —
//     перенос на октаву, шаг по ладу, другая цветная нота; кончаются чаще на 1, 5, ♭2.
namespace mg
{
    const juce::StringArray& arpsyStructureNames();   // "auto", "ABAC", "ABCD", "ABAB", "AABC", "ABCB", "AAAB"

    // colorNotes — сколько не-тоник в такте A (0..12); octaves — диапазон прыжков (1..3);
    // structure — индекс в arpsyStructureNames()
    Clip generateArpsy(const juce::String& tonality, const juce::Array<int>& steps,
                       int colorNotes, int bars, int octaves, int structure,
                       juce::Random& rng, juce::StringArray& infoOut);
}
