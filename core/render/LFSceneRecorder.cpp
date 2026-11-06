#include "render/LFSceneRecorder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace {

constexpr float kAntialiasMargin = 2.0f;
constexpr float kDamageMergeMargin = 1.0f;
constexpr float kFullRedrawCoverage = 0.45f;
constexpr size_t kMaximumDamageRects = 8;

enum class CommandType : uint8_t {
    PushState,
    PopState,
    Translate,
    Rotate,
    Scale,
    SetOpacity,
    ClipRect,
    DrawRect,
    DrawRoundedRect,
    StrokeRect,
    StrokeRoundedRect,
    DrawLine,
    DrawText,
    DrawImage,
    DrawShadow,
};

struct SceneState {
    std::array<float, 6> transform{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    LFDrawRect clip;
    float opacity = 1.0f;
};

struct SceneCommand {
    CommandType type = CommandType::PushState;
    SceneState state;
    LFDrawRect bounds;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
    float radius = 0.0f;
    float strokeWidth = 0.0f;
    uint32_t color = 0;
    LFStrokeCap strokeCap = LFStrokeCap::Butt;

    LFFontHandle font = 0;
    std::string text;
    float fontSize = 16.0f;
    float lineHeight = 1.0f;
    LFTextOrigin textOrigin = LFTextOrigin::Baseline;
    bool italic = false;

