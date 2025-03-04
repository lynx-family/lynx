// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import static com.lynx.tasm.resourceprovider.LynxResourceRequest.LynxResourceType.LynxResourceTypeTemplate;
import static com.lynx.tasm.resourceprovider.LynxResourceResponse.ResponseState.SUCCESS;

import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;

/**
 * Help LynxResourceLoader to load template from LynxTemplateResourceFetcher.
 *
 * Currently there are two classes with the same name in LynxSdk:
 * com.lynx.tasm.resourceprovider.LynxResourceResponse and
 * com.lynx.tasm.provider.LynxResourceResponse.To keep the code clean, separate the
 * LynxTemplateResourceFetcher that uses resourceprovider into this class
 *
 * TODO(zhoupeng.z): in long term, remove com.lynx.tasm.provider and this class
 */
class TemplateLoaderHelper {
  private final LynxTemplateResourceFetcher mTemplateFetcher;

  public TemplateLoaderHelper(LynxTemplateResourceFetcher templateFetcher) {
    mTemplateFetcher = templateFetcher;
  }

  public boolean hasTemplateFetcher() {
    return mTemplateFetcher != null;
  }

  public void fetchTemplateByGenericTemplateFetcher(
      String url, LynxResourceLoader.TemplateLoaderCallback callback) {
    mTemplateFetcher.fetchTemplate(new LynxResourceRequest(url, LynxResourceTypeTemplate),
        new LynxResourceCallback<TemplateProviderResult>() {
          private final LynxResourceLoader.TemplateLoaderCallback mCallback = callback;

          @Override
          public void onResponse(LynxResourceResponse<TemplateProviderResult> response) {
            TemplateProviderResult data = response.getData();
            byte[] template = data != null ? data.getTemplateBinary() : null;
            TemplateBundle bundle = data != null ? data.getTemplateBundle() : null;
            String errorMsg = response.getError() != null ? response.getError().getMessage() : null;
            callback.onTemplateLoaded(response.getState() == SUCCESS, template, bundle, errorMsg);
          }
        });
  }
}
