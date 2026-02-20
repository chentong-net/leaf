Pod::Spec.new do |s|
  s.name             = 'Leaf_iOS'
  s.version          = '0.1.0'
  s.summary          = 'Leaf iOS SDK with OpenGL ES renderer.'
  s.description      = <<-DESC
Leaf iOS SDK module with LeafView/LeafRenderer and embedded core runtime.
  DESC

  s.homepage         = 'https://github.com/chentong-net/leaf'
  s.license          = { :type => 'Proprietary', :text => 'Leaf iOS SDK' }
  s.author           = { 'Leaf Team' => ' contact@chentong.net' }
  s.source           = { :git => 'https://github.com/chentong-net/leaf.git', :tag => s.version.to_s }

  s.platform         = :ios, '13.0'
  s.requires_arc     = true
  s.static_framework = true

  s.source_files         = [
    'Classes/**/*.{h,m,mm,c,cpp}',
    'Vendor/core/**/*.h',
    'Vendor/third_party/quickjs/*.h',
    'Vendor/third_party/nanovg/src/*.h',
    'Vendor/third_party/yoga/**/*.h'
  ]
  s.public_header_files  = 'Classes/**/*.h'
  s.resource_bundles     = {
    'Leaf_iOS_Assets' => ['Assets/**/*']
  }

  s.frameworks = 'Foundation', 'UIKit', 'CoreGraphics', 'QuartzCore', 'OpenGLES'
  s.libraries  = 'c++', 'z'
  s.compiler_flags = '-D_GNU_SOURCE -D__STDC_FORMAT_MACROS -DCONFIG_BIGNUM'
  s.dependency 'Leaf_Plugin', s.version.to_s

  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'gnu++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'CLANG_ALLOW_NON_MODULAR_INCLUDES_IN_FRAMEWORK_MODULES' => 'YES',
    'HEADER_SEARCH_PATHS' => '$(inherited) "${PODS_TARGET_SRCROOT}/Vendor/core" "${PODS_TARGET_SRCROOT}/Vendor/third_party/quickjs" "${PODS_TARGET_SRCROOT}/Vendor/third_party/nanovg/src" "${PODS_TARGET_SRCROOT}/Vendor/third_party/yoga" "${PODS_TARGET_SRCROOT}/Vendor/app_adapter" "${PODS_TARGET_SRCROOT}/Vendor/examples/reader_app" "${PODS_TARGET_SRCROOT}/Vendor/examples/my_profile" "${PODS_TARGET_SRCROOT}/../../plugins/file_picker"',
    'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) __APPLE__=1'
  }
end