    LFImageDraw image;
    LFShadowDraw shadow;
};

bool isFinite(float value) {
    return std::isfinite(value);
}

bool isEmpty(const LFDrawRect& rect) {
    return !isFinite(rect.x) || !isFinite(rect.y) ||
           !isFinite(rect.width) || !isFinite(rect.height) ||
           rect.width <= 0.0f || rect.height <= 0.0f;
}

LFDrawRect normalized(const LFDrawRect& rect) {
    if (!isFinite(rect.x) || !isFinite(rect.y) ||
        !isFinite(rect.width) || !isFinite(rect.height)) {
        return {};
    }
    const float x0 = std::min(rect.x, rect.x + rect.width);
    const float y0 = std::min(rect.y, rect.y + rect.height);
    const float x1 = std::max(rect.x, rect.x + rect.width);
    const float y1 = std::max(rect.y, rect.y + rect.height);
    return {x0, y0, x1 - x0, y1 - y0};
}

LFDrawRect inset(const LFDrawRect& rect, float amount) {
    return {
        rect.x + amount,
        rect.y + amount,
        rect.width - amount * 2.0f,
        rect.height - amount * 2.0f,
    };
}

LFDrawRect inflate(const LFDrawRect& rect, float amount) {
    return {
        rect.x - amount,
        rect.y - amount,
        rect.width + amount * 2.0f,
        rect.height + amount * 2.0f,
    };
}

bool intersects(const LFDrawRect& left, const LFDrawRect& right) {
    return left.x < right.x + right.width &&
           left.x + left.width > right.x &&
           left.y < right.y + right.height &&
           left.y + left.height > right.y;
}

bool touchesOrOverlaps(const LFDrawRect& left, const LFDrawRect& right) {
    return left.x <= right.x + right.width + kDamageMergeMargin &&
           left.x + left.width + kDamageMergeMargin >= right.x &&
           left.y <= right.y + right.height + kDamageMergeMargin &&
           left.y + left.height + kDamageMergeMargin >= right.y;
}

LFDrawRect intersection(const LFDrawRect& left, const LFDrawRect& right) {
    const float x0 = std::max(left.x, right.x);
    const float y0 = std::max(left.y, right.y);
    const float x1 = std::min(left.x + left.width, right.x + right.width);
    const float y1 = std::min(left.y + left.height, right.y + right.height);
    if (x1 <= x0 || y1 <= y0) {
        return {};
    }
    return {x0, y0, x1 - x0, y1 - y0};
}

LFDrawRect unionRect(const LFDrawRect& left, const LFDrawRect& right) {
    if (isEmpty(left)) return right;
    if (isEmpty(right)) return left;
    const float x0 = std::min(left.x, right.x);
    const float y0 = std::min(left.y, right.y);
    const float x1 = std::max(left.x + left.width, right.x + right.width);
    const float y1 = std::max(left.y + left.height, right.y + right.height);
    return {x0, y0, x1 - x0, y1 - y0};
}

void multiply(std::array<float, 6>& left, const std::array<float, 6>& right) {
    const float m0 = left[0] * right[0] + left[1] * right[2];
    const float m2 = left[2] * right[0] + left[3] * right[2];
    const float m4 = left[4] * right[0] + left[5] * right[2] + right[4];
    left[1] = left[0] * right[1] + left[1] * right[3];
    left[3] = left[2] * right[1] + left[3] * right[3];
    left[5] = left[4] * right[1] + left[5] * right[3] + right[5];
    left[0] = m0;
    left[2] = m2;
    left[4] = m4;
}

void premultiply(std::array<float, 6>& transform,
                 const std::array<float, 6>& operation) {
    auto result = operation;
    multiply(result, transform);
    transform = result;
}

void translateTransform(std::array<float, 6>& transform, float x, float y) {
    premultiply(transform, {1.0f, 0.0f, 0.0f, 1.0f, x, y});
}

void scaleTransform(std::array<float, 6>& transform, float x, float y) {
    premultiply(transform, {x, 0.0f, 0.0f, y, 0.0f, 0.0f});
}

void rotateTransform(std::array<float, 6>& transform, float degrees) {
    const float radians = degrees * 0.017453292519943295769f;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    premultiply(transform, {cosine, sine, -sine, cosine, 0.0f, 0.0f});
}

LFDrawRect transformBounds(const std::array<float, 6>& transform,
                           const LFDrawRect& rect) {
    if (isEmpty(rect)) {
        return {};
    }
    const std::array<std::pair<float, float>, 4> points{{
        {rect.x, rect.y},
        {rect.x + rect.width, rect.y},
        {rect.x, rect.y + rect.height},
        {rect.x + rect.width, rect.y + rect.height},
    }};

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    for (const auto& [x, y] : points) {
        const float transformedX = transform[0] * x + transform[2] * y + transform[4];
        const float transformedY = transform[1] * x + transform[3] * y + transform[5];
        minX = std::min(minX, transformedX);
        minY = std::min(minY, transformedY);
        maxX = std::max(maxX, transformedX);
        maxY = std::max(maxY, transformedY);
    }
    return {minX, minY, maxX - minX, maxY - minY};
}

LFDrawRect visibleBounds(const SceneState& state, const LFDrawRect& bounds) {
    return intersection(transformBounds(state.transform, bounds), state.clip);
}

bool sameRect(const LFDrawRect& left, const LFDrawRect& right) {
    return left.x == right.x && left.y == right.y &&
           left.width == right.width && left.height == right.height;
}

bool sameState(const SceneState& left, const SceneState& right) {
    return left.transform == right.transform &&
           sameRect(left.clip, right.clip) &&
           left.opacity == right.opacity;
}

bool sameVisual(const SceneCommand& left, const SceneCommand& right) {
    if (left.type != right.type ||
        !sameState(left.state, right.state) ||
        !sameRect(left.bounds, right.bounds)) {
        return false;
    }

    switch (left.type) {
        case CommandType::DrawRect:
        case CommandType::DrawRoundedRect:
        case CommandType::StrokeRect:
        case CommandType::StrokeRoundedRect:
            return left.x == right.x && left.y == right.y &&
                   left.z == right.z && left.w == right.w &&
                   left.radius == right.radius &&
                   left.strokeWidth == right.strokeWidth &&
                   left.color == right.color;
        case CommandType::DrawLine:
            return left.x == right.x && left.y == right.y &&
                   left.z == right.z && left.w == right.w &&
                   left.strokeWidth == right.strokeWidth &&
                   left.strokeCap == right.strokeCap &&
                   left.color == right.color;
        case CommandType::DrawText:
            return left.font == right.font &&
                   left.text == right.text &&
                   left.x == right.x && left.y == right.y &&
                   left.fontSize == right.fontSize &&
                   left.lineHeight == right.lineHeight &&
                   left.color == right.color &&
                   left.textOrigin == right.textOrigin &&
                   left.italic == right.italic;
        case CommandType::DrawImage:
            return left.image.texture == right.image.texture &&
                   left.image.contentVersion == right.image.contentVersion &&
                   sameRect(left.image.imageRect, right.image.imageRect) &&
                   sameRect(left.image.clipRect, right.image.clipRect) &&
                   left.image.radius == right.image.radius;
        case CommandType::DrawShadow:
            return sameRect(left.shadow.rect, right.shadow.rect) &&
                   left.shadow.radius == right.shadow.radius &&
                   left.shadow.offsetX == right.shadow.offsetX &&
                   left.shadow.offsetY == right.shadow.offsetY &&
                   left.shadow.blur == right.shadow.blur &&
                   left.shadow.spread == right.shadow.spread &&
                   left.shadow.color == right.shadow.color;
        default:
            return true;
    }
}

LFDrawRect rectForCommand(const SceneCommand& command) {
    switch (command.type) {
        case CommandType::DrawRect:
        case CommandType::DrawRoundedRect:
            return inflate({command.x, command.y, command.z, command.w},
                           kAntialiasMargin);
        case CommandType::StrokeRect:
        case CommandType::StrokeRoundedRect:
            return inflate(
                inflate({command.x, command.y, command.z, command.w},
                        command.strokeWidth * 0.5f),
                kAntialiasMargin
            );
        case CommandType::DrawLine: {
            const float x0 = std::min(command.x, command.z);
            const float y0 = std::min(command.y, command.w);
            const float x1 = std::max(command.x, command.z);
            const float y1 = std::max(command.y, command.w);
            return inflate(
                inflate({x0, y0, x1 - x0, y1 - y0},
                        command.strokeWidth * 0.5f),
                kAntialiasMargin
            );
        }
        case CommandType::DrawText:
            return inflate(command.bounds, kAntialiasMargin);
        case CommandType::DrawImage:
            return inflate(command.image.clipRect, kAntialiasMargin);
        case CommandType::DrawShadow: {
            const float reach = std::max(0.0f, command.shadow.blur) +
                                std::max(0.0f, command.shadow.spread);
            const float x0 = std::min(command.shadow.rect.x,
                                      command.shadow.rect.x + command.shadow.offsetX);
            const float y0 = std::min(command.shadow.rect.y,
                                      command.shadow.rect.y + command.shadow.offsetY);
            const float x1 = std::max(command.shadow.rect.x + command.shadow.rect.width,
                                      command.shadow.rect.x + command.shadow.offsetX +
                                          command.shadow.rect.width);
            const float y1 = std::max(command.shadow.rect.y + command.shadow.rect.height,
                                      command.shadow.rect.y + command.shadow.offsetY +
                                          command.shadow.rect.height);
            return inflate({x0, y0, x1 - x0, y1 - y0},
                           reach + kAntialiasMargin);
        }
        default:
            return {};
    }
}

void addDamageRect(LFDamageRegion& damage,
                   const LFDrawRect& source,
                   const LFDrawRect& viewport) {
    LFDrawRect rect = intersection(source, viewport);
    if (isEmpty(rect)) {
        return;
    }

    for (size_t index = 0; index < damage.rects.size();) {
        if (!touchesOrOverlaps(damage.rects[index], rect)) {
            ++index;
            continue;
        }
        rect = unionRect(damage.rects[index], rect);
        damage.rects.erase(damage.rects.begin() + static_cast<std::ptrdiff_t>(index));
        index = 0;
    }
    damage.rects.push_back(rect);

    if (damage.rects.size() <= kMaximumDamageRects) {
        return;
    }
    LFDrawRect combined = damage.rects.front();
    for (size_t index = 1; index < damage.rects.size(); ++index) {
        combined = unionRect(combined, damage.rects[index]);
    }
    damage.rects = {combined};
}

LFDrawRect damageBoundsFor(const SceneCommand& command) {
    return intersection(command.bounds, command.state.clip);
}

void replayCommand(LFPaint& target, const SceneCommand& command) {
    switch (command.type) {
        case CommandType::PushState:
            target.pushState();
            break;
        case CommandType::PopState:
            target.popState();
            break;
        case CommandType::Translate:
            target.translate(command.x, command.y);
            break;
        case CommandType::Rotate:
            target.rotate(command.x);
            break;
        case CommandType::Scale:
            target.scale(command.x, command.y);
            break;
        case CommandType::SetOpacity:
            target.setOpacity(command.x);
            break;
        case CommandType::ClipRect:
            target.clipRect({command.x, command.y, command.z, command.w});
            break;
        case CommandType::DrawRect:
            target.drawRect({command.x, command.y, command.z, command.w}, command.color);
            break;
        case CommandType::DrawRoundedRect:
            target.drawRoundedRect(
                {command.x, command.y, command.z, command.w},
                command.radius,
                command.color
            );
            break;
        case CommandType::StrokeRect:
            target.strokeRect(
                {command.x, command.y, command.z, command.w},
                command.color,
                command.strokeWidth
            );
            break;
        case CommandType::StrokeRoundedRect:
            target.strokeRoundedRect(
                {command.x, command.y, command.z, command.w},
                command.radius,
                command.color,
                command.strokeWidth
            );
            break;
        case CommandType::DrawLine:
            target.drawLine(
                command.x,
                command.y,
                command.z,
                command.w,
                command.color,
                command.strokeWidth,
                command.strokeCap
            );
            break;
        case CommandType::DrawText: {
            LFTextDraw text;
            text.font = command.font;
            text.text = command.text;
            text.x = command.x;
            text.y = command.y;
            text.visualBounds = command.bounds;
            text.fontSize = command.fontSize;
            text.lineHeight = command.lineHeight;
            text.color = command.color;
            text.origin = command.textOrigin;
            text.italic = command.italic;
            target.drawText(text);
            break;
        }
        case CommandType::DrawImage:
            target.drawImage(command.image);
            break;
        case CommandType::DrawShadow:
            target.drawShadow(command.shadow);
            break;
    }
}

bool isDrawCommand(CommandType type) {
    return type >= CommandType::DrawRect;
}

} // namespace

