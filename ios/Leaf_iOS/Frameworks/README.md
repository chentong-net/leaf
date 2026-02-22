# LeafNative.xcframework

This directory is used for the prebuilt native runtime consumed by `Leaf_iOS.podspec`.

Generate the framework before running `pod install`:

```bash
./ios/scripts/build_leaf_native_xcframework.sh
```

Output path:

```text
ios/Leaf_iOS/Frameworks/LeafNative.xcframework
```

The xcframework bundles native static libraries and exported C++ headers
(core, third-party dependencies used by core, plugin public headers, and demo public headers).
