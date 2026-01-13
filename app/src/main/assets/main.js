import {
    engine,
    Column, Row, Text, Container, Image,
    Size, Gravity, Window
} from './js/sengine.js';

function onAppStart(width, height, density) {
    Window.WIDTH = width;
    Window.HEIGHT = height;
    Window.DENSITY = density;
}
globalThis.onAppStart = onAppStart;

const profilePage = new Container({
    width: Size.MATCH_PARENT,
    height: Size.MATCH_PARENT,
    color: 0xFFF5F5F7, // 浅灰色
    gravity: Gravity.TOP | Gravity.CENTER_HORIZONTAL,
    padding: 20,
    children: [
        new Column({
            width: Size.MATCH_PARENT,
            spacing: 15,
            gravity: Gravity.CENTER_HORIZONTAL,
            children: [
                // 头像
                new Image("avatar.jpg", {
                    width: 80,
                    height: 80,
                    radius: 40
                }),

                // 用户名
                new Text("Chen Tong", {
                    fontSize: 22,
                    color: 0xFF1D1D1F, // 黑色
                }),

                // 简介
                new Text("Full-stack Engineer & UI Designer", {
                    fontSize: 14,
                    color: 0xFF86868B, // 灰色
                }),

                // 数据统计行
                new Row({
                    width: Size.WRAP_CONTENT,
                    spacing: 30,
                    children: [
                        new StatItem("99+", "Posts"),
                        new StatItem("12k", "Followers"),
                        new StatItem("350", "Following"),
                    ]
                }),

                // 操作按钮
                new Container({
                    width: 200, height: 45,
                    color: 0xFF000000,
                    radius: 12,
                    gravity: Gravity.CENTER,
                    children: [
                        new Text("Edit Profile", { color: 0xFFFFFFFF, fontSize: 16 })
                    ]
                })
            ]
        }),

        new Container({
            width: Size.MATCH_PARENT,
            height: Size.MATCH_PARENT,
            gravity: Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL,
            children: [
                new Text("contact@chentong.net", { fontSize: 16, color: 0xFF999999 })
            ]
        })
    ]
});

// 辅助组件：统计项
function StatItem(value, label) {
    return new Column({
        gravity: Gravity.CENTER_HORIZONTAL,
        children: [
            new Text(value, { fontSize: 18, color: 0xFF1D1D1F }),
            new Text(label, { fontSize: 12, color: 0xFF86868B }),
        ]
    });
}

engine.run(profilePage);