import { Element } from '../core/element.js';
import { CMD, Size, Window } from '../core/constants.js';

export class Image extends Element {
    constructor(path, props = {}) {
        super({
            path: path,
            radius: 0,
            ...props
        });
    }

    paint() {
        const d = Window.DENSITY || 1.0;
        nativeDraw(4, {
            x: this.absX * d,
            y: this.absY * d,
            w: this.renderWidth * d,
            h: this.renderHeight * d,
            path: this.props.path,
            radius: this.props.radius * d
        });
    }
}