struct LFSceneRecorder::Impl {
    float width = 0.0f;
    float height = 0.0f;
    float pixelRatio = 1.0f;
    uint32_t clearColor = 0;
    bool hasFrame = false;

    SceneState state;
    std::vector<SceneState> stateStack;
    std::vector<SceneCommand> commands;
    std::vector<size_t> drawIndices;

    void resetDrawState() {
        state = {};
        state.clip = {0.0f, 0.0f, width, height};
        stateStack.clear();
    }

    void addStateCommand(CommandType type) {
        SceneCommand command;
        command.type = type;
        commands.push_back(std::move(command));
    }

    void addDrawCommand(SceneCommand command) {
        command.state = state;
        command.bounds = visibleBounds(state, rectForCommand(command));
        const size_t index = commands.size();
        commands.push_back(std::move(command));
        if (!isEmpty(commands[index].bounds)) {
            drawIndices.push_back(index);
        }
    }
};

LFSceneRecorder::LFSceneRecorder()
    : m_impl(std::make_unique<Impl>()) {
}

LFSceneRecorder::~LFSceneRecorder() = default;

LFSceneRecorder::LFSceneRecorder(LFSceneRecorder&&) noexcept = default;

LFSceneRecorder& LFSceneRecorder::operator=(LFSceneRecorder&&) noexcept = default;

