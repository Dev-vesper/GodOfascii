#pragma once
#include <string>
#include <vector>

class CharGrid;

// Development instrumentation: samples per-frame timings and output volume,
// shows a compact live overlay and dumps a summary on exit. Enable with
// ASCII3D_DEBUG=1; the dump path defaults to /tmp/ascii3d-stats.txt and can
// be overridden with ASCII3D_STATS. The same switch makes the Terminal
// backend log every input poll (raw key bytes, chord clocks) to
// /tmp/ascii3d-input.txt (ASCII3D_INPUT_LOG overrides).
class Diagnostics {
public:
    static bool enabled();

    void recordFrame(float ms);
    void recordUpdate(float ms);
    void recordRender(float ms);
    void recordPresent(float ms, size_t bytes);

    void drawOverlay(CharGrid& grid, int fps) const;
    void writeSummary(const std::string& path) const;

private:
    static float percentile(const std::vector<float>& v, float p);

    std::vector<float> frameMs_;
    std::vector<float> updateMs_;
    std::vector<float> renderMs_;
    std::vector<float> presentMs_;
    std::vector<size_t> presentBytes_;
};
