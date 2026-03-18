Pod::Spec.new do |s|
  s.name             = 'Leaf_iOS'
  s.version          = '1.0.0'
  s.summary          = 'Leaf iOS SDK with OpenGL ES renderer.'
  s.description      = <<-DESC
Leaf iOS SDK module with LeafView/LeafRenderer, plugin dispatcher, and prebuilt native runtime.
  DESC

  s.homepage         = 'https://github.com/chentong-net/leaf'
  s.license          = { :type => 'Proprietary', :text => 'Leaf iOS SDK' }
  s.author           = { 'Leaf Team' => 'contact@chentong.net' }
  s.source           = { :git => 'https://github.com/chentong-net/leaf.git', :tag => s.version.to_s }

  s.platform         = :ios, '13.0'
  s.requires_arc     = true
  s.static_framework = true

  s.source_files         = 'Classes/**/*.{h,m,mm}'
  s.public_header_files  = 'Classes/**/*.h'
  s.vendored_frameworks  = 'Frameworks/LeafNative.xcframework'
  s.resource_bundles     = {
    'Leaf_iOS_Assets' => ['Assets/**/*']
  }

  s.frameworks = 'Foundation', 'UIKit', 'CoreGraphics', 'QuartzCore', 'OpenGLES'
  s.libraries  = 'c++', 'z'
  s.dependency 'Leaf_Plugin', s.version.to_s

  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'gnu++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'CLANG_ALLOW_NON_MODULAR_INCLUDES_IN_FRAMEWORK_MODULES' => 'YES',
    'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) __APPLE__=1',
    'DEFINES_MODULE' => 'YES'
  }
end
