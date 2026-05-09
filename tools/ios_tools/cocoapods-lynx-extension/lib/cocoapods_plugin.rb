# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'cocoapods-lynx-extension'

module Pod
  class Podfile
    module LynxExtensionDSL
      def use_lynx_extension!(options = {})
        Lynx::Extension::Autolink.install!(self, options)
      end
    end

    include LynxExtensionDSL

    module DSL
      include LynxExtensionDSL
    end
  end
end
