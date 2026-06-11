// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ViewController.h"
#include <memory>
#include "explorer/darwin/macos/lynx_explorer/LynxExplorer/fetcher/ExampleGenericResourceFetcher.h"
#include "explorer/darwin/macos/lynx_explorer/LynxExplorer/module/LynxNodeAPIModule.h"
#include "explorer/darwin/macos/lynx_explorer/LynxExplorer/runtime/ExampleLynxRuntimeLifecycleObserver.h"
#include "lynx_env.h"
#include "lynx_native_view.h"
#include "lynx_value.h"
#include "lynx_view.h"
#if ENABLE_TESTBENCH_REPLAY
#include "platform/embedder/lynx_recorder/test_bench_action_manager.h"
#endif

using lynx::pub::LynxValue;

static constexpr int32_t kLynxLocalPrefixLength = 20;

static BOOL IsTruthyQueryValue(NSString *value) {
  if (value == nil) {
    return NO;
  }
  NSString *lower = value.lowercaseString;
  return [lower isEqualToString:@"1"] || [lower isEqualToString:@"true"] ||
         [lower isEqualToString:@"yes"];
}

static BOOL ShouldEnableNapiAddonForURL(NSString *url) {
  if (url.length == 0) {
    return NO;
  }
  NSRange queryRange = [url rangeOfString:@".lynx.bundle?"];
  if (queryRange.location == NSNotFound) {
    return NO;
  }
  NSUInteger queryStart = queryRange.location + queryRange.length;
  if (queryStart >= url.length) {
    return NO;
  }
  NSString *query = [url substringFromIndex:queryStart];
  for (NSString *item in [query componentsSeparatedByString:@"&"]) {
    NSArray<NSString *> *parts = [item componentsSeparatedByString:@"="];
    if (parts.count >= 2 && [parts[0] isEqualToString:@"enable_napi_addon"]) {
      return IsTruthyQueryValue(parts[1]);
    }
  }
  return NO;
}

class FakeView : public lynx::pub::LynxNativeView {
 public:
  FakeView(void *opaque) : vc_((__bridge ViewController *)opaque) {}
  ~FakeView() override {}
  bool OnCreate() override { return true; }
  void OnDestroy() override {}
  void OnAttach() override {}
  void OnDetach() override {}
  void OnLayoutChanged(float left, float top, float width, float height,
                       float pixel_ratio) override {
    LynxValue detail(LynxValue::kCreateAsMapTag);
    detail.SetProperty("pixelRatio", LynxValue(pixel_ratio));
    detail.SetProperty("width", LynxValue(width));
    detail.SetProperty("height", LynxValue(height));
    TriggerEvent("resize", std::move(detail));
  }
  void OnPropertiesChanged(const LynxValue &attrs, const LynxValue &events) override {}
  void OnMethodInvoked(const char *method, const LynxValue &attrs,
                       std::function<void(int, lynx::pub::LynxValue &&)> callback) override {
    callback(kSuccess, LynxValue(LynxValue::kCreateAsNullTag));
  }
  bool IsScrollEnabled() override { return true; }
  bool IsSurfaceEnabled() override { return true; }

 private:
  __weak ViewController *vc_;
};

std::vector<uint8_t> ConvertNSBinary(NSData *binary) {
  std::vector<uint8_t> result;
  auto len = binary.length;
  if (len > 0) {
    auto begin = reinterpret_cast<const uint8_t *>(binary.bytes);
    result.assign(begin, begin + len);
  }
  return result;
}

@interface ViewController ()

@property(nonatomic) std::shared_ptr<lynx::pub::LynxView> lynxView;
#if ENABLE_TESTBENCH_REPLAY
@property(nonatomic) std::shared_ptr<lynx::embedder::TestBenchActionManager> testBenchActionManager;
@property(nonatomic) BOOL isTestBenchReplay;
#endif
@end

@implementation ViewController {
  CGFloat _lastScaleFactor;
}

- (void)loadView {
  self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)];
}

- (instancetype)initWithUrl:(NSString *)url {
  self = [super init];
  if (self) {
    self.url = url;

    NSScreen *screen = [NSScreen mainScreen];
    _lastScaleFactor = screen.backingScaleFactor;
#if ENABLE_TESTBENCH_REPLAY
    self.isTestBenchReplay = [_url hasPrefix:@"sslocal://arkview?"];
#endif
  }
  return self;
}

- (void)viewDidLoad {
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(notifyWindowBecomeActive)
                                               name:NSWindowDidDeminiaturizeNotification
                                             object:nil];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(notifyWindowEnterBackground)
                                               name:NSWindowDidMiniaturizeNotification
                                             object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowScreenDidChange:)
                                               name:NSWindowDidChangeScreenNotification
                                             object:nil];
  [super viewDidLoad];
  [self loadLynxView];
  [self reloadTemplate];
}

