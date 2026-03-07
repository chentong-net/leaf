import { hapTasks } from '@ohos/hvigor-ohos-plugin';

import { leafAssetsPlugin } from '../hvigor/leaf-assets-plugin';

export default {
  system: hapTasks, /* Built-in plugin of Hvigor. It cannot be modified. */
  plugins: [leafAssetsPlugin] /* Custom plugin to extend the functionality of Hvigor. */
};