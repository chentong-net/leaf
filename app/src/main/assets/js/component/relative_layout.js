import { Element } from '../core/element.js';
import { Size, Gravity } from '../core/constants.js';

export class RelativeLayout extends Element {
    measure(pW, pH) {
        super.measure(pW, pH);
        if (this.props.width === Size.WRAP_CONTENT || this.props.height === Size.WRAP_CONTENT) {
            let maxW = 0, maxH = 0;
            this.children.forEach(child => {
                child.measure(pW, pH);
                maxW = Math.max(maxW, child.measuredWidth + (child.props.left || 0));
                maxH = Math.max(maxH, child.measuredHeight + (child.props.top || 0));
            });
            if (this.props.width === Size.WRAP_CONTENT) this.measuredWidth = maxW;
            if (this.props.height === Size.WRAP_CONTENT) this.measuredHeight = maxH;
        }
    }

    layout(x, y, w, h) {
        super.layout(x, y, w, h);
        this.children.forEach(child => {
            child.measure(this.renderWidth, this.renderHeight);
            let cx = this.absX + (child.props.left || 0);
            let cy = this.absY + (child.props.top || 0);
            if (child.props.right !== undefined) cx = this.absX + this.renderWidth - child.measuredWidth - child.props.right;
            if (child.props.bottom !== undefined) cy = this.absY + this.renderHeight - child.measuredHeight - child.props.bottom;
            child.layout(cx, cy, child.measuredWidth, child.measuredHeight);
        });
    }
}