- (void)windowScreenDidChange:(NSNotification *)notification {
  NSScreen *newScreen = self.view.window.screen;
  if (!newScreen || _lastScaleFactor == newScreen.backingScaleFactor) return;

  _lastScaleFactor = newScreen.backingScaleFactor;
  _lynxView->UpdateScreenMetrics(self.view.frame.size.width, self.view.frame.size.height,
                                 _lastScaleFactor);
}

- (void)notifyWindowBecomeActive {
  _lynxView->OnEnterForeground();
}

- (void)notifyWindowEnterBackground {
  _lynxView->OnEnterBackground();
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  _lynxView.reset();
}

- (void)viewDidLayout {
  [super viewDidLayout];
  _lynxView->UpdateScreenMetrics(self.view.frame.size.width, self.view.frame.size.height,
                                 _lastScaleFactor);
  _lynxView->SetFrame(0, 0, self.view.frame.size.width, self.view.frame.size.height);
}

- (BOOL)acceptsFirstResponder {
  return false;
}

- (void)reloadTemplate {
  NSString *remote_debug_url = nil;
  if (self.url == nil) {
    auto args = [NSProcessInfo processInfo].arguments;
    for (NSUInteger i = 1; args && i < args.count; i++) {
      if ([args[i] hasPrefix:@"--url="]) {
        self.url = [args[i] substringFromIndex:6];
        break;
      } else if ([args[i] hasPrefix:@"--remote-debug="]) {
        remote_debug_url = [args[i] substringFromIndex:15];
        break;
      }
    }
  }
  const char *remote_debug_url_str = nil;
  if (remote_debug_url) {
    remote_debug_url_str = [remote_debug_url UTF8String];
  }
  if (remote_debug_url_str) {
    auto &lynx_env = lynx::pub::LynxEnv::GetInstance();
    lynx_env.SetDevtoolAppInfo("App", "LynxExplorer");
    lynx_env.SetDevtoolAppInfo("AppVersion", "1.0.0");
    lynx_env.ConnectDevtool(remote_debug_url_str);
  }

  if (!self.url || [self.url length] == 0) {
    auto meta_data = std::make_shared<lynx::pub::LynxLoadMeta>();
    meta_data->SetUrl("assets://main.lynx.bundle");
    NSData *data =
        [NSData dataWithContentsOfFile:[[NSBundle mainBundle]
                                           pathForResource:@"Resource/homepage/main.lynx.bundle"
                                                    ofType:nil]];
    meta_data->SetGlobalProps(std::make_shared<lynx::pub::LynxTemplateData>(
        "{\"theme\":\"light\",\"platform\":\"macos\"}"));
    meta_data->SetBinaryData(ConvertNSBinary(data));
    _lynxView->LoadTemplate(meta_data);
    return;
  }
#if ENABLE_TESTBENCH_REPLAY
  if (_isTestBenchReplay) {
    _testBenchActionManager->StartWithUrl([self.url UTF8String]);
    return;
  }
#endif
  NSURL *source = [NSURL URLWithString:self.url];

  if ([source.scheme isEqualToString:@"sslocal"]) {
    NSURL *subSourceUrl = [NSURL URLWithString:source.query];
    if ([subSourceUrl.scheme isEqualToString:@"local"]) {
      NSString *str =
          [NSString stringWithFormat:@"Resource/%@%@", subSourceUrl.host, subSourceUrl.path];
      NSString *targetUrl =
          [[NSBundle mainBundle] pathForResource:[str stringByDeletingPathExtension] ofType:@"js"];
      if (targetUrl == nil) {
        return;
      }
      auto meta_data = std::make_shared<lynx::pub::LynxLoadMeta>();
      meta_data->SetUrl("assets://main.lynx.bundle");
      NSData *data = [NSData dataWithContentsOfFile:targetUrl];
      meta_data->SetBinaryData(ConvertNSBinary(data));
      _lynxView->LoadTemplate(meta_data);
    }
  } else if ([source.scheme isEqualToString:@"file"]) {
    NSString *filePath = self.url;
    NSRange bundleExtensionRange = [filePath rangeOfString:@".lynx.bundle"
                                                   options:NSBackwardsSearch];
    if (bundleExtensionRange.location != NSNotFound) {
      NSUInteger newLength = bundleExtensionRange.location + bundleExtensionRange.length;
      if (newLength <= filePath.length) {
        filePath = [filePath substringToIndex:newLength];
      }
    }
    if ([filePath hasPrefix:@"file://lynx?local://"]) {
      NSString *relativePath = [filePath substringFromIndex:kLynxLocalPrefixLength];
      NSString *bundleDir = [[NSBundle mainBundle] resourcePath];
      NSString *resource_path = [NSString stringWithFormat:@"Resource/%@", relativePath];
      filePath = [bundleDir stringByAppendingPathComponent:resource_path];
      BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:filePath];
      if (!fileExists) {
        NSString *resourceName = [relativePath stringByDeletingPathExtension];
        NSString *resourceType = @"bundle";
        NSString *resourceSubDir = @"Resource";
        filePath = [[NSBundle mainBundle] pathForResource:resourceName
                                                   ofType:resourceType
                                              inDirectory:resourceSubDir];
      }
    }
    NSData *data = [NSData dataWithContentsOfFile:filePath];
    if (data) {
      auto meta_data = std::make_shared<lynx::pub::LynxLoadMeta>();
      meta_data->SetUrl("assets://main.lynx.bundle");
      meta_data->SetBinaryData(ConvertNSBinary(data));
      auto global_props = std::make_shared<lynx::pub::LynxTemplateData>(
          "{\"theme\": \"light\", \"platform\": \"macos\"}");
      meta_data->SetGlobalProps(global_props);
      _lynxView->LoadTemplate(meta_data);
    }
  } else {
    auto meta_data = std::make_shared<lynx::pub::LynxLoadMeta>();
    meta_data->SetUrl([self.url UTF8String]);
    _lynxView->LoadTemplate(meta_data);
  }
}

