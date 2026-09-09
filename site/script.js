const repo = "rclevenger-hm/knogn-browser";
const releaseUrl = `https://github.com/${repo}/releases/latest`;
const labels = {windows:"Windows x86-64",linux:"Linux x86-64",macos:"macOS Apple Silicon"};
function assetKind(name){if(name.endsWith(".exe"))return"windows";if(name.endsWith(".deb"))return"linux";if(name.endsWith(".dmg"))return"macos";return null}
async function hydrateRelease(){const version=document.querySelector("[data-version]");const buttons=[...document.querySelectorAll("[data-os]")];buttons.forEach(b=>b.href=releaseUrl);try{const r=await fetch(`https://api.github.com/repos/${repo}/releases/latest`,{headers:{Accept:"application/vnd.github+json"}});if(!r.ok)throw new Error("release fetch failed");const release=await r.json();if(version)version.textContent=release.name||release.tag_name;for(const asset of release.assets||[]){const kind=assetKind(asset.name);const button=buttons.find(b=>b.dataset.os===kind);if(button){button.href=asset.browser_download_url;button.querySelector("span").textContent=`${labels[kind]} · ${asset.name}`}}}catch(_){if(version)version.textContent="Latest release"}}
hydrateRelease();
