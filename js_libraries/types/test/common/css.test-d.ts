import { assertType, describe, expectTypeOf, it } from 'vitest';
import { CSSProperties, CSSPropertiesWithLonghands, CSSPropertiesWithShorthands, StrictCSSProperties } from '../../types';

declare const css: CSSProperties;
declare const cssWithLonghands: CSSPropertiesWithLonghands;
declare const cssWithShorthands: CSSPropertiesWithShorthands;

describe('CSSProperty Type Test', () => {
  it('example', () => {
    assertType<CSSProperties>({
      // layout
      flexFlow: '1',
      marginInlineStart: '1px',
      marginInlineEnd: '1px',
      paddingInlineStart: '1px',
      paddingInlineEnd: '1px',
      // typography
      outline: '1px solid red',
      textDecoration: '1px',
      // visual
      border: '1px',
      borderRight: '1px',
      borderLeft: '1px',
      borderTop: '1px',
      borderBottom: '1px',
      // animation
      transition: '1px',
      transitionProperty: 'margin',
      transitionDuration: '1s',
      transitionDelay: '1s',
      transitionTimingFunction: 'ease-in',
      // other
      top: '1px',
      visibility: 'hidden',
      content: '1px',
      overflowX: 'hidden',
      overflowY: 'hidden',
      wordBreak: 'normal',
      verticalAlign: 'baseline',
      direction: 'normal',
      pointerEvents: 'auto',
    });

    assertType<CSSProperties>({
      pointerEvents: 'none',
    });

    // Loose mode (CSSProperties) accepts broader string types for backward compatibility
    assertType<CSSProperties>({
      pointerEvents: 'xxx', // This is allowed in loose mode
      position: 'static', // Even though not in Lynx enums, still allowed
      display: 'inline-block', // Custom values allowed
    });

    assertType<CSSPropertiesWithLonghands>({
      // layout
      marginInlineStart: '1px',
      marginInlineEnd: '1px',
      paddingInlineStart: '1px',
      paddingInlineEnd: '1px',
      // typography
      outlineColor: 'red',
      // visual
      borderBottomLeftRadius: '1px',
      // animation
      transitionProperty: 'margin',
      transitionDuration: '1s',
      transitionDelay: '1s',
      transitionTimingFunction: 'ease-in',
      // other
      top: '1px',
      visibility: 'hidden',
      content: '1px',
      overflowX: 'hidden',
      overflowY: 'hidden',
      wordBreak: 'normal',
      verticalAlign: 'baseline',
      direction: 'normal',
      pointerEvents: 'auto',
    });

    assertType<CSSPropertiesWithShorthands>({
      // layout
      flexFlow: '1',
      // typography
      outline: '1px',
      textDecoration: '1px',
      // visual
      border: '1px',
      borderRight: '1px',
      borderLeft: '1px',
      borderTop: '1px',
      borderBottom: '1px',
      // animation
      transition: '1px',
      // other
      overflow: 'hidden',
    });
  });

  it('cssWithLonghands and cssWithShorthands should be assignable to CSSProperties', () => {
    assertType<CSSProperties>(cssWithLonghands);
    assertType<CSSProperties>(cssWithShorthands);

    assertType<CSSPropertiesWithLonghands>(css);

    assertType<CSSPropertiesWithShorthands>(css);
  });

  it('StrictCSSProperties should enforce strict enum types', () => {
    // Strict mode accepts known enum values
    assertType<StrictCSSProperties>({
      pointerEvents: 'auto',
      position: 'absolute',
      display: 'flex',
    });

    // Strict mode rejects unknown values
    assertType<StrictCSSProperties>({
      // @ts-expect-error: StrictCSSProperties only accepts known enum values
      pointerEvents: 'xxx',
    });

    assertType<StrictCSSProperties>({
      // @ts-expect-error: position 'static' not in Lynx enums
      position: 'static',
    });
  });

  it('Lynx-specific properties should have strict types in both modes', () => {
    // Lynx-specific properties are strict in both modes
    assertType<CSSProperties>({
      linearGravity: 'center',
      linearWeight: 1,
    });

    assertType<CSSProperties>({
      // @ts-expect-error: linearGravity only accepts specific enum values
      linearGravity: 'invalid',
    });

    // Same for strict mode
    assertType<StrictCSSProperties>({
      linearGravity: 'center',
      linearWeight: 1,
    });

    assertType<StrictCSSProperties>({
      // @ts-expect-error: linearGravity only accepts specific enum values
      linearGravity: 'invalid',
    });
  });

  it('CSSProperties should be compatible with csstype inheritance', () => {
    // Test that standard CSS properties work
    assertType<CSSProperties>({
      color: 'red',
      backgroundColor: '#fff',
      fontSize: '16px',
      padding: '10px 20px',
      margin: 'auto',
    });

    // Test that we can use numeric values from csstype
    assertType<CSSProperties>({
      opacity: 0.5,
      zIndex: 10,
      flexGrow: 1,
    });
  });

  it('should support gradual migration from loose to strict', () => {
    // Step 1: Use loose mode (default CSSProperties) for backward compatibility
    const looseStyles: CSSProperties = {
      position: 'sticky',
      display: 'flex',
      pointerEvents: 'auto',
    };
    assertType<CSSProperties>(looseStyles);

    // Step 2: Gradually migrate to strict mode
    const strictStyles: StrictCSSProperties = {
      position: 'sticky',
      display: 'flex',
      pointerEvents: 'auto',
    };
    assertType<StrictCSSProperties>(strictStyles);

    // Step 3: Strict mode can be used where type safety is important
    assertType<StrictCSSProperties>({
      // All known values work
      position: 'relative',
      display: 'grid',
      visibility: 'visible',
    });
  });
});
