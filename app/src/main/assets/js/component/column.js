import { Element } from '../core/element.js';
import { Size, Gravity } from '../core/constants.js';

export class Column extends Element {
    measure(pW, pH) {
        let totalH = 0, maxW = 0;
        const spacing = this.props.spacing || 0;
        this.children.forEach((child, i) => {
            child.measure(pW, pH);
            totalH += child.measuredHeight + (i > 0 ? spacing : 0);
            maxW = Math.max(maxW, child.measuredWidth);
        });
        if (this.props.width === Size.WRAP_CONTENT) this.measuredWidth = maxW;
        else super.measure(pW, pH);
        if (this.props.height === Size.WRAP_CONTENT) this.measuredHeight = totalH;
        else super.measure(pW, pH);
    }

    layout(x, y, w, h) {
        super.layout(x, y, w, h);
        let curY = this.absY;
        this.children.forEach(child => {
            let curX = this.absX;
            if (this.props.gravity & Gravity.CENTER_HORIZONTAL) {
                curX += (this.renderWidth - child.measuredWidth) / 2;
            }
            child.layout(curX, curY, child.measuredWidth, child.measuredHeight);
            curY += child.measuredHeight + (this.props.spacing || 0);
        });
    }
}