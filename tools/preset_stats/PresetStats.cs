// Corpuscular preset statistics: reads a folder tree of .vital presets and reports, per sound type,
// where the knobs actually sit (percentiles), which switches and models are used and what modulates what.
// Build: csc /out:PresetStats.exe PresetStats.cs      Run: PresetStats.exe <report.txt> <folder> [<folder>...]
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;

static class Json {
  // Minimal JSON reader: objects -> Dictionary, arrays -> List, numbers -> double
  public static object Parse(string s) { int i = 0; return Value(s, ref i); }

  static void Skip(string s, ref int i) { while (i < s.Length && char.IsWhiteSpace(s[i])) i++; }

  static object Value(string s, ref int i) {
    Skip(s, ref i);
    char c = s[i];
    if (c == '{') {
      var d = new Dictionary<string, object>(); i++;
      Skip(s, ref i);
      if (s[i] == '}') { i++; return d; }
      while (true) {
        Skip(s, ref i); string k = Str(s, ref i); Skip(s, ref i); i++; // ':'
        d[k] = Value(s, ref i); Skip(s, ref i);
        if (s[i] == ',') { i++; continue; }
        i++; return d;
      }
    }
    if (c == '[') {
      var l = new List<object>(); i++;
      Skip(s, ref i);
      if (s[i] == ']') { i++; return l; }
      while (true) {
        l.Add(Value(s, ref i)); Skip(s, ref i);
        if (s[i] == ',') { i++; continue; }
        i++; return l;
      }
    }
    if (c == '"') return Str(s, ref i);
    if (s.Substring(i, Math.Min(4, s.Length - i)) == "true") { i += 4; return 1.0; }
    if (s.Substring(i, Math.Min(5, s.Length - i)) == "false") { i += 5; return 0.0; }
    if (s.Substring(i, Math.Min(4, s.Length - i)) == "null") { i += 4; return null; }
    int start = i;
    while (i < s.Length && "+-0123456789.eE".IndexOf(s[i]) >= 0) i++;
    return double.Parse(s.Substring(start, i - start), CultureInfo.InvariantCulture);
  }

  static string Str(string s, ref int i) {
    i++; var sb = new StringBuilder();
    while (s[i] != '"') {
      if (s[i] == '\\') { i++; char e = s[i]; if (e == 'u') { sb.Append((char)Convert.ToInt32(s.Substring(i + 1, 4), 16)); i += 4; } else sb.Append(e == 'n' ? '\n' : e == 't' ? '\t' : e); }
      else sb.Append(s[i]);
      i++;
    }
    i++; return sb.ToString();
  }
}

class Preset {
  public string Name, Style, Folder, Type;
  public Dictionary<string, double> S = new Dictionary<string, double>();
  public List<Tuple<string, string, double>> Mods = new List<Tuple<string, string, double>>();
  public double Get(string k, double d = 0) { double v; return S.TryGetValue(k, out v) ? v : d; }
}

static class Program {
  static readonly string[] Types = { "Bass", "Lead", "Pluck", "Arp", "Sequence", "Pad", "Keys", "Stab", "Vocal", "SFX", "Percussion", "Other" };

  static string Classify(string name, string style) {
    string n = " " + name.ToUpperInvariant().Replace('_', ' ').Replace('-', ' ') + " ";
    Func<string[], bool> has = keys => keys.Any(k => n.Contains(" " + k + " ") || n.StartsWith(" " + k));
    if (has(new[] { "ARP", "ARPS" })) return "Arp";
    if (has(new[] { "SQ", "SEQ", "SEQUENCE", "SEQ." })) return "Sequence";
    if (has(new[] { "BS", "BA", "BASS", "SUB", "ROLL", "RB" })) return "Bass";
    if (has(new[] { "LD", "LEAD", "LEADS" })) return "Lead";
    if (has(new[] { "PL", "PLK", "PLUCK" })) return "Pluck";
    if (has(new[] { "PD", "PAD", "ATM", "ATMO", "TX", "TEXTURE", "AMB", "DRONE" })) return "Pad";
    if (has(new[] { "KEY", "KEYS", "KY", "PIANO", "BELL", "BL" })) return "Keys";
    if (has(new[] { "STAB", "ST", "STB", "CHORD", "CH" })) return "Stab";
    if (has(new[] { "VOX", "VOC", "VOCAL", "TALK" })) return "Vocal";
    if (has(new[] { "FX", "SFX", "RISER", "RS", "SWEEP", "NOISE", "IMPACT", "DOWNLIFTER", "UPLIFTER" })) return "SFX";
    if (has(new[] { "DR", "DRUM", "KICK", "PERC", "HAT", "SNARE", "CLAP" })) return "Percussion";
    switch ((style ?? "").ToLowerInvariant()) {
      case "bass": return "Bass";
      case "lead": return "Lead";
      case "pluck": return "Pluck";
      case "pad": return "Pad";
      case "keys": return "Keys";
      case "sequence": return "Sequence";
      case "arp": return "Arp";
      case "sfx": case "experimental": return "SFX";
      case "percussion": return "Percussion";
      case "vocal": return "Vocal";
    }
    return "Other";
  }

