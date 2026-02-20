Pod::Spec.new do |s|
  s.name             = 'File_Picker'
  s.version          = '0.1.0'
  s.summary          = 'Leaf file picker plugin for iOS.'
  s.description      = <<-DESC
Leaf file picker plugin for iOS. Implements file_picker.pick and file_picker.open_fd.
  DESC

  s.homepage         = 'https://github.com/chentong-net/leaf'
  s.license          = { :type => 'Proprietary', :text => 'Leaf file picker iOS plugin' }
  s.author           = { 'Leaf Team' => 'contact@chentong.net' }
  s.source           = { :git => 'https://github.com/chentong-net/leaf.git', :tag => s.version.to_s }

  s.platform         = :ios, '13.0'
  s.requires_arc     = true
  s.static_framework = true
  s.module_name      = 'File_Picker'

  s.source_files        = 'Classes/**/*.{h,m,mm}'
  s.public_header_files = 'Classes/**/*.h'
  s.frameworks          = 'Foundation', 'UIKit'
  s.dependency 'Leaf_Plugin', '0.1.0'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES'
  }
end
