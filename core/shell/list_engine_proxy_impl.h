//
// Created by ByteDance on 2025/10/20.
//

#ifndef CORE_SHELL_LIST_ENGINE_PROXY_IMPL_H
#define CORE_SHELL_LIST_ENGINE_PROXY_IMPL_H
#include "core/shell/list_engine_proxy.h"
#include "core/shell/lynx_engine.h"
#include "base/include/lynx_actor.h"

namespace lynx{
    namespace shell {
        class ListEngineProxyImpl: public ListEngineProxy  {
        public:
            explicit ListEngineProxyImpl(const std::shared_ptr<LynxActor<shell::LynxEngine>> & engine_actor):engine_actor_(engine_actor){};
            virtual ~ListEngineProxyImpl() = default;

            void ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                                       float original_x, float original_y) override;
            void ScrollToPosition(int32_t tag, int index, float offset, int align,
                                  bool smooth) override;
            void ScrollStopped(int32_t tag) override;
            
        private:
            std::weak_ptr<LynxActor<shell::LynxEngine>> engine_actor_;   

};
    } // namespace shell
    
} // namespace lynx





#endif //ANDROID_LIST_ENGINE_PROXY_IMPL_H
