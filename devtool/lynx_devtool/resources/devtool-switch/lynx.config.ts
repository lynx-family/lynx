import { defineConfig } from "@lynx-js/rspeedy";
import { pluginQRCode } from "@lynx-js/qrcode-rsbuild-plugin";
import { pluginReactLynx } from "@lynx-js/react-rsbuild-plugin";
import os from "os";

function getLocalIP() {
  const nets = os.networkInterfaces();
  const ignoreKeywords = [
    "mullvad",
    "tun",
    "tap",
    "vpn",
    "utun",
    "vethernet",
    "wsl",
    "hyper-v",
    "virtual",
  ];
  let fallbackIp = null;

  for (const [iface, addrs] of Object.entries(nets)) {
    const ifaceLower = iface.toLowerCase();
    if (ignoreKeywords.some((keyword) => ifaceLower.includes(keyword))) {
      continue;
    }
    for (const addr of addrs || []) {
      if (addr.family === "IPv4" && !addr.internal) {
        return addr.address;
      }
    }
  }
  // Fallback: if none pass filter choose first Ipv4
  for (const addrs of Object.values(nets)) {
    for (const addr of addrs || []) {
      if (addr.family === "IPv4" && !addr.internal) {
        fallbackIp = addr.address;
        break;
      }
    }
    if (fallbackIp) break;
  }
  return fallbackIp || "127.0.0.1";
}

const localIP = getLocalIP();

export default defineConfig({
  server: {
    host: localIP,
    port: 3000,
  },
  plugins: [
    pluginQRCode({
      schema(url) {
        const u = new URL(url);
        u.hostname = localIP;
        // Append the fullscreen query parameter
        u.searchParams.set("fullscreen", "true");
        return u.toString();
      },
    }),
    pluginReactLynx(),
  ],
});
