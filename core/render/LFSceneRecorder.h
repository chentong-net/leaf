#ifndef LEAF_LFSCENERECORDER_H
#define LEAF_LFSCENERECORDER_H

#include "render/LFPaint.h"

#include <cstdint>
#include <memory>
#include <vector>

struct LFDamageRegion {
    bool full = false;
    std::vector<LFDrawRect> rects;
};

/**
 * Records backend-independent paint commands and compares the visible draw
 * list with a previous frame.
 */
class LFSceneRecorder final : public LFPaint {
public:
    LFSceneRecorder();
    ~LFSceneRecorder() override;

    LFSceneRecorder(const LFSceneRecorder&) = delete;
    LFSceneRecorder& operator=(const LFSceneRecorder&) = delete;
    LFSceneRecorder(LFSceneRecorder&&) noexcept;
    LFSceneRecorder& operator=(LFSceneRecorder&&) noexcept;

    void beginFrame(float width, float height, float pixelRatio, uint32_t clearColor);
    LFDamageRegion diff(const LFSceneRecorder& previous) const;
    void replay(LFPaint& target, const LFDrawRect& damage) const;

    void pushState() override;
    void popState() override;

    void translate(float x, float y) override;
    void rotate(float degrees) override;
    void scale(float x, float y) override;
    void setOpacity(float opacity) override;
    void clipRect(const LFDrawRect& rect) override;

    void drawRect(const LFDrawRect& rect, uint32_t color) override;
    void drawRoundedRect(const LFDrawRect& rect, float radius, uint32_t color) override;
    void strokeRect(const LFDrawRect& rect, uint32_t color, float width) override;
    void strokeRoundedRect(const LFDrawRect& rect, float radius,
                           uint32_t color, float width) override;
    void drawLine(float x1, float y1, float x2, float y2,
                  uint32_t color, float width,
                  LFStrokeCap cap = LFStrokeCap::Butt) override;

    void drawText(const LFTextDraw& text) override;
    void drawImage(const LFImageDraw& image) override;
    void drawShadow(const LFShadowDraw& shadow) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // LEAF_LFSCENERECORDER_H
