import {CMD, Size, Gravity, Window } from './constants.js';

export class Element {
    constructor(props = {}) {
        this.props = {
            width: Size.WRAP_CONTENT,
            height: Size.WRAP_CONTENT,
            padding: 0,
            marginLeft: 0, marginTop: 0, marginRight: 0, marginBottom: 0,
            gravity: Gravity.LEFT | Gravity.TOP,
            color: 0x00000000,
            ...props
        };
        this.children = props.children || [];
        this.measuredWidth = 0;
        this.measuredHeight = 0;
        this.renderWidth = 0;
        this.renderHeight = 0;
        this.absX = 0;
        this.absY = 0;
    }

    measure(parentWidth, parentHeight) {
        let mw = 0, mh = 0;
        if (this.props.width === Size.MATCH_PARENT) mw = parentWidth;
        else if (this.props.width >= 0) mw = this.props.width;

        if (this.props.height === Size.MATCH_PARENT) mh = parentHeight;
        else if (this.props.height >= 0) mh = this.props.height;

        this.measuredWidth = mw;
        this.measuredHeight = mh;
    }

    layout(x, y, width, height) {
        this.absX = x + this.props.marginLeft;
        this.absY = y + this.props.marginTop;
        this.renderWidth = width - this.props.marginLeft - this.props.marginRight;
        this.renderHeight = height - this.props.marginTop - this.props.marginBottom;
    }

    paint() {
        const d = Window.DENSITY || 1.0;
        if (this.props.color !== 0x00000000 || this.props.type !== undefined) {
            nativeDraw(this.props.type || CMD.RECT, {
                x: this.absX * d,
                y: this.absY * d,
                w: this.renderWidth * d,
                h: this.renderHeight * d,
                ...this.props
            });
        }
        this.children.forEach(c => c.paint());
    }
}