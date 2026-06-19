Pod::Spec.new do |s|
  s.name             = 'Audio_Player'
  s.version          = '1.0.0'
  s.summary          = 'Leaf audio player plugin for iOS.'
  s.description      = <<-DESC
Leaf audio player plugin for iOS. Implements audio_player.* methods using AVAudioPlayer.
  DESC

  s.homepage         = 'https://github.com/chentong-net/leaf'
  s.license          = { :type => 'Proprietary', :text => 'Leaf audio player iOS plugin' }
  s.author           = { 'Leaf Team' => 'contact@chentong.net' }
  s.source           = { :git => 'https://github.com/chentong-net/leaf.git', :tag => s.version.to_s }

  s.platform         = :ios, '13.0'
  s.requires_arc     = true
  s.static_framework = true
  s.module_name      = 'Audio_Player'

  s.source_files        = 'Classes/**/*.{h,m,mm}'
  s.public_header_files = 'Classes/**/*.h'
  s.frameworks          = 'AVFoundation', 'Foundation'
  s.dependency 'Leaf_Plugin', '1.0.0'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES'
  }
end
