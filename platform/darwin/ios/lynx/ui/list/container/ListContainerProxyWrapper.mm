#import <Lynx/ListContainerProxyWrapper.h>

@interface ListContainerProxyWrapper () {
  lynx::shell::ListContainerProxy *listContainerProxy_;  // 裸指针成员变量
}
@end

@implementation ListContainerProxyWrapper

- (instancetype)initWithListEngineProxy:
    (const std::shared_ptr<lynx::shell::ListEngineProxy> &)listEngineProxy {
  self = [super init];
  if (self) {
    listContainerProxy_ = new lynx::shell::ListContainerProxy(listEngineProxy);
  }
  return self;
}

- (lynx::shell::ListContainerProxy *)getListContainerProxy {
  return listContainerProxy_;
}

@end
