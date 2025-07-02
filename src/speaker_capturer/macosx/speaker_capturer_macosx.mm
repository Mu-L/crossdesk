#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include "speaker_capturer_macosx.h"

// 这个delegate用来接收SCStream回调
@interface SpeakerCaptureDelegate : NSObject <SCStreamDelegate, SCStreamOutput>
@property(nonatomic, assign) SpeakerCapturerMacosx* owner;  // assign用于C++指针，不用weak
- (instancetype)initWithOwner:(SpeakerCapturerMacosx*)owner;
@end

@implementation SpeakerCaptureDelegate
- (instancetype)initWithOwner:(SpeakerCapturerMacosx*)owner {
  self = [super init];
  if (self) {
    _owner = owner;
  }
  return self;
}

- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type {
  if (type == SCStreamOutputTypeAudio) {
    CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
    size_t length = CMBlockBufferGetDataLength(blockBuffer);
    char* dataPtr = NULL;
    CMBlockBufferGetDataPointer(blockBuffer, 0, NULL, NULL, &dataPtr);

    // 获取输入格式
    CMAudioFormatDescriptionRef formatDesc = CMSampleBufferGetFormatDescription(sampleBuffer);
    const AudioStreamBasicDescription* asbd =
        CMAudioFormatDescriptionGetStreamBasicDescription(formatDesc);

    if (_owner->cb_ && dataPtr && length > 0 && asbd) {
      std::vector<short> out_pcm16;
      if (asbd->mFormatFlags & kAudioFormatFlagIsFloat) {
        int channels = asbd->mChannelsPerFrame;
        int samples = (int)(length / sizeof(float));
        float* floatData = (float*)dataPtr;
        std::vector<short> pcm16(samples);
        for (int i = 0; i < samples; ++i) {
          float v = floatData[i];
          if (v > 1.0f) v = 1.0f;
          if (v < -1.0f) v = -1.0f;
          pcm16[i] = (short)(v * 32767.0f);
        }
        // 混合为单声道
        if (channels > 1) {
          int mono_samples = samples / channels;
          out_pcm16.resize(mono_samples);
          for (int i = 0; i < mono_samples; ++i) {
            int sum = 0;
            for (int c = 0; c < channels; ++c) {
              sum += pcm16[i * channels + c];
            }
            out_pcm16[i] = sum / channels;
          }
        } else {
          out_pcm16 = std::move(pcm16);
        }
      } else if (asbd->mBitsPerChannel == 16) {
        int channels = asbd->mChannelsPerFrame;
        int samples = (int)(length / 2);
        short* src = (short*)dataPtr;
        if (channels > 1) {
          int mono_samples = samples / channels;
          out_pcm16.resize(mono_samples);
          for (int i = 0; i < mono_samples; ++i) {
            int sum = 0;
            for (int c = 0; c < channels; ++c) {
              sum += src[i * channels + c];
            }
            out_pcm16[i] = sum / channels;
          }
        } else {
          out_pcm16.assign(src, src + samples);
        }
      }

      // 分包，每960字节送一次cb_（即480采样点）
      size_t frame_bytes = 960;  // 480 * 2
      size_t total_bytes = out_pcm16.size() * sizeof(short);
      unsigned char* p = (unsigned char*)out_pcm16.data();
      for (size_t offset = 0; offset + frame_bytes <= total_bytes; offset += frame_bytes) {
        _owner->cb_(p + offset, frame_bytes, "audio");
      }
      // 如有剩余，可缓存到下次补齐
    }
  }
}
@end

// C++类实现细节，放这里，隐藏在.mm中
class SpeakerCapturerMacosx::Impl {
 public:
  SCStreamConfiguration* config = nil;
  SCStream* stream = nil;
  SpeakerCaptureDelegate* delegate = nil;
};

SpeakerCapturerMacosx::SpeakerCapturerMacosx() { impl_ = new Impl(); }

SpeakerCapturerMacosx::~SpeakerCapturerMacosx() {
  Destroy();
  delete impl_;
  impl_ = nullptr;
}

int SpeakerCapturerMacosx::Init(speaker_data_cb cb) {
  if (inited_) return 0;
  cb_ = cb;

  impl_->config = [[SCStreamConfiguration alloc] init];
  impl_->config.capturesAudio = YES;
  impl_->config.sampleRate = 48000;
  impl_->config.channelCount = 1;

  impl_->delegate = [[SpeakerCaptureDelegate alloc] initWithOwner:this];

  // 异步获取可共享内容，改为同步等待
  dispatch_semaphore_t sema = dispatch_semaphore_create(0);
  __block SCShareableContent* content = nil;
  __block NSError* error = nil;
  [SCShareableContent
      getShareableContentWithCompletionHandler:^(SCShareableContent* c, NSError* e) {
        content = c;
        error = e;
        dispatch_semaphore_signal(sema);
      }];
  dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);

  if (error || !content) return -1;

  // 选择主显示屏
  CGDirectDisplayID mainDisplayId = CGMainDisplayID();
  SCDisplay* mainDisplay = nil;
  for (SCDisplay* d in content.displays) {
    if (d.displayID == mainDisplayId) {
      mainDisplay = d;
      break;
    }
  }
  if (!mainDisplay) return -1;

  // 用SCContentFilter包装
  SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:mainDisplay
                                                    excludingWindows:@[]];

  impl_->stream = [[SCStream alloc] initWithFilter:filter
                                     configuration:impl_->config
                                          delegate:impl_->delegate];

  NSError* addOutputError = nil;
  dispatch_queue_t queue = dispatch_queue_create("SpeakerAudio.Queue", DISPATCH_QUEUE_SERIAL);
  BOOL ok = [impl_->stream addStreamOutput:impl_->delegate
                                      type:SCStreamOutputTypeAudio
                        sampleHandlerQueue:queue
                                     error:&addOutputError];
  if (!ok || addOutputError) {
    NSLog(@"addStreamOutput error: %@", addOutputError);
    return -1;
  }

  inited_ = true;
  return 0;
}

int SpeakerCapturerMacosx::Start() {
  if (!inited_) return -1;
  dispatch_semaphore_t sema = dispatch_semaphore_create(0);
  __block int ret = 0;
  [impl_->stream startCaptureWithCompletionHandler:^(NSError* _Nullable error) {
    if (error) ret = -1;
    dispatch_semaphore_signal(sema);
  }];
  dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
  return ret;
}

int SpeakerCapturerMacosx::Stop() {
  if (!inited_) return -1;
  dispatch_semaphore_t sema = dispatch_semaphore_create(0);
  [impl_->stream stopCaptureWithCompletionHandler:^(NSError* error) {
    dispatch_semaphore_signal(sema);
  }];
  dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
  inited_ = false;
  return 0;
}

int SpeakerCapturerMacosx::Destroy() {
  Stop();
  cb_ = nullptr;
  if (impl_) {
    impl_->delegate = nil;
    impl_->stream = nil;
    impl_->config = nil;
  }
  inited_ = false;
  return 0;
}

int SpeakerCapturerMacosx::Pause() {
  // ScreenCaptureKit无暂停接口，暂时无实现
  return 0;
}

int SpeakerCapturerMacosx::Resume() { return Start(); }
