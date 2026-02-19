Pod::Spec.new do |s|
  s.name             = 'Leaf_Plugin'
  s.version          = '0.1.0'
  s.summary          = 'Leaf plugin abstraction interfaces for iOS.'
  s.description      = <<-DESC
Leaf iOS plugin abstraction layer. Defines LeafPlugin/LeafMethodCall/LeafResult
for plugins that can be dispatched by the Leaf iOS SDK.
  DESC

  s.homepage         = 'https://github.com/chentong-net/leaf'
  s.license          = { :type => 'Proprietary', :text => 'Leaf iOS plugin interfaces' }
  s.author           = { 'Leaf Team' => 'contact@chentong.net' }
  s.source           = { :git => 'https://github.com/chentong-net/leaf.git', :tag => s.version.to_s }

  s.platform         = :ios, '13.0'
  s.requires_arc     = true
  s.static_framework = true
  s.module_name      = 'Leaf_Plugin'

  s.source_files        = 'Classes/**/*.{h,m,mm}'
  s.public_header_files = 'Classes/**/*.h'
  s.frameworks          = 'Foundation'
  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES'
  }
end
