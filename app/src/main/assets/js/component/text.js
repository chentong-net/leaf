import { Element } from '../core/element.js';
import { CMD, Size, Window } from '../core/constants.js';

export class Text extends Element {
    constructor(text, props = {}) {
        super({
            text: text,
            fontSize: 16,
            color: 0xFF000000, // 默认黑色
            ...props
        });
    }

    // 重写测量逻辑：根据文本内容决定大小
    measure(pW, pH) {
        const d = Window.DENSITY || 1.0;
        // 如果用户没指定宽高，则调用 Native 接口测量文本实际物理大小
        if (this.props.width === Size.WRAP_CONTENT || this.props.height === Size.WRAP_CONTENT) {
            const size = nativeMeasureText(this.props.text, this.props.fontSize * d);

            // 将物理像素转回逻辑像素存储
            if (this.props.width === Size.WRAP_CONTENT) {
                this.measuredWidth = size.width / d;
            } else {
                super.measure(pW, pH); // MATCH_PARENT 或 固定值
            }

            if (this.props.height === Size.WRAP_CONTENT) {
                this.measuredHeight = size.height / d;
            } else {
                super.measure(pW, pH);
            }
        } else {
            super.measure(pW, pH);
        }
    }

    paint() {
        const d = Window.DENSITY || 1.0;
        // 调用 C++ 的 CMD_TEXT (值为 2)
        nativeDraw(CMD.TEXT, {
            x: this.absX * d,
            y: this.absY * d,
            text: this.props.text,
            fontSize: this.props.fontSize * d,
            color: this.props.color
        });
    }
}