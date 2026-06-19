Pod::Spec.new do |s|
  s.name             = 'I18n'
  s.version          = '1.0.0'
  s.summary          = 'Leaf i18n plugin for iOS.'
  s.description      = <<-DESC
Leaf i18n plugin for iOS. Implements i18n.get_system_language.
  DESC

  s.homepage         = 'https://github.com/chentong-net/leaf'
  s.license          = { :type => 'Proprietary', :text => 'Leaf i18n iOS plugin' }
  s.author           = { 'Leaf Team' => 'contact@chentong.net' }
  s.source           = { :git => 'https://github.com/chentong-net/leaf.git', :tag => s.version.to_s }

  s.platform         = :ios, '13.0'
  s.requires_arc     = true
  s.static_framework = true
  s.module_name      = 'I18n'

  s.source_files        = 'Classes/**/*.{h,m,mm}'
  s.public_header_files = 'Classes/**/*.h'
  s.frameworks          = 'Foundation'
  s.dependency 'Leaf_Plugin', '1.0.0'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES'
  }
end
