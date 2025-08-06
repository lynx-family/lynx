// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, expect, it, expectTypeOf } from 'vitest';
import { NODE_REF_INVOKE_ERROR_CODE, uiMethodOptions, uiFieldsOptions, PathData, FieldsParams, FieldsData, PathCallback, NodesRef, SelectorQuery, AnimationV2 } from '../../types/';

describe('Nodes-ref Type Definitions', () => {
  describe('uiMethodOptions', () => {
    it('should match expected interface', () => {
      const options: uiMethodOptions = {
        method: 'testMethod',
        params: { key: 'value' },
        success: (res) => console.log(res),
        fail: (res) => console.error(res),
      };

      expect(options).toHaveProperty('method');
      expect(options).toHaveProperty('params');
      expect(options).toHaveProperty('success');
      expect(options).toHaveProperty('fail');
    });
  });

  describe('uiFieldsOptions', () => {
    it('should match expected interface', () => {
      const options: uiFieldsOptions = {
        id: true,
        dataset: false,
        tag: true,
        query: false,
        unique_id: true,
      };

      expect(options).toHaveProperty('id');
      expect(options).toHaveProperty('dataset');
      expect(options).toHaveProperty('tag');
      expect(options).toHaveProperty('query');
      expect(options).toHaveProperty('unique_id');
    });
  });

  describe('PathData', () => {
    it('should match expected interface', () => {
      const pathData: PathData = {
        data: [
          {
            tag: 'div',
            id: 'test-id',
            class: ['class1', 'class2'],
            dataSet: { key: 'value' },
            index: 0,
          },
        ],
      };

      expect(pathData.data[0]).toHaveProperty('tag');
      expect(pathData.data[0]).toHaveProperty('id');
      expect(pathData.data[0]).toHaveProperty('class');
      expect(pathData.data[0]).toHaveProperty('dataSet');
      expect(pathData.data[0]).toHaveProperty('index');
    });
  });

  describe('FieldsParams and FieldsData', () => {
    it('should match expected interfaces', () => {
      const mockSelectorQuery: SelectorQuery = {
        select: () => mockNodesRef,
        selectAll: () => mockNodesRef,
        selectRoot: () => mockNodesRef,
        exec: () => {},
      };
      const mockNodesRef: NodesRef = {
        invoke: () => mockSelectorQuery,
        path: () => mockSelectorQuery,
        fields: () => mockSelectorQuery,
        animate: () => mockSelectorQuery,
        playAnimation: () => mockSelectorQuery,
        pauseAnimation: () => mockSelectorQuery,
        cancelAnimation: () => mockSelectorQuery,
        setNativeProps: () => mockSelectorQuery,
      };
      const fieldsParams: FieldsParams = {
        id: true,
        dataset: false,
        tag: true,
        index: false,
        class: true,
        attribute: false,
        query: true,
      };

      const fieldsData: FieldsData = {
        id: 'test-id',
        tag: 'div',
        dataset: { key: 'value' },
        index: 0,
        class: ['class1', 'class2'],
        attribute: { 'data-test': 'value' },
        query: mockSelectorQuery,
      };

      expect(fieldsParams).toHaveProperty('id');
      expect(fieldsParams).toHaveProperty('dataset');
      expect(fieldsParams).toHaveProperty('tag');
      expect(fieldsParams).toHaveProperty('index');
      expect(fieldsParams).toHaveProperty('class');
      expect(fieldsParams).toHaveProperty('attribute');
      expect(fieldsParams).toHaveProperty('query');

      expect(fieldsData).toHaveProperty('id');
      expect(fieldsData).toHaveProperty('tag');
      expect(fieldsData).toHaveProperty('dataset');
      expect(fieldsData).toHaveProperty('index');
      expect(fieldsData).toHaveProperty('class');
      expect(fieldsData).toHaveProperty('attribute');
    });
  });

  describe('NodesRef', () => {
    it('should have all required methods', () => {
      const mockSelectorQuery: SelectorQuery = {
        select: () => mockNodesRef,
        selectAll: () => mockNodesRef,
        selectRoot: () => mockNodesRef,
        exec: () => {},
      };

      const mockNodesRef: NodesRef = {
        invoke: () => mockSelectorQuery,
        path: () => mockSelectorQuery,
        fields: () => mockSelectorQuery,
        animate: () => mockSelectorQuery,
        playAnimation: () => mockSelectorQuery,
        pauseAnimation: () => mockSelectorQuery,
        cancelAnimation: () => mockSelectorQuery,
        setNativeProps: () => mockSelectorQuery,
      };

      it('should validate SelectorQuery interface completeness', () => {
        expectTypeOf<SelectorQuery>().toHaveProperty('select');
        expectTypeOf<SelectorQuery>().toHaveProperty('selectAll');
        expectTypeOf<SelectorQuery>().toHaveProperty('selectRoot');
        expectTypeOf<SelectorQuery>().toHaveProperty('exec');

        expectTypeOf(mockSelectorQuery.select('')).toMatchTypeOf<NodesRef>();
        expectTypeOf(mockSelectorQuery.selectAll('')).toMatchTypeOf<NodesRef>();
        expectTypeOf(mockSelectorQuery.selectRoot()).toMatchTypeOf<NodesRef>();
        expectTypeOf(mockSelectorQuery.exec()).toBeVoid();
      });

      it('should validate NodesRef methods return SelectorQuery', () => {
        expectTypeOf(mockNodesRef.invoke({ method: '' })).toMatchTypeOf<SelectorQuery>();
        expectTypeOf(mockNodesRef.path(() => {})).toMatchTypeOf<SelectorQuery>();
        expectTypeOf(mockNodesRef.fields({ id: true }, () => {})).toMatchTypeOf<SelectorQuery>();
      });
    });
  });

  describe('SelectorQuery', () => {
    it('should have all required methods', () => {
      const selectorQuery: SelectorQuery = {
        select: (selector) => ({} as NodesRef),
        selectAll: (selector) => ({} as NodesRef),
        selectRoot: () => ({} as NodesRef),
        exec: () => {},
      };

      expect(selectorQuery).toHaveProperty('select');
      expect(selectorQuery).toHaveProperty('selectAll');
      expect(selectorQuery).toHaveProperty('selectRoot');
      expect(selectorQuery).toHaveProperty('exec');
    });
  });
});
