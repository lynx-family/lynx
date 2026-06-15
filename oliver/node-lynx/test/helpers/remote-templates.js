const DEFAULT_HELLO_WORLD_TEMPLATE_URL =
  'https://lynxjs.org/lynx-examples/hello-world/dist/main.lynx.bundle';
const DEFAULT_GALLERY_TEMPLATE_URL =
  'https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle';

function envUrl(name, fallback) {
  const value = process.env[name];
  return value && value.trim() ? value : fallback;
}

function getHelloWorldTemplateUrl() {
  return envUrl(
    'NODE_LYNX_TEMPLATE_URL',
    envUrl('NODE_LYNX_REMOTE_TEMPLATE_URL', DEFAULT_HELLO_WORLD_TEMPLATE_URL)
  );
}

function getGalleryTemplateUrl() {
  return envUrl(
    'NODE_LYNX_GALLERY_TEMPLATE_URL',
    DEFAULT_GALLERY_TEMPLATE_URL
  );
}

module.exports = {
  DEFAULT_GALLERY_TEMPLATE_URL,
  DEFAULT_HELLO_WORLD_TEMPLATE_URL,
  getGalleryTemplateUrl,
  getHelloWorldTemplateUrl,
};
