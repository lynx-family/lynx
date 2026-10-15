# Legacy regexp compiler

This directory preserves the regexp compiler for the established 7-byte header
and opcode table. Only the template encoder links it. Regexp execution continues
to use the runtime's PrimJS engine.

Keeping the serialized format stable lets existing SDKs execute newly encoded
templates. Current runtimes recognize the legacy format and recompile the
serialized pattern and flags into their internal format.

The compiler is derived from PrimJS's MIT-licensed `libregexp.cc`; the matching
opcode table is kept alongside it. Execution code is omitted, and the compiler
entry point is namespaced to avoid colliding with the runtime engine. Character
range and Unicode helpers come from PrimJS.

Debug dumping and optional full Unicode property tables are omitted to match
the Encoder's existing build configuration.

Preserve the header, opcode numbers, instruction sizes, and flag bits when
maintaining this compiler. New regexp features require an explicit minimum SDK
contract before they can change the template encoding.
