#pragma leco tool
export module song;
import nessa;

using namespace nessa::midi;

export struct song : nessa::midi::player {
  constexpr auto clamp(float b) const noexcept { return b > 1.0f ? 1.0 : b; }
  constexpr float nenv(float tb) const noexcept {
    return 1.0 - clamp(tb * 8.0f);
  }

  constexpr float vol_at(float t) const noexcept override {
    constexpr const auto volume = 0.125f;

    float tb = t * bps();

    float vsq1 = nessa::gen::square(t * note_freq(0));
    float vsq2 = nessa::gen::square(t * note_freq(1));
    float vtri = nessa::gen::triangle(t * note_freq(2));
    float vnoi = 0.5 * nenv(tb) * nessa::gen::noise(t * note_freq(3));

    return (vsq1 + vsq2 + vtri + vnoi) * volume;
  }

  void play(unsigned i, note a, note b) {
    note r = i % 2 == 0 ? G4 : G5;
    play_notes({a, b, MUTE, r});
  }
};

void p0(song &s) {
  for (auto i = 0; i < 8; i++) {
    s.play(i, MUTE, MUTE);
  }
}

extern "C" int main() {
  song s{};
  s.set_bpm(140);

  p0(s);
  p0(s);

  s.play(0, C5, MUTE);
  s.play(1, EXTEND, EXTEND);
  s.play(2, EXTEND, G5);
  s.play(3, EXTEND, EXTEND);
  s.play(4, C5, A4);
  s.play(5, A4, MUTE);
  s.play(6, C5, MUTE);
  s.play(7, F5, MUTE);

  s.play(0, C5, MUTE);
  s.play(1, EXTEND, EXTEND);
  s.play(2, EXTEND, G5);
  s.play(3, EXTEND, EXTEND);
  s.play(4, C5, A4);
  s.play(5, E4, MUTE);
  s.play(6, C5, MUTE);
  s.play(7, A5, MUTE);

  s.play(0, C5, MUTE);
  s.play(1, EXTEND, EXTEND);
  s.play(2, EXTEND, G5);
  s.play(3, EXTEND, EXTEND);
  s.play(4, C5, A4);
  s.play(5, D4, MUTE);
  s.play(6, C5, MUTE);
  s.play(7, B5, MUTE);
  s.play(8, C5, MUTE);
  s.play(9, B5, MUTE);
  s.play(10, F6, MUTE);
  s.play(11, EXTEND, MUTE);

  s.play(0, C5, MUTE);
  s.play(1, D4, MUTE);
  s.play(2, C5, MUTE);
  s.play(3, B5, MUTE);
  s.play(4, C5, MUTE);
  s.play(5, B5, MUTE);
  s.play(6, F6, MUTE);
  s.play(7, EXTEND, MUTE);

  s.play(0, F6, MUTE);
  s.play(1, D6, MUTE);
  s.play(2, F6, MUTE);
  s.play(3, D6, MUTE);
  s.play(4, F6, MUTE);
  s.play(5, D6, MUTE);
  s.play(6, F5, MUTE);
  s.play(7, D5, MUTE);

  s.play(0, F6, MUTE);
  s.play(1, D6, MUTE);
  s.play(2, F6, MUTE);
  s.play(3, D6, MUTE);
  s.play(4, F6, MUTE);
  s.play(5, D6, MUTE);
}