void LFSceneRecorder::beginFrame(float width, float height, float pixelRatio,
                                 uint32_t clearColor) {
    m_impl->width = width;
    m_impl->height = height;
    m_impl->pixelRatio = pixelRatio;
    m_impl->clearColor = clearColor;
    m_impl->hasFrame = true;
    m_impl->commands.clear();
    m_impl->drawIndices.clear();
    m_impl->resetDrawState();
}

LFDamageRegion LFSceneRecorder::diff(const LFSceneRecorder& previous) const {
    LFDamageRegion damage;
    if (!m_impl->hasFrame || !previous.m_impl->hasFrame ||
        m_impl->width != previous.m_impl->width ||
        m_impl->height != previous.m_impl->height ||
        m_impl->pixelRatio != previous.m_impl->pixelRatio ||
        m_impl->clearColor != previous.m_impl->clearColor) {
        damage.full = true;
        damage.rects.push_back({0.0f, 0.0f, m_impl->width, m_impl->height});
        return damage;
    }

    const LFDrawRect viewport{0.0f, 0.0f, m_impl->width, m_impl->height};
    const auto& previousDraws = previous.m_impl->drawIndices;
    const auto& currentDraws = m_impl->drawIndices;
    const auto& previousCommands = previous.m_impl->commands;
    const auto& currentCommands = m_impl->commands;

    size_t prefix = 0;
    while (prefix < previousDraws.size() && prefix < currentDraws.size() &&
           sameVisual(previousCommands[previousDraws[prefix]],
                      currentCommands[currentDraws[prefix]])) {
        ++prefix;
    }

    size_t suffix = 0;
    while (suffix < previousDraws.size() - prefix &&
           suffix < currentDraws.size() - prefix) {
        const size_t previousIndex = previousDraws.size() - 1 - suffix;
        const size_t currentIndex = currentDraws.size() - 1 - suffix;
        if (!sameVisual(previousCommands[previousDraws[previousIndex]],
                        currentCommands[currentDraws[currentIndex]])) {
            break;
        }
        ++suffix;
    }

    if (previousDraws.size() == currentDraws.size()) {
        for (size_t index = prefix; index < previousDraws.size() - suffix; ++index) {
            const SceneCommand& before = previousCommands[previousDraws[index]];
            const SceneCommand& after = currentCommands[currentDraws[index]];
            if (!sameVisual(before, after)) {
                addDamageRect(damage, damageBoundsFor(before), viewport);
                addDamageRect(damage, damageBoundsFor(after), viewport);
            }
        }
    } else {
        for (size_t index = prefix; index < previousDraws.size() - suffix; ++index) {
            addDamageRect(
                damage,
                damageBoundsFor(previousCommands[previousDraws[index]]),
                viewport
            );
        }
        for (size_t index = prefix; index < currentDraws.size() - suffix; ++index) {
            addDamageRect(
                damage,
                damageBoundsFor(currentCommands[currentDraws[index]]),
                viewport
            );
        }
    }

    if (damage.rects.empty()) {
        return damage;
    }

    float coverage = 0.0f;
    for (const LFDrawRect& rect : damage.rects) {
        coverage += std::max(0.0f, rect.width) * std::max(0.0f, rect.height);
    }
    const float viewportArea = std::max(1.0f, viewport.width * viewport.height);
    if (coverage >= viewportArea * kFullRedrawCoverage) {
        damage.full = true;
        damage.rects = {viewport};
    }
    return damage;
}