  static Preset Load(string file, string folder) {
    var root = Json.Parse(File.ReadAllText(file)) as Dictionary<string, object>;
    if (root == null || !root.ContainsKey("settings")) return null;
    var p = new Preset { Folder = folder, Name = Path.GetFileNameWithoutExtension(file) };
    object o;
    p.Style = root.TryGetValue("preset_style", out o) ? o as string : "";
    var st = (Dictionary<string, object>)root["settings"];
    foreach (var kv in st) if (kv.Value is double) p.S[kv.Key] = (double)kv.Value;
    if (st.TryGetValue("modulations", out o) && o is List<object>) {
      int idx = 0;
      foreach (var m in (List<object>)o) {
        idx++;
        var md = m as Dictionary<string, object>;
        if (md == null) continue;
        string src = md.ContainsKey("source") ? md["source"] as string : "";
        string dst = md.ContainsKey("destination") ? md["destination"] as string : "";
        if (string.IsNullOrEmpty(src) || string.IsNullOrEmpty(dst)) continue;
        p.Mods.Add(Tuple.Create(src, dst, p.Get("modulation_" + idx + "_amount")));
      }
    }
    p.Type = Classify(p.Name, p.Style);
    return p;
  }

  static string Pct(List<double> v) {
    if (v.Count == 0) return "-";
    v.Sort();
    Func<double, double> q = f => v[(int)Math.Round(f * (v.Count - 1))];
    return string.Format(CultureInfo.InvariantCulture, "n={0,4}  p10={1,8:0.###}  p25={2,8:0.###}  med={3,8:0.###}  p75={4,8:0.###}  p90={5,8:0.###}",
                         v.Count, q(0.1), q(0.25), q(0.5), q(0.75), q(0.9));
  }

  static string Dist(IEnumerable<double> values) {
    var g = values.GroupBy(x => Math.Round(x, 3)).OrderByDescending(x => x.Count()).Take(8).ToList();
    int total = values.Count();
    if (total == 0) return "-";
    return string.Join("  ", g.Select(x => string.Format(CultureInfo.InvariantCulture, "{0}:{1:0}%", x.Key, 100.0 * x.Count() / total)));
  }

