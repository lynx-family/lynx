const fs = require('fs');
const path = require('path');

const IMPROPER_KEY_PATTERNS = [/^nested_value_\d+$/, /^unsupported_value_\d*$/];

const CODE_DESCRIPTION_PATTERN = /^<code>([^<]+)<\/code>$/;

function extractNameFromDescription(description) {
  const match = description.match(CODE_DESCRIPTION_PATTERN);
  if (match) {
    return match[1].trim();
  }
  return null;
}

function isImproperKey(key) {
  return IMPROPER_KEY_PATTERNS.some((pattern) => pattern.test(key));
}

function sanitizeKey(key) {
  return key
    .replace(/\s+/g, '-')
    .replace(/[^a-zA-Z_0-9\-$@]/g, '')
    .replace(/^-+|-+$/g, '');
}

function checkObject(obj, filePath, currentPath, issues) {
  for (const [key, value] of Object.entries(obj)) {
    if (key === '__compat') {
      continue;
    }

    if (typeof value === 'object' && value !== null) {
      const newPath = currentPath ? `${currentPath}.${key}` : key;

      if (isImproperKey(key)) {
        const compat = value.__compat;
        if (compat && typeof compat.description === 'string') {
          const suggestedKey = extractNameFromDescription(compat.description);
          if (suggestedKey) {
            const sanitized = sanitizeKey(suggestedKey);
            if (sanitized && sanitized !== key) {
              issues.push({
                file: filePath,
                path: newPath,
                currentKey: key,
                suggestedKey: sanitized,
                description: compat.description,
              });
            }
          }
        }
      }

      checkObject(value, filePath, newPath, issues);
    }
  }
}

function fixObject(obj) {
  let changed = false;
  const keys = Object.keys(obj);

  for (const key of keys) {
    if (key === '__compat') {
      continue;
    }

    const value = obj[key];
    if (typeof value === 'object' && value !== null) {
      if (isImproperKey(key)) {
        const compat = value.__compat;
        if (compat && typeof compat.description === 'string') {
          const suggestedKey = extractNameFromDescription(compat.description);
          if (suggestedKey) {
            const sanitized = sanitizeKey(suggestedKey);
            if (sanitized && sanitized !== key && !obj[sanitized]) {
              obj[sanitized] = value;
              delete obj[key];
              changed = true;
            }
          }
        }
      }

      if (fixObject(value)) {
        changed = true;
      }
    }
  }

  return changed;
}

function processFile(filePath, fix) {
  const content = fs.readFileSync(filePath, 'utf-8');
  const data = JSON.parse(content);
  const issues = [];

  if (data.compat_data && typeof data.compat_data === 'object') {
    checkObject(data.compat_data, filePath, 'compat_data', issues);

    if (fix && issues.length > 0) {
      if (fixObject(data.compat_data)) {
        fs.writeFileSync(filePath, JSON.stringify(data, null, 2) + '\n');
      }
    }
  }

  return issues;
}

function processAllFiles(fix) {
  const results = [];
  const cssDefinesDir = path.join(__dirname, '..', 'css_defines');

  if (!fs.existsSync(cssDefinesDir)) {
    console.error(`Directory not found: ${cssDefinesDir}`);
    process.exit(1);
  }

  const files = fs.readdirSync(cssDefinesDir).filter((f) => f.endsWith('.json'));

  for (const file of files) {
    const filePath = path.join(cssDefinesDir, file);
    const issues = processFile(filePath, fix);
    if (issues.length > 0) {
      results.push({
        file: path.relative(path.join(__dirname, '..'), filePath),
        issues,
      });
    }
  }

  return results;
}

const args = process.argv.slice(2);
const fix = args.includes('--fix');

console.log(
  fix
    ? 'Checking and fixing improperly named keys in compat_data...'
    : 'Checking for improperly named keys in compat_data...',
);
console.log();

const results = processAllFiles(fix);

if (results.length === 0) {
  console.log('✅ No issues found!');
  process.exit(0);
}

let totalIssues = 0;
for (const result of results) {
  console.log(`📄 ${result.file}`);
  for (const issue of result.issues) {
    totalIssues++;
    if (fix) {
      console.log(
        `   ✅ Fixed: "${issue.currentKey}" → "${issue.suggestedKey}"`,
      );
    } else {
      console.log(
        `   ⚠️  "${issue.currentKey}" should be "${issue.suggestedKey}" (from: ${issue.description})`,
      );
    }
  }
  console.log();
}

console.log(`Total: ${totalIssues} issue(s) ${fix ? 'fixed' : 'found'}`);

if (!fix && totalIssues > 0) {
  console.log('\nRun with --fix to auto-fix these issues.');
}

process.exit(1);
