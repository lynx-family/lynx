#import <Foundation/Foundation.h>
#include "core/shell/list_container_proxy.h"
#include "core/shell/list_engine_proxy.h"

@interface ListContainerProxyWrapper : NSObject

- (instancetype)initWithListEngineProxy:
    (const std::shared_ptr<lynx::shell::ListEngineProxy> &)listEngineProxy;
- (lynx::shell::ListContainerProxy *)getListContainerProxy;

@end
