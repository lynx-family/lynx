# url_parse

`url_parse.{h,cc}` are based on the same files in Chromium under
`//url/third_party/mozilla` but have been slightly modified for our use case.
`url_parse_internal.{h,cc}` contains additional functions needed by the former
files but aren't provided directly.  These are also ported from Chromium's
version.

## version and path

path: //chromium/src/third_party/openscreen/src/third_party/mozilla

commit: e50a1d65fa41a9864e900e60bd7a9c59dfe4bb78