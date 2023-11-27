export module beeps;
import nessa;
import rng;
import siaudio;
import sitime;

using namespace nessa::midi;

export class beeps : siaudio::os_streamer {
  static constexpr const auto frate = static_cast<float>(rate);

  sitime::stopwatch m_watch;
  float m_walk{-1};
  float m_walk_dt{-1};
  float m_eat{-1};
  float m_eat_dt{1};

  float now() noexcept { return m_watch.millis() / 1000.0f; }

  constexpr float clamp(float n) const noexcept {
    return n < 0.0 ? 0.0 : (n > 1.0 ? 1.0 : n);
  }
  constexpr float eat(float t) const noexcept {
    if (m_eat < 0)
      return 0;

    float n = t - m_eat;
    n = n * 3.0;
    n = -4 * n * n + 4 * n;
    n = clamp(n);
    n = n * 0.3;
    return n * nessa::gen::triangle(m_eat_dt * t * nessa::midi::note_freq(C4));
  }
  constexpr float walk(float t) const noexcept {
    if (m_walk < 0)
      return 0;

    float n = t - m_walk;
    n = n * 6.0;
    n = 1.0 / (10.0 * n);
    n = clamp(n);
    n = n * 0.05;
    return n * nessa::gen::noise(m_walk_dt * t * nessa::midi::note_freq(C6));
  }
  constexpr float vol_at(float t) const noexcept {
    auto e = eat(t);
    auto w = walk(t);
    return (e + w) / 2.0;
  }

  void fill_buffer(float *buf, unsigned len) override {
    auto ref = now();
    for (auto i = 0; i < len; ++i) {
      *buf++ = vol_at(ref + i / frate);
    }
  }

public:
  void reset() {
    m_eat = -1;
    m_walk = -1;
  }

  void eat() {
    m_eat = now();
    m_eat_dt = 1.0 + 0.1 * rng::randf();
  }
  void walk() {
    m_walk = now();
    m_walk_dt = 1.0 + 0.1 * rng::randf();
  }
};
