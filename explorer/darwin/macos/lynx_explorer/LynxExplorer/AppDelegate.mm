// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "AppDelegate.h"
#import "LynxWindow.h"
#import "LynxWindowController.h"
#import "ViewController.h"
#include "explorer/darwin/macos/lynx_explorer/LynxExplorer/module/LynxDemoModule.h"
#include "explorer/darwin/macos/lynx_explorer/LynxExplorer/service/LynxHttpService.h"
#include "explorer/embedder/lynx_explorer/module/lynx_demo_extension_module.h"
#include "lynx_env.h"

#if defined(LYNX_NODE_API_ADDON_USE_HEADER)
#include LYNX_NODE_API_ADDON_USE_HEADER
#endif

@interface AppDelegate ()

@property(strong) IBOutlet NSWindow *window;

@end

@implementation AppDelegate {
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  // Insert code here to initialize your application
  [self initSettingsMenu];

  auto &lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.SetDevtoolEnabled(true);
  NSDictionary *env = [[NSProcessInfo processInfo] environment];
  const char *remote_debug_url = nil;
  if ([[env allKeys] containsObject:@"lynx_scheme_url"]) {
    remote_debug_url = [env[@"lynx_scheme_url"] UTF8String];
  }
  if (remote_debug_url) {
    lynx_env.SetDevtoolAppInfo("App", "LynxExplorer");
    lynx_env.SetDevtoolAppInfo("AppVersion", "1.0.0");
    lynx_env.ConnectDevtool(remote_debug_url);
  }

  // TODO: Prefer to use the C++ Wrapper. Use c-napi now because of the compatibility.
  lynx_env_register_native_module("ExplorerModule", &ExplorerModuleCreator, nullptr);
  lynx_env.RegisterExtensionModule("LynxDemoExtensionModule",
                                   &lynx::example::LynxDemoExtensionModule::CreateCModule, false,
                                   nullptr);

  // Init LynxServices
  lynx::pub::LynxServiceCenter::GetInstance().RegisterService(
      std::make_shared<lynx::service::LynxHttpServiceImpl>());

  self.window.contentViewController = [[ViewController alloc] initWithUrl:nil];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
  // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
  return YES;
}

- (void)initLynx {
}

- (void)initSettingsMenu {
  NSMenu *mainMenu = [NSApp mainMenu];
  NSMenuItem *debugSettingsItem = [[NSMenuItem alloc] init];
  [debugSettingsItem setTitle:@"DebugSettingsMenu"];
  if ([mainMenu itemWithTitle:@"Help"] != nil) {
    NSInteger index = [mainMenu indexOfItemWithTitle:@"Help"];
    [mainMenu insertItem:debugSettingsItem atIndex:index];
  } else {
    [mainMenu addItem:debugSettingsItem];
  }
  NSMenu *subMenu = [[NSMenu alloc] initWithTitle:@"DebugSettings"];
  NSMenuItem *openPanelItem = [[NSMenuItem alloc] initWithTitle:@"DebugSettingsPanel"
                                                         action:@selector(openSettingsPanel)
                                                  keyEquivalent:@""];
  openPanelItem.target = self;
  [subMenu addItem:openPanelItem];
  [debugSettingsItem setSubmenu:subMenu];
}

- (void)openSettingsPanel {
}

@end
