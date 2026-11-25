# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'cocoapods-lynx-library'

module Pod
  class Podfile
    module LynxLibraryDSL
      def use_lynx_library!(options = {})
        Lynx::Library::Autolink.install!(self, options)
      end
    end

    include LynxLibraryDSL

    module DSL
      include LynxLibraryDSL
    end
  end
end
