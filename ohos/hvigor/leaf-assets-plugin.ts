import * as fs from 'fs';
import * as path from 'path';

import type { HvigorNode, HvigorPlugin } from '@ohos/hvigor';
import { OhosPluginId, type OhosHapContext } from '@ohos/hvigor-ohos-plugin';

const LEAF_ASSETS_PREPARED_KEY = 'leaf_assets_prepared';

function toPosix(p: string): string {
  return p.replace(/\\/g, '/');
}

function countFiles(root: string): number {
  if (!fs.existsSync(root)) {
    return 0;
  }

  let count = 0;
  const stack: string[] = [root];
  while (stack.length > 0) {
    const current = stack.pop()!;
    const entries = fs.readdirSync(current, { withFileTypes: true });
    for (const entry of entries) {
      const fullPath = path.join(current, entry.name);
      if (entry.isDirectory()) {
        stack.push(fullPath);
      } else if (entry.isFile()) {
        count += 1;
      }
    }
  }

  return count;
}

function ensureResourceDirectories(context: OhosHapContext): void {
  const buildProfileOpt = context.getBuildProfileOpt();
  if (!buildProfileOpt || !Array.isArray(buildProfileOpt.targets)) {
    return;
  }

  for (const target of buildProfileOpt.targets) {
    const existing = target.resource?.directories ?? [];
    const directories = new Set<string>(existing.map((item) => toPosix(item)));

    directories.add('./src/main/resources');
    directories.add('./build/generated/leaf/resources');

    target.resource = {
      ...(target.resource ?? {}),
      directories: Array.from(directories)
    };
  }

  context.setBuildProfileOpt(buildProfileOpt);
}

function syncLeafAssets(context: OhosHapContext): void {
  const modulePath = context.getModulePath();
  const leafAssetsRoot = path.resolve(modulePath, '..', '..', 'application', 'assets');
  const generatedResourcesRoot = path.resolve(modulePath, 'build', 'generated', 'leaf', 'resources');
  const generatedRawfileRoot = path.join(generatedResourcesRoot, 'rawfile');

  if (!fs.existsSync(leafAssetsRoot)) {
    throw new Error(`[leaf-assets] assets directory does not exist: ${leafAssetsRoot}`);
  }

  const sourceFileCount = countFiles(leafAssetsRoot);
  if (sourceFileCount === 0) {
    throw new Error(`[leaf-assets] no files found in assets directory: ${leafAssetsRoot}`);
  }

  fs.rmSync(generatedRawfileRoot, { recursive: true, force: true });
  fs.mkdirSync(generatedResourcesRoot, { recursive: true });
  fs.cpSync(leafAssetsRoot, generatedRawfileRoot, { recursive: true, force: true });

  console.info(`[leaf-assets] synced ${sourceFileCount} files to ${generatedRawfileRoot}`);
}

export const leafAssetsPlugin: HvigorPlugin = {
  pluginId: 'leaf-assets-plugin',
  apply(node: HvigorNode) {
    node.afterNodeEvaluate((currentNode: HvigorNode) => {
      if (currentNode.getExtraOption(LEAF_ASSETS_PREPARED_KEY)) {
        return;
      }

      const context = currentNode.getContext(OhosPluginId.OHOS_HAP_PLUGIN) as OhosHapContext | undefined;
      if (!context) {
        throw new Error('[leaf-assets] failed to get OhosHapContext from com.ohos.hap plugin');
      }

      ensureResourceDirectories(context);
      syncLeafAssets(context);

      currentNode.addExtraOption(LEAF_ASSETS_PREPARED_KEY, true);
      console.info('[leaf-assets] plugin configured for module:', currentNode.getNodeName());
    });
  }
};