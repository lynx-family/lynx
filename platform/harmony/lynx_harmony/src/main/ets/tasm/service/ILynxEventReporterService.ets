// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceProvider } from './IServiceProvider';
import { ILynxEventReportObserver } from '../eventreport/ILynxEventReportObserver';
import { IPerformanceObserver } from '../performance/IPerformanceObserver';

export interface ILynxEventReporterService extends IServiceProvider, ILynxEventReportObserver, IPerformanceObserver {}