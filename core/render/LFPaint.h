#ifndef LEAF_LFPAINT_H
#define LEAF_LFPAINT_H

#include "render/LFRenderTypes.h"

#include <cstdint>

/**
 * 当前渲染帧使用的后端无关绘制接口。
 */
class LFPaint {
public:
    virtual ~LFPaint() = default;

    virtual void pushState() = 0;
    virtual void popState() = 0;

    virtual void translate(float x, float y) = 0;
    virtual void rotate(float degrees) = 0;
    virtual void scale(float x, float y) = 0;
    virtual void setOpacity(float opacity) = 0;
    virtual void clipRect(const LFDrawRect& rect) = 0;

    virtual void drawRect(const LFDrawRect& rect, uint32_t color) = 0;
    virtual void drawRoundedRect(const LFDrawRect& rect, float radius, uint32_t color) = 0;
    virtual void strokeRect(const LFDrawRect& rect, uint32_t color, float width) = 0;
    virtual void strokeRoundedRect(const LFDrawRect& rect, float radius,
                                   uint32_t color, float width) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2,
                          uint32_t color, float width,
                          LFStrokeCap cap = LFStrokeCap::Butt) = 0;

    virtual void drawText(const LFTextDraw& text) = 0;
    virtual void drawImage(const LFImageDraw& image) = 0;
    virtual void drawShadow(const LFShadowDraw& shadow) = 0;
};

#endif // LEAF_LFPAINT_H
