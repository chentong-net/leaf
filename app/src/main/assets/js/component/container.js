import { Element } from '../core/element.js';
import { Size, Gravity } from '../core/constants.js';

export class Container extends Element {
    measure(pW, pH) {
        const p2 = this.props.padding * 2;
        if (this.props.width === Size.WRAP_CONTENT || this.props.height === Size.WRAP_CONTENT) {
            let maxW = 0, maxH = 0;
            this.children.forEach(child => {
                child.measure(pW - p2, pH - p2);
                maxW = Math.max(maxW, child.measuredWidth);
                maxH = Math.max(maxH, child.measuredHeight);
            });
            if (this.props.width === Size.WRAP_CONTENT) this.measuredWidth = maxW + p2;
            if (this.props.height === Size.WRAP_CONTENT) this.measuredHeight = maxH + p2;
        }
        if (this.props.width !== Size.WRAP_CONTENT) super.measure(pW, pH);
        if (this.props.height !== Size.WRAP_CONTENT) {
            if (this.props.height === Size.MATCH_PARENT) this.measuredHeight = pH;
            else if (this.props.height >= 0) this.measuredHeight = this.props.height;
        }
    }

    layout(x, y, w, h) {
        super.layout(x, y, w, h);
        const p = this.props.padding;
        this.children.forEach(child => {
            child.measure(this.renderWidth - p * 2, this.renderHeight - p * 2);
            let cx = p, cy = p;
            const g = this.props.gravity;
            if (g & Gravity.CENTER_HORIZONTAL) cx = (this.renderWidth - child.measuredWidth) / 2;
            else if (g & Gravity.RIGHT) cx = this.renderWidth - child.measuredWidth - p;
            if (g & Gravity.CENTER_VERTICAL) cy = (this.renderHeight - child.measuredHeight) / 2;
            else if (g & Gravity.BOTTOM) cy = this.renderHeight - child.measuredHeight - p;
            child.layout(this.absX + cx, this.absY + cy, child.measuredWidth, child.measuredHeight);
        });
    }
}