- (void)loadTemplateFromURL:(NSString *)url {
}

- (void)onRefresh {
  [self reload];
}

- (void)reload {
  [self reloadTemplate];
}

- (void)loadLynxView {
  // auto group = std::make_shared<lynx::pub::LynxGroup>("group_name", "group_id");
  NSString *resolvedUrl = self.url;
  if (resolvedUrl == nil) {
    auto args = [NSProcessInfo processInfo].arguments;
    for (NSUInteger i = 1; args && i < args.count; i++) {
      if ([args[i] hasPrefix:@"--url="]) {
        resolvedUrl = [args[i] substringFromIndex:6];
        break;
      }
    }
  }
  BOOL enableNapiAddon = ShouldEnableNapiAddonForURL(resolvedUrl);
  uint64_t tokenId = reinterpret_cast<uint64_t>((__bridge void *)self);
  lynx::pub::LynxView::Builder builder;
  builder.SetScreenSize(self.view.frame.size.width, self.view.frame.size.height, 1.0)
      .SetFrame(0, 0, self.view.frame.size.width, self.view.frame.size.height)
      .SetParent((__bridge NativeWindow)self.view)
      // .SetLynxGroup(group)
      .SetGenericResourceFetcher(std::make_shared<lynx::example::ExampleGenericResourceFetcher>())
      .RegisterNativeView<FakeView>("x-fake-view", (__bridge void *)self);
  if (enableNapiAddon) {
    builder.RegisterNativeModule("LynxNodeAPI", LynxNodeAPIModuleCreator, (__bridge void *)self);
  }
#if ENABLE_TESTBENCH_REPLAY
  if (_isTestBenchReplay) {
    lynx::embedder::TestBenchReplayDataModule::RegisterJSB(
        [&builder](const std::string &name, napi_module_creator creator) {
          lynx_view_builder_register_native_module(builder.Impl(), name.c_str(), creator, nullptr);
        });
  }
#endif
  _lynxView = builder.Build();
  _lynxView->RegisterNativeView<FakeView>("x-fake-view-alias", (__bridge void *)self);
  // Example of runtime lifecycle observer.
  _lynxView->RegisterRuntimeLifecycleObserver(
      std::make_shared<lynx::example::ExampleLynxRuntimeLifecycleObserver>(enableNapiAddon ? tokenId
                                                                                           : 0));
#if ENABLE_TESTBENCH_REPLAY
  if (_isTestBenchReplay) {
    _testBenchActionManager = std::make_shared<lynx::embedder::TestBenchActionManager>(
        _lynxView,
        [self](int width, int height) { [self.view setFrame:NSMakeRect(0, 0, width, height)]; });
    _testBenchActionManager->SetFetchCallback(
        [](const std::string &url_str, std::function<void(const std::string &result)> callback) {
          NSString *ns_string = [NSString stringWithUTF8String:url_str.c_str()];
          NSURL *url = [NSURL URLWithString:ns_string];
          NSMutableURLRequest *nsRequest = [NSMutableURLRequest requestWithURL:url];
          NSURLSession *session = [NSURLSession sharedSession];
          NSURLSessionDataTask *dataTask = [session
              dataTaskWithRequest:nsRequest
                completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                                    NSError *_Nullable error) {
                  if (data && data.length > 0) {
                    std::string result((const char *)data.bytes, data.length);
                    callback(std::move(result));
                  } else {
                    callback("");
                  }
                }];

          [dataTask resume];
        });
  }
#endif
}

@end
