export module song;
import nessa;
import sith;
import sitime;

using namespace nessa::midi;

export struct song : nessa::midi::player {
  constexpr auto clamp(float b) const noexcept { return b > 1.0f ? 1.0 : b; }
  constexpr float nenv(float tb) const noexcept {
    return 1.0 - clamp(tb * 8.0f);
  }
  constexpr float senv(float tb) const noexcept {
    return 0.9 - clamp(tb * 2.0f) * 0.4;
  }

  constexpr float vol_at(float t) const noexcept override {
    float tb = t * bps();

    float vsq1 = 0.5 * senv(tb) * nessa::gen::square(t * note_freq(0));
    float vsq2 = 0.5 * senv(tb) * nessa::gen::square(t * note_freq(1));
    float vtri = nessa::gen::triangle(t * note_freq(2));
    float vnoi = nenv(tb) * nessa::gen::noise(t * note_freq(3));

    return 0.02 * (vsq1 + vsq2 + vtri + vnoi);
  }

  void play(unsigned i, note a, note b) {
    note r = i % 2 == 0 ? G4 : G5;
    play_notes({a, b, MUTE, r});
  }

  void p0() {
    for (auto i = 0; i < 8; i++) {
      play(i, MUTE, MUTE);
    }
  }

  void ps(auto... ns) {
    static_assert(sizeof...(ns) == 6 || sizeof...(ns) == 8);
    auto i = 0;
    (play(i++, ns, MUTE), ...);
  }

  static void play(sith::thread *);
};

void song::play(sith::thread *) {
  song s{};
  s.set_bpm(140);

  //

  s.p0();
  s.p0();

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

  //

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

  s.ps(C5, D4, C5, B5, C5, B5, F6, EXTEND);
  s.ps(F6, D6, F6, D6, F6, D6, F5, D5);
  s.ps(F6, D6, F6, D6, F6, D6);

  //

  s.ps(F5, D5, D5, E5, E5, G5);
  s.ps(F5, D4, D4, E4, E4, G4);
  s.ps(F4, D4, MUTE, G4, F4, D4, MUTE, G4);
  s.ps(F4, E4, C4, G4, F4, C4, A3, G4);

  //

  s.ps(F4, E4, B3, A3, G3, A3, B3, MUTE);
  s.ps(D4, C4, B3, MUTE, D4, C4, B3, MUTE);

  s.play(0, D4, MUTE);
  s.play(1, C4, MUTE);
  s.play_notes({B3, MUTE, MUTE, MUTE});
  s.play_notes({A3, MUTE, MUTE, MUTE});
  s.play_notes({G3, MUTE, MUTE, MUTE});
  s.play_notes({A3, MUTE, MUTE, MUTE});
  s.play_notes({B3, MUTE, MUTE, MUTE});
  s.play_notes({C4, MUTE, MUTE, MUTE});

  //
}
