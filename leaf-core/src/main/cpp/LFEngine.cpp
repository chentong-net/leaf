#define NANOVG_GLES3_IMPLEMENTATION

#include "LFEngine.h"
#include "LFFlex.h"
#include "LFLinear.h"

static LFRenderNode::Ptr g_rootNode = nullptr;
static NVGcontext *g_vg = nullptr;
static float g_width = 0, g_height = 0, g_density = 1.0f;

extern "C" {

/**
 * 引擎初始化
 * 此函数必须在 GL 线程调用
 */
void leaf_init(std::function<std::string(const char *path)> loader) {
    // 1. 初始化 NanoVG 上下文
    // 开启抗锯齿和模板纹理支持
    g_vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (g_vg == nullptr) {
        LF_LOGI("Could not init nanovg GLES3");
        return;
    }

    // 2. 创建并初始化根节点
    auto root = std::make_shared<LFFlex>();
    root->setBackgroundColor(0xFFFFFFFF); // 默认白色底色
    g_rootNode = root;

    // 3. 将根节点绑定到 LFEngine 单例，供后续渲染循环使用
    LFEngine::getInstance().setRootNode(g_rootNode);

    LF_LOGI("Leaf Engine initialized successfully");
}

/**
 * 屏幕尺寸变更同步
 */
void leaf_update_size(int w, int h, float d) {
    g_width = (float) w;
    g_height = (float) h;
    g_density = d;
}

/**
 * 每帧渲染逻辑
 */
void leaf_render() {
    if (!g_vg || !g_rootNode) return;

    // 1. 布局计算：从单例触发影子树的 Yoga 布局计算
    // 使用像素进行约束计算
    LFEngine::getInstance().update(g_width, g_height);

    // 2. 执行渲染
    // 清除缓冲区通常由 Java 层处理，这里负责绘制 UI 内容
    nvgBeginFrame(g_vg, g_width, g_height, g_density);

    // 递归绘制树中的所有节点
    LFEngine::getInstance().render(g_vg);

    nvgEndFrame(g_vg);
}

/**
 * 模拟JS构建页面
 */
void leaf_eval_js(const char *code) {
    if (!g_rootNode) return;

    // 创建一个垂直线性布局容器
    auto list = LFLinear::createVertical();

    list->setMargin(YGEdgeAll, 10);
    list->setGap(10);

    // 创建子块 A (红色)
    auto box1 = std::make_shared<LFFlex>();
    box1->setWidth(100);
    box1->setHeight(100);
    box1->setBackgroundColor(0xFFFF0000);

    // 创建子块 B (蓝色)
    auto box2 = std::make_shared<LFFlex>();
    box2->setWidth(100);
    box2->setHeight(100);
    box2->setBackgroundColor(0xFF0000FF);

    // 组装树
    list->addChild(box1);
    list->addChild(box2);
    g_rootNode->addChild(list);

    LF_LOGI("Build UI done");
}

}