#include "game/Diagnostics.h"
#include "render/CharGrid.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace {
constexpr Rgb kDbgFg{120, 255, 120};
constexpr Rgb kDbgBg{10, 12, 10};
}  // namespace

bool Diagnostics::enabled() {
    const char* v = std::getenv("ASCII3D_DEBUG");
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}

void Diagnostics::recordFrame(float ms) { frameMs_.push_back(ms); }
void Diagnostics::recordUpdate(float ms) { updateMs_.push_back(ms); }
void Diagnostics::recordRender(float ms) { renderMs_.push_back(ms); }
void Diagnostics::recordPresent(float ms, size_t bytes) {
    presentMs_.push_back(ms);
    presentBytes_.push_back(bytes);
}

float Diagnostics::percentile(const std::vector<float>& v, float p) {
    if (v.empty()) return 0.0f;
    std::vector<float> s(v);
    std::sort(s.begin(), s.end());
    const size_t i = std::min(s.size() - 1,
                              static_cast<size_t>(p * (s.size() - 1)));
    return s[i];
}

void Diagnostics::drawOverlay(CharGrid& grid, int fps) const {
    if (frameMs_.empty()) return;
    const float f = frameMs_.back();
    const float u = updateMs_.empty() ? 0.0f : updateMs_.back();
    const float r = renderMs_.empty() ? 0.0f : renderMs_.back();
    const float p = presentMs_.empty() ? 0.0f : presentMs_.back();
    const size_t b = presentBytes_.empty() ? 0 : presentBytes_.back();
    char buf[80];
    std::snprintf(buf, sizeof buf, "%dfps f%.1f u%.2f r%.2f p%.2f %zuB",
                  fps, f, u, r, p, b);
    grid.setText(1, 1, buf, kDbgFg, kDbgBg);
}

void Diagnostics::writeSummary(const std::string& path) const {
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (f == nullptr) return;

    if (frameMs_.empty()) {
        std::fprintf(f, "no frames recorded\n");
        std::fclose(f);
        return;
    }

    float sum = 0.0f;
    float worst = 0.0f;
    for (float v : frameMs_) {
        sum += v;
        worst = std::max(worst, v);
    }
    const float avg = sum / frameMs_.size();
    const float fpsAvg = 1000.0f / std::max(avg, 0.001f);
    const float fpsWorst = 1000.0f / std::max(worst, 0.001f);

    const auto section = [&](const char* name, const std::vector<float>& v) {
        if (v.empty()) return;
        float s = 0.0f;
        float w = 0.0f;
        for (float t : v) {
            s += t;
            w = std::max(w, t);
        }
        std::fprintf(f, "%-8s avg %.3f  p50 %.3f  p99 %.3f  max %.3f ms\n",
                     name, s / v.size(), percentile(v, 0.50f),
                     percentile(v, 0.99f), w);
    };

    std::time_t now = std::time(nullptr);
    std::fprintf(f, "ascii3d stats %s", std::ctime(&now));
    std::fprintf(f, "frames %zu  span %.1fs\n", frameMs_.size(),
                 sum / 1000.0f);
    std::fprintf(f, "fps avg %.1f  worst-instant %.1f\n", fpsAvg, fpsWorst);
    std::fprintf(f, "frame ms avg %.2f  p50 %.2f  p90 %.2f  p99 %.2f  max %.2f\n\n",
                 avg, percentile(frameMs_, 0.50f), percentile(frameMs_, 0.90f),
                 percentile(frameMs_, 0.99f), worst);
    section("update", updateMs_);
    section("render", renderMs_);
    section("present", presentMs_);

    if (!presentBytes_.empty()) {
        size_t s = 0;
        size_t w = 0;
        for (size_t v : presentBytes_) {
            s += v;
            w = std::max(w, v);
        }
        std::fprintf(f, "\noutput avg %.1f KB/frame  max %.1f KB  total %.1f MB\n",
                     static_cast<double>(s) / presentBytes_.size() / 1024.0,
                     static_cast<double>(w) / 1024.0,
                     static_cast<double>(s) / 1024.0 / 1024.0);
    }
    std::fclose(f);
}

float Diagnostics::worstFrameMs() const {
    return percentile(frameMs_, 1.0f);
}
