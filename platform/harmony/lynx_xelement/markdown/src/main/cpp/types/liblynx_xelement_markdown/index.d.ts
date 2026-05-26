declare module "liblynx_xelement_markdown.so" {
  interface MarkdownNativeModule {
    initMarkdown(): void;
  }

  const markdown: MarkdownNativeModule;
  export default markdown;
}
