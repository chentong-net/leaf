## Leaf_Details - 细节优化清单

说明：`README.md` 负责宏观迭代计划，本文件负责细节优化与工程落地任务。


### 高优先级

- [x] LFListView：动态高度真实支持（估算高度、实测回填、高度缓存、锚点补偿防跳动）
- [ ] LFListView：帧预算分片（按帧拆分绑定与测量，避免单帧卡顿，暂时搁置）
  - [ ] LFListView：分片模式滑动抖动问题修复（尤其从上至下滑动时抖动明显，暂时搁置）
- [ ] LFListView：横向列表支持
- [ ] LFListView：瀑布流支持

### 低优先级

- [ ] LFListView：Sticky Header（分组头吸附）
- [ ] LFListView：稳定身份缓存（itemId维度缓存与复用）
- [ ] LFListView：增量更新API（notifyItemInserted/Removed/Changed/Range）
- [ ] LFListView：高度前缀结构升级（Fenwick Tree增量维护）
- [ ] LFListView：滚动位置稳定策略（数据变更后保持可见锚点）
- [ ] LFListView：复用生命周期回调（onAttach/onDetach/onRecycle）
- [ ] LFListView：复用池与预加载窗口精细化配置
- [ ] LFListView：诊断与性能指标面板（create/reuse/bind/measure计数）
- [ ] LFListView：容器语义补齐（contentPadding、itemSpacing、scrollToIndex对齐策略）
