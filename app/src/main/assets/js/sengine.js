import { CMD } from './core/constants.js';
import { Element } from './core/element.js';
import { Column } from './component/column.js';
import { Row } from './component/row.js';
import { Container } from './component/container.js';
import { RelativeLayout } from './component/relative_layout.js';
import { Text } from './component/text.js';
import { Image } from './component/image.js';
import { Size, Gravity } from './core/constants.js';
import { Window } from "./core/constants.js";

export class SEngine {
    constructor() {
        this.root = null;
        this.bgColor = 0xFFFFFFFF;
    }

    run(rootElement) {
        this.root = rootElement;
        const tick = () => {
            nativeDraw(3, { color: this.bgColor }); // CMD_CLEAR
            if (this.root) {
                const sw = Window.WIDTH / Window.DENSITY;
                const sh = Window.HEIGHT / Window.DENSITY;
                this.root.measure(sw, sh);
                this.root.layout(0, 0, sw, sh);
                this.root.paint();
            }
            nativeRequestAnimationFrame(tick);
        };
        tick();
    }
}

export const engine = new SEngine();
export { Element, Column, Row, Container, RelativeLayout, Text, Image, Size, Gravity, Window };