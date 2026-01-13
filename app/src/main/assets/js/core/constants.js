export const CMD = { RECT: 0, CIRCLE: 1, TEXT: 2, CLEAR: 3 };

export const Size = {
    MATCH_PARENT: -1,
    WRAP_CONTENT: -2
};

export const Gravity = {
    LEFT: 1,
    RIGHT: 2,
    CENTER_HORIZONTAL: 4,
    TOP: 8,
    BOTTOM: 16,
    CENTER_VERTICAL: 32,
    CENTER: 36 // CENTER_HORIZONTAL | CENTER_VERTICAL
};

export class Window {
    static WIDTH = 1;
    static HEIGHT = 1;
    static DENSITY = 1.0;

    static getRatio() {
        return this.WIDTH / this.HEIGHT;
    }
}