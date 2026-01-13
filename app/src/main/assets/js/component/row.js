import { Element } from '../core/element.js';
import { Size, Gravity } from '../core/constants.js';

export class Row extends Element {
    measure(pW, pH) {
        let totalW = 0, maxH = 0;
        const spacing = this.props.spacing || 0;
        this.children.forEach((child, i) => {
            child.measure(pW, pH);
            totalW += child.measuredWidth + (i > 0 ? spacing : 0);
            maxH = Math.max(maxH, child.measuredHeight);
        });
        if (this.props.width === Size.WRAP_CONTENT) this.measuredWidth = totalW;
        else super.measure(pW, pH);
        if (this.props.height === Size.WRAP_CONTENT) this.measuredHeight = maxH;
        else super.measure(pW, pH);
    }

    layout(x, y, w, h) {
        super.layout(x, y, w, h);
        let curX = this.absX;
        this.children.forEach(child => {
            let curY = this.absY;
            if (this.props.gravity & Gravity.CENTER_VERTICAL) {
                curY += (this.renderHeight - child.measuredHeight) / 2;
            }
            child.layout(curX, curY, child.measuredWidth, child.measuredHeight);
            curX += child.measuredWidth + (this.props.spacing || 0);
        });
    }
}