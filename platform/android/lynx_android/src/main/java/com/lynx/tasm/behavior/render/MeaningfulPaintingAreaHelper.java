// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.ui.ILynxUIMeaningfulContent;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.service.ILynxTextService.Page;
import java.util.ArrayList;
import java.util.List;

final class MeaningfulPaintingAreaHelper {
  private static final int RECORD_SIZE = 6;
  private static final int RECORD_INDEX_SIGN = 0;
  private static final int RECORD_INDEX_TYPE = 1;
  private static final int RECORD_INDEX_X = 2;
  private static final int RECORD_INDEX_Y = 3;
  private static final int RECORD_INDEX_WIDTH = 4;
  private static final int RECORD_INDEX_HEIGHT = 5;

  private MeaningfulPaintingAreaHelper() {}

  static List<MeaningfulPaintingArea> buildMeaningfulPaintingAreas(
      int[] records, PlatformRendererContext platformRendererContext, LynxContext context) {
    if (records == null || records.length == 0) {
      return new ArrayList<>();
    }

    ArrayList<MeaningfulPaintingArea> areas = new ArrayList<>(records.length / RECORD_SIZE);
    for (int i = 0; i + RECORD_SIZE <= records.length; i += RECORD_SIZE) {
      MeaningfulPaintingArea area =
          buildMeaningfulPaintingArea(records, i, platformRendererContext, context);
      if (area != null) {
        areas.add(area);
      }
    }
    return areas;
  }

  static MeaningfulPaintingArea buildMeaningfulPaintingArea(int[] records, int baseIndex,
      PlatformRendererContext platformRendererContext, LynxContext context) {
    int sign = records[baseIndex + RECORD_INDEX_SIGN];
    int type = records[baseIndex + RECORD_INDEX_TYPE];
    int x = records[baseIndex + RECORD_INDEX_X];
    int y = records[baseIndex + RECORD_INDEX_Y];
    int width = records[baseIndex + RECORD_INDEX_WIDTH];
    int height = records[baseIndex + RECORD_INDEX_HEIGHT];

    if (width <= 0 || height <= 0) {
      return null;
    }

    ILynxUIMeaningfulContent.MeaningfulContentStatus status =
        getMeaningfulContentStatus(type, sign, platformRendererContext, context);
    if (status == ILynxUIMeaningfulContent.MeaningfulContentStatus.IRRELEVANT) {
      return null;
    }

    MeaningfulPaintingArea area = new MeaningfulPaintingArea(
        x, y, width, height, isMeaningfulContentValid(type, sign, platformRendererContext));
    area.setVisibleStatus(platformRendererContext.getMeaningfulPaintingAreaVisibleStatus(sign));
    area.setAlpha(platformRendererContext.getMeaningfulPaintingAreaAlpha(sign));
    area.setScaleX(platformRendererContext.getMeaningfulPaintingAreaScaleX(sign));
    area.setScaleY(platformRendererContext.getMeaningfulPaintingAreaScaleY(sign));
    area.setMeaningfulContentStatus(status);
    area.setFirstMeaningfulContentPresentedTimestampMicros(-1);
    return area;
  }

  private static boolean isMeaningfulContentValid(
      int type, int sign, PlatformRendererContext platformRendererContext) {
    if (type != PlatformRendererContext.PlatformRendererType.kImage) {
      return true;
    }
    LynxImageManager imageManager = platformRendererContext.getImage(sign);
    return imageManager != null && imageManager.getHasContent();
  }

  private static ILynxUIMeaningfulContent.MeaningfulContentStatus getMeaningfulContentStatus(
      int type, int sign, PlatformRendererContext platformRendererContext, LynxContext context) {
    if (type == PlatformRendererContext.PlatformRendererType.kImage) {
      LynxImageManager imageManager = platformRendererContext.getImage(sign);
      if (imageManager != null && imageManager.getHasContent()) {
        return ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED;
      }
      return ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING;
    }

    if (type == PlatformRendererContext.PlatformRendererType.kText) {
      if (isTextPresented(sign, platformRendererContext, context)) {
        return ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED;
      }
      return ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING;
    }

    return ILynxUIMeaningfulContent.MeaningfulContentStatus.IRRELEVANT;
  }

  private static boolean isTextPresented(
      int sign, PlatformRendererContext platformRendererContext, LynxContext context) {
    if (context != null && context.isTextServiceModeOn()) {
      Page page = platformRendererContext.getTextBundle(sign);
      return page != null;
    }

    TextMeasurer textMeasurer = platformRendererContext.getTextMeasurer();
    return textMeasurer != null && textMeasurer.takeTextLayout(sign) != null;
  }
}