void LFSceneRecorder::replay(LFPaint& target, const LFDrawRect& damage) const {
    for (const SceneCommand& command : m_impl->commands) {
        if (isDrawCommand(command.type) && !intersects(command.bounds, damage)) {
            continue;
        }
        replayCommand(target, command);
    }
}

void LFSceneRecorder::pushState() {
    m_impl->addStateCommand(CommandType::PushState);
    m_impl->stateStack.push_back(m_impl->state);
}

void LFSceneRecorder::popState() {
    m_impl->addStateCommand(CommandType::PopState);
    if (!m_impl->stateStack.empty()) {
        m_impl->state = m_impl->stateStack.back();
        m_impl->stateStack.pop_back();
    }
}

void LFSceneRecorder::translate(float x, float y) {
    SceneCommand command;
    command.type = CommandType::Translate;
    command.x = x;
    command.y = y;
    m_impl->commands.push_back(std::move(command));
    translateTransform(m_impl->state.transform, x, y);
}

void LFSceneRecorder::rotate(float degrees) {
    SceneCommand command;
    command.type = CommandType::Rotate;
    command.x = degrees;
    m_impl->commands.push_back(std::move(command));
    rotateTransform(m_impl->state.transform, degrees);
}

void LFSceneRecorder::scale(float x, float y) {
    SceneCommand command;
    command.type = CommandType::Scale;
    command.x = x;
    command.y = y;
    m_impl->commands.push_back(std::move(command));
    scaleTransform(m_impl->state.transform, x, y);
}

