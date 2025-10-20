//
// Created by ByteDance on 2025/10/20.
//

#ifndef CORE_SHELL_LIST_ENGINE_PROXY_H
#define CORE_SHELL_LIST_ENGINE_PROXY_H

#include <memory>
#include <utility>

namespace lynx {
    namespace shell {

        class ListEngineProxy {
        public:
            virtual void ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                                               float original_x, float original_y) = 0;

            virtual void ScrollToPosition(int32_t tag, int index, float offset, int align,
                                          bool smooth) = 0;

            virtual void ScrollStopped(int32_t tag) = 0;


           

        };
        
    } // namespace shell
   
} // namespace Lynx




#endif //ANDROID_LIST_ENGINE_PROXY_H
