Pod::Spec.new do |s|
  s.name             = 'Path_Provider'
  s.version          = '1.0.0'
  s.summary          = 'Leaf path provider plugin for iOS.'
  s.description      = <<-DESC
Leaf path provider plugin for iOS. Implements path_provider path query methods.
  DESC

  s.homepage         = 'https://github.com/chentong-net/leaf'
  s.license          = { :type => 'Proprietary', :text => 'Leaf path provider iOS plugin' }
  s.author           = { 'Leaf Team' => 'contact@chentong.net' }
  s.source           = { :git => 'https://github.com/chentong-net/leaf.git', :tag => s.version.to_s }

  s.platform         = :ios, '13.0'
  s.requires_arc     = true
  s.static_framework = true
  s.module_name      = 'Path_Provider'

  s.source_files        = 'Classes/**/*.{h,m,mm}'
  s.public_header_files = 'Classes/**/*.h'
  s.frameworks          = 'Foundation'
  s.dependency 'Leaf_Plugin', '0.1.0'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES'
  }
end