void LFSceneRecorder::setOpacity(float opacity) {
    SceneCommand command;
    command.type = CommandType::SetOpacity;
    command.x = opacity;
    m_impl->commands.push_back(std::move(command));
    m_impl->state.opacity = std::clamp(opacity, 0.0f, 1.0f);
}

void LFSceneRecorder::clipRect(const LFDrawRect& rect) {
    SceneCommand command;
    command.type = CommandType::ClipRect;
    command.x = rect.x;
    command.y = rect.y;
    command.z = rect.width;
    command.w = rect.height;
    m_impl->commands.push_back(std::move(command));

    const LFDrawRect transformed = transformBounds(
        m_impl->state.transform,
        normalized(rect)
    );
    m_impl->state.clip = intersection(m_impl->state.clip, transformed);
}

void LFSceneRecorder::drawRect(const LFDrawRect& rect, uint32_t color) {
    SceneCommand command;
    command.type = CommandType::DrawRect;
    command.x = rect.x;
    command.y = rect.y;
    command.z = rect.width;
    command.w = rect.height;
    command.color = color;
    m_impl->addDrawCommand(std::move(command));
}

void LFSceneRecorder::drawRoundedRect(const LFDrawRect& rect, float radius,
                                      uint32_t color) {
    SceneCommand command;
    command.type = CommandType::DrawRoundedRect;
    command.x = rect.x;
    command.y = rect.y;
    command.z = rect.width;
    command.w = rect.height;
    command.radius = radius;
    command.color = color;
    m_impl->addDrawCommand(std::move(command));
}

void LFSceneRecorder::strokeRect(const LFDrawRect& rect, uint32_t color,
                                 float width) {
    SceneCommand command;
    command.type = CommandType::StrokeRect;
    command.x = rect.x;
    command.y = rect.y;
    command.z = rect.width;
    command.w = rect.height;
    command.strokeWidth = width;
    command.color = color;
    m_impl->addDrawCommand(std::move(command));
}

void LFSceneRecorder::strokeRoundedRect(const LFDrawRect& rect, float radius,
                                        uint32_t color, float width) {
    SceneCommand command;
    command.type = CommandType::StrokeRoundedRect;
    command.x = rect.x;
    command.y = rect.y;
    command.z = rect.width;
    command.w = rect.height;
    command.radius = radius;
    command.strokeWidth = width;
    command.color = color;
    m_impl->addDrawCommand(std::move(command));
}

void LFSceneRecorder::drawLine(float x1, float y1, float x2, float y2,
                               uint32_t color, float width, LFStrokeCap cap) {
    SceneCommand command;
    command.type = CommandType::DrawLine;
    command.x = x1;
    command.y = y1;
    command.z = x2;
    command.w = y2;
    command.color = color;
    command.strokeWidth = width;
    command.strokeCap = cap;
    m_impl->addDrawCommand(std::move(command));
}

void LFSceneRecorder::drawText(const LFTextDraw& text) {
    SceneCommand command;
    command.type = CommandType::DrawText;
    command.font = text.font;
    command.text.assign(text.text.data(), text.text.size());
    command.x = text.x;
    command.y = text.y;
    command.fontSize = text.fontSize;
    command.lineHeight = text.lineHeight;
    command.color = text.color;
    command.textOrigin = text.origin;
    command.italic = text.italic;
    command.bounds = text.visualBounds;
    m_impl->addDrawCommand(std::move(command));
}

void LFSceneRecorder::drawImage(const LFImageDraw& image) {
    SceneCommand command;
    command.type = CommandType::DrawImage;
    command.image = image;
    m_impl->addDrawCommand(std::move(command));
}

void LFSceneRecorder::drawShadow(const LFShadowDraw& shadow) {
    SceneCommand command;
    command.type = CommandType::DrawShadow;
    command.shadow = shadow;
    m_impl->addDrawCommand(std::move(command));
}
