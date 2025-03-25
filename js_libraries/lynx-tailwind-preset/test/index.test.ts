/// <reference types="jest" />
import preset from '../src/index';

describe('@lynx-js/tailwind-preset', () => {
  it('should have the correct theme configuration', () => {
    expect(preset.theme?.extend?.utilities).toBeDefined();
    expect(preset.theme?.extend?.utilities['.safe-top']).toBeDefined();
    expect(preset.theme?.extend?.utilities['.safe-bottom']).toBeDefined();
    expect(preset.theme?.extend?.utilities['.touch-feedback']).toBeDefined();
    expect(preset.theme?.extend?.utilities['.optimize-gradient']).toBeDefined();
  });

  it('should have the correct plugins', () => {
    expect(preset.plugins).toBeDefined();
    expect(preset.plugins?.length).toBe(3); // Component mapping, scroll behavior, and interactive states
  });

  it('should handle component mapping correctly', () => {
    const componentPlugin = preset.plugins?.[0];
    expect(componentPlugin).toBeDefined();
    expect(typeof componentPlugin).toBe('function');
  });

  it('should handle scroll behavior correctly', () => {
    const scrollPlugin = preset.plugins?.[1];
    expect(scrollPlugin).toBeDefined();
    expect(typeof scrollPlugin).toBe('function');
  });

  it('should handle interactive states correctly', () => {
    const statesPlugin = preset.plugins?.[2];
    expect(statesPlugin).toBeDefined();
    expect(typeof statesPlugin).toBe('function');
  });
}); 