  static void Main(string[] args) {
    var all = new List<Preset>();
    foreach (string dir in args.Skip(1)) {
      foreach (string f in Directory.EnumerateFiles(dir, "*.vital", SearchOption.AllDirectories)) {
        try { var p = Load(f, Path.GetFileName(dir)); if (p != null) all.Add(p); }
        catch (Exception e) { Console.Error.WriteLine("skip " + f + ": " + e.Message); }
      }
    }

    var w = new StringBuilder();
    w.AppendLine("Presets read: " + all.Count);
    foreach (var t in Types) w.AppendLine(string.Format("  {0,-11} {1}", t, all.Count(p => p.Type == t)));

    // Knobs reported only where their module is on
    var oscKnobs = new[] { "level", "transpose", "tune", "unison_voices", "unison_detune", "unison_blend", "stereo_spread", "wave_frame",
                           "spectral_morph_amount", "distortion_amount", "pan", "destination", "spectral_morph_type", "distortion_type", "random_phase", "phase" };
    var filterKnobs = new[] { "model", "style", "cutoff", "resonance", "drive", "blend", "keytrack", "mix", "formant_x", "formant_y" };
    var fx = new Dictionary<string, string[]> {
      { "distortion", new[] { "type", "drive", "mix", "filter_order", "filter_cutoff" } },
      { "compressor", new[] { "mix", "enabled_bands", "attack", "release" } },
      { "delay", new[] { "dry_wet", "feedback", "tempo", "sync", "style", "filter_cutoff", "filter_spread" } },
      { "reverb", new[] { "dry_wet", "size", "decay_time", "pre_low_cutoff", "pre_high_cutoff", "high_shelf_gain", "chorus_amount" } },
      { "chorus", new[] { "dry_wet", "voices", "feedback", "mod_depth", "cutoff" } },
      { "phaser", new[] { "dry_wet", "feedback", "center", "mod_depth", "tempo" } },
      { "flanger", new[] { "dry_wet", "feedback", "center", "mod_depth", "tempo" } },
      { "eq", new[] { "low_mode", "low_cutoff", "low_gain", "band_cutoff", "band_gain", "high_mode", "high_cutoff", "high_gain" } },
      { "filter_fx", new[] { "model", "style", "cutoff", "resonance", "drive", "mix" } },
    };

    foreach (var t in Types) {
      var g = all.Where(p => p.Type == t).ToList();
      if (g.Count < 5) continue;
      w.AppendLine(); w.AppendLine("==================== " + t + " (" + g.Count + ") ====================");
      w.AppendLine("volume          " + Pct(g.Select(p => p.Get("volume", 5473)).ToList()));
      w.AppendLine("polyphony       " + Dist(g.Select(p => p.Get("polyphony", 8))));
      w.AppendLine("legato          " + Dist(g.Select(p => p.Get("legato"))));
      w.AppendLine("portamento_time " + Pct(g.Select(p => p.Get("portamento_time", -10)).ToList()));
      w.AppendLine("oversampling    " + Dist(g.Select(p => p.Get("oversampling", 1))));
      w.AppendLine("oscs on count   " + Dist(g.Select(p => (double)Enumerable.Range(1, 3).Count(o => p.Get("osc_" + o + "_on") > 0.5))));
      w.AppendLine("sample on       " + Dist(g.Select(p => p.Get("sample_on"))) + "   level " + Pct(g.Where(p => p.Get("sample_on") > 0.5).Select(p => p.Get("sample_level")).ToList()));
      for (int o = 1; o <= 3; o++) {
        var on = g.Where(p => p.Get("osc_" + o + "_on") > 0.5).ToList();
        w.AppendLine(string.Format("-- osc {0} on in {1:0}%", o, 100.0 * on.Count / g.Count));
        foreach (var k in oscKnobs) {
          string key = "osc_" + o + "_" + k;
          if (k == "destination" || k == "spectral_morph_type" || k == "distortion_type" || k == "random_phase" || k == "transpose" || k == "unison_voices")
            w.AppendLine(string.Format("   {0,-22} {1}", k, Dist(on.Select(p => p.Get(key)))));
          else
            w.AppendLine(string.Format("   {0,-22} {1}", k, Pct(on.Select(p => p.Get(key)).ToList())));
        }
      }
      for (int f = 1; f <= 2; f++) {
        var on = g.Where(p => p.Get("filter_" + f + "_on") > 0.5).ToList();
        w.AppendLine(string.Format("-- filter {0} on in {1:0}%", f, 100.0 * on.Count / g.Count));
        foreach (var k in filterKnobs) {
          string key = "filter_" + f + "_" + k;
          if (k == "model" || k == "style") w.AppendLine(string.Format("   {0,-22} {1}", k, Dist(on.Select(p => p.Get(key)))));
          else w.AppendLine(string.Format("   {0,-22} {1}", k, Pct(on.Select(p => p.Get(key)).ToList())));
        }
      }
      w.AppendLine("filter routing  serial " + Dist(g.Select(p => p.Get("filter_1_filter_input"))));
      for (int e = 1; e <= 3; e++) {
        w.AppendLine("-- env " + e);
        foreach (var k in new[] { "delay", "attack", "hold", "decay", "sustain", "release", "attack_power", "decay_power", "release_power" })
          w.AppendLine(string.Format("   {0,-22} {1}", k, Pct(g.Select(p => p.Get("env_" + e + "_" + k)).ToList())));
      }
      foreach (var kv in fx) {
        var on = g.Where(p => p.Get(kv.Key + "_on") > 0.5).ToList();
        w.AppendLine(string.Format("-- {0} on in {1:0}%", kv.Key, 100.0 * on.Count / g.Count));
        foreach (var k in kv.Value) {
          string key = kv.Key + "_" + k;
          bool cat = k == "type" || k == "style" || k == "model" || k == "sync" || k == "tempo" || k.EndsWith("_mode") || k == "voices" || k == "enabled_bands" || k == "filter_order";
          w.AppendLine(string.Format("   {0,-22} {1}", k, cat ? Dist(on.Select(p => p.Get(key))) : Pct(on.Select(p => p.Get(key)).ToList())));
        }
      }
      w.AppendLine("effect order    " + Dist(g.Select(p => p.Get("effect_chain_order"))));
      w.AppendLine("lfo tempo       " + Dist(g.SelectMany(p => Enumerable.Range(1, 4).Where(l => p.Mods.Any(m => m.Item1 == "lfo_" + l)).Select(l => p.Get("lfo_" + l + "_tempo")))));
      w.AppendLine("lfo sync        " + Dist(g.SelectMany(p => Enumerable.Range(1, 4).Where(l => p.Mods.Any(m => m.Item1 == "lfo_" + l)).Select(l => p.Get("lfo_" + l + "_sync")))));
      w.AppendLine("lfo sync_type   " + Dist(g.SelectMany(p => Enumerable.Range(1, 4).Where(l => p.Mods.Any(m => m.Item1 == "lfo_" + l)).Select(l => p.Get("lfo_" + l + "_sync_type")))));
      w.AppendLine("mods per preset " + Pct(g.Select(p => (double)p.Mods.Count).ToList()));

      // Most common modulations: source family -> destination
      Func<string, string> family = s => new string(s.TakeWhile(ch => !char.IsDigit(ch)).ToArray()).TrimEnd('_');
      var pairs = g.SelectMany(p => p.Mods.Select(m => new { P = p, Key = family(m.Item1) + " -> " + m.Item2, A = m.Item3 }))
                   .GroupBy(x => x.Key).OrderByDescending(x => x.Select(y => y.P).Distinct().Count()).Take(30);
      w.AppendLine("-- top modulations (share of presets, amount percentiles)");
      foreach (var pr in pairs)
        w.AppendLine(string.Format("   {0,-44} {1,4:0}%  {2}", pr.Key, 100.0 * pr.Select(y => y.P).Distinct().Count() / g.Count, Pct(pr.Select(y => y.A).ToList())));
    }

    // Type fingerprints: what makes the producers' leads, pads, plucks, basses, sequences and arps what they are
    w.AppendLine(); w.AppendLine("==================== TYPE FINGERPRINTS ====================");
    w.AppendLine("times in seconds; register = lowest transpose among sounding oscillators; reverb = dry/wet when on, else 0");
    foreach (var t in Types) {
      var g = all.Where(p => p.Type == t).ToList();
      if (g.Count < 5) continue;
      Func<Preset, string, double> secs = (p, k) => Math.Pow(Math.Max(0, p.Get(k)), 4);
      Func<Preset, bool> pitchPattern = p => p.Mods.Any(m => m.Item1.StartsWith("lfo_") && m.Item2.EndsWith("_transpose"));
      Func<Preset, bool> gate = p => p.Mods.Any(m => m.Item1.StartsWith("lfo_") && (m.Item2.EndsWith("_level") && m.Item2.StartsWith("osc_")));
      Func<Preset, double> register = p => {
        var on = Enumerable.Range(1, 3).Where(o => p.Get("osc_" + o + "_on") > 0.5).ToList();
        return on.Count == 0 ? 0 : on.Min(o => p.Get("osc_" + o + "_transpose"));
      };
      w.AppendLine(); w.AppendLine("-- " + t + " (" + g.Count + ")");
      w.AppendLine("   attack s        " + Pct(g.Select(p => secs(p, "env_1_attack")).ToList()));
      w.AppendLine("   decay s         " + Pct(g.Select(p => secs(p, "env_1_decay")).ToList()));
      w.AppendLine("   sustain         " + Pct(g.Select(p => p.Get("env_1_sustain")).ToList()));
      w.AppendLine("   release s       " + Pct(g.Select(p => secs(p, "env_1_release")).ToList()));
      w.AppendLine("   register        " + Pct(g.Select(register).ToList()));
      w.AppendLine("   reverb mix      " + Pct(g.Select(p => p.Get("reverb_on") > 0.5 ? p.Get("reverb_dry_wet") : 0).ToList()));
      w.AppendLine("   max unison      " + Pct(g.Select(p => Enumerable.Range(1, 3).Where(o => p.Get("osc_" + o + "_on") > 0.5).Select(o => p.Get("osc_" + o + "_unison_voices")).DefaultIfEmpty(1).Max()).ToList()));
      w.AppendLine(string.Format("   pitch pattern {0,4:0}%   gate {1,4:0}%   mono {2,4:0}%   legato {3,4:0}%",
                                 100.0 * g.Count(pitchPattern) / g.Count, 100.0 * g.Count(gate) / g.Count,
                                 100.0 * g.Count(p => p.Get("polyphony", 8) <= 1) / g.Count, 100.0 * g.Count(p => p.Get("legato") > 0.5) / g.Count));
    }

    File.WriteAllText(args[0], w.ToString());
    Console.WriteLine("presets: " + all.Count + ", report: " + args[0]);
  }
}
