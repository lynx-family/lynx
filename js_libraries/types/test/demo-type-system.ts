/**
 * Demonstration of Lynx CSS Type System
 * 
 * This file showcases the gradual type strengthening system for inline styles.
 * Run with: tsc --noEmit demo-type-system.ts
 */

import { CSSProperties, StrictCSSProperties } from '../types';

// =============================================================================
// Example 1: Backward Compatibility (Loose Mode)
// =============================================================================

// ✅ This works - backward compatible with existing code
const looseExample1: CSSProperties = {
  position: 'absolute',
  display: 'flex',
  color: 'red',
};

// ✅ Custom CSS values work in loose mode
const looseExample2: CSSProperties = {
  position: 'static', // Even though not in Lynx enums
  display: 'inline-block', // Custom values allowed
  overflow: 'scroll', // Any overflow value
};

// ✅ Lynx-specific properties remain strict even in loose mode
const looseExample3: CSSProperties = {
  linearGravity: 'center',
  linearWeight: 1,
  linearOrientation: 'horizontal',
};

// ❌ This will error - Lynx-specific properties are always strict
const looseExample4: CSSProperties = {
  // @ts-expect-error: linearGravity only accepts specific enum values
  linearGravity: 'invalid',
};

// =============================================================================
// Example 2: Strict Mode for Type Safety
// =============================================================================

// ✅ Known enum values work in strict mode
const strictExample1: StrictCSSProperties = {
  position: 'absolute',
  display: 'flex',
  visibility: 'visible',
};

// ❌ Custom values are rejected in strict mode
const strictExample2: StrictCSSProperties = {
  // @ts-expect-error: position 'static' not in Lynx enums
  position: 'static',
  // @ts-expect-error: display 'inline-block' not in Lynx enums
  display: 'inline-block',
};

// ✅ Lynx-specific properties work the same in strict mode
const strictExample3: StrictCSSProperties = {
  linearGravity: 'center',
  linearWeight: 1,
};

// =============================================================================
// Example 3: Mixed Use Cases
// =============================================================================

// Scenario: Legacy component with loose typing
function LegacyComponent() {
  const style: CSSProperties = {
    // Can use any CSS values for backward compatibility
    position: 'sticky',
    display: 'flex',
    overflow: 'auto',
  };
  
  return { style };
}

// Scenario: New component with strict typing
function NewComponent() {
  const style: StrictCSSProperties = {
    // Only Lynx-supported values allowed
    position: 'absolute',
    display: 'grid',
    visibility: 'visible',
  };
  
  return { style };
}

// =============================================================================
// Example 4: Dynamic Styles
// =============================================================================

// Loose mode allows flexibility for dynamic styles
function createDynamicStyle(theme: any): CSSProperties {
  return {
    backgroundColor: theme.primaryColor, // Any color value works
    display: theme.isGrid ? 'grid' : 'flex',
    position: theme.isFixed ? 'fixed' : 'relative',
  };
}

// Strict mode ensures type safety for known values
type DisplayValue = 'none' | 'flex' | 'grid' | 'linear';

function createStrictLayout(display: DisplayValue): StrictCSSProperties {
  return {
    display, // Type-checked at compile time
    position: 'relative',
    linearGravity: 'center',
  };
}

// =============================================================================
// Example 5: Standard CSS + Lynx-Specific Properties
// =============================================================================

// Combining standard CSS with Lynx-specific properties
const combinedStyles: CSSProperties = {
  // Standard CSS
  color: 'red',
  fontSize: '16px',
  padding: '10px 20px',
  
  // Lynx-specific layout
  display: 'linear',
  linearGravity: 'center',
  linearWeight: 1,
  linearOrientation: 'vertical',
  
  // Position
  position: 'absolute',
  top: '10px',
  left: '20px',
};

// =============================================================================
// Example 6: Utility Type Usage
// =============================================================================

import { CSSPropertiesWithShorthands, CSSPropertiesWithLonghands } from '../types';

// Only shorthand properties
const shorthands: CSSPropertiesWithShorthands = {
  border: '1px solid red',
  padding: '10px',
  margin: '5px',
  outline: '2px solid blue',
  borderLeft: '1px solid red', // borderLeft is a shorthand
};

// Only longhand properties
const longhands: CSSPropertiesWithLonghands = {
  borderLeftColor: 'red', // Use longhand properties
  paddingTop: '10px',
  marginBottom: '5px',
  outlineColor: 'blue',
};

// =============================================================================
// Example 7: Gradual Migration Pattern
// =============================================================================

// Step 1: Start with loose mode (no breaking changes)
const step1_loose: CSSProperties = {
  position: 'sticky',
  display: 'flex',
};

// Step 2: Test with strict mode on new components
const step2_strict: StrictCSSProperties = {
  position: 'absolute',
  display: 'grid',
};

// Step 3: Both can coexist in the same codebase
interface ComponentStyles {
  legacy: CSSProperties;
  modern: StrictCSSProperties;
}

const componentStyles: ComponentStyles = {
  legacy: {
    position: 'sticky',
    display: 'flex',
  },
  modern: {
    position: 'absolute',
    display: 'grid',
  },
};

// =============================================================================
// Example 8: Real-World Component Example
// =============================================================================

interface NavbarProps {
  isFixed?: boolean;
  theme?: 'light' | 'dark';
}

function createNavbarStyles(props: NavbarProps): CSSProperties {
  const { isFixed = false, theme = 'light' } = props;
  
  return {
    // Layout
    display: 'flex',
    flexDirection: 'row',
    position: isFixed ? 'fixed' : 'relative',
    
    // Styling
    backgroundColor: theme === 'light' ? '#ffffff' : '#000000',
    color: theme === 'light' ? '#000000' : '#ffffff',
    padding: '10px 20px',
    
    // Lynx-specific
    linearGravity: 'space-between',
    
    // Dimensions
    height: '60px',
    width: '100%',
  };
}

// =============================================================================
// Example 9: Error Demonstrations
// =============================================================================

// Demonstrate what errors look like

// ❌ Error: Lynx-specific property with invalid value (both modes)
const error1: CSSProperties = {
  // @ts-expect-error
  linearGravity: 'invalid-value',
};

// ❌ Error: Invalid value in strict mode
const error2: StrictCSSProperties = {
  // @ts-expect-error
  position: 'static', // Not in Lynx enum
};

// ✅ But this works in loose mode
const noError: CSSProperties = {
  position: 'static', // Allowed in loose mode
};

// =============================================================================
// Summary
// =============================================================================

/**
 * Key Takeaways:
 * 
 * 1. CSSProperties (default):
 *    - Backward compatible
 *    - Standard CSS properties accept broader types
 *    - Lynx-specific properties remain strict
 *    - Recommended for existing projects
 * 
 * 2. StrictCSSProperties:
 *    - Opt-in strict mode
 *    - All properties have strict enum types
 *    - Better type safety
 *    - Recommended for new projects
 * 
 * 3. Migration Strategy:
 *    - No breaking changes
 *    - Gradual adoption possible
 *    - Both modes can coexist
 */

export {
  looseExample1,
  looseExample2,
  looseExample3,
  strictExample1,
  strictExample3,
  combinedStyles,
  componentStyles,
  createNavbarStyles,
};
