#include "identitydetails.h"

QString identityDiagnosticsHtml() {
    return QString::fromUtf8(R"KNOGN(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Knogn Identity Compatibility</title>
<style>
:root { color-scheme: light dark; font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
body { margin:0; background:Canvas; color:CanvasText; }
main { max-width:920px; margin:0 auto; padding:32px 24px 64px; }
h1 { margin:0 0 8px; font-size:30px; }
p { line-height:1.55; }
.note { padding:14px 16px; border:1px solid color-mix(in srgb, CanvasText 20%, transparent); border-radius:10px; margin:18px 0 24px; }
table { width:100%; border-collapse:collapse; }
th,td { text-align:left; padding:10px 12px; border-bottom:1px solid color-mix(in srgb, CanvasText 18%, transparent); vertical-align:top; }
th { font-weight:650; }
.ok { font-weight:700; }
.no { font-weight:700; }
code { font-family:ui-monospace, SFMono-Regular, Consolas, monospace; word-break:break-all; }
.small { opacity:.76; font-size:.92rem; }
</style>
</head>
<body>
<main>
<h1>Knogn Identity Compatibility</h1>
<p>This page reports identity APIs exposed by the exact browser engine currently running. It makes no external network requests and does not attempt to sign in to any provider.</p>
<div class="note">
<strong>Important:</strong> API availability does not guarantee that Google or another identity provider will accept this browser. Providers can independently reject embedded user-agents.
</div>
<table>
<thead><tr><th>Capability</th><th>Status</th><th>Notes</th></tr></thead>
<tbody id="results"></tbody>
</table>
<p class="small">Knogn keeps general third-party state blocked. The current compatibility policy permits only a narrow set of known identity-provider origins to use third-party state during normal web sign-in flows.</p>
</main>
<script>
(() => {
  const rows = [];
  const add = (name, ok, note='') => rows.push([name, !!ok, note]);

  add('Credentials Management API', !!navigator.credentials,
      'Required by WebAuthn and modern federated credential flows.');
  add('WebAuthn / passkeys', !!window.PublicKeyCredential,
      'Platform authenticator support still depends on OS and engine integration.');
  add('FedCM IdentityCredential surface', 'IdentityCredential' in window,
      'Google Identity Services increasingly relies on FedCM in Chromium-class browsers.');
  add('User-Agent Client Hints', !!navigator.userAgentData,
      'Used by modern sites to identify browser/platform capabilities.');
  add('Storage Access API', typeof document.requestStorageAccess === 'function',
      'Used by some cross-site sign-in and embedded account experiences.');
  add('Popup support', typeof window.open === 'function',
      'Knogn routes requested browser windows into normal tabs/windows.');

  const policy = document.featurePolicy || document.permissionsPolicy;
  let fedcmPolicy = 'not exposed';
  try {
    if (policy && typeof policy.allowsFeature === 'function') {
      fedcmPolicy = policy.allowsFeature('identity-credentials-get') ? 'allowed' : 'not allowed';
    }
  } catch (_) {}
  rows.push(['FedCM permissions policy', fedcmPolicy === 'allowed', fedcmPolicy]);

  const tbody = document.getElementById('results');
  for (const [name, ok, note] of rows) {
    const tr = document.createElement('tr');
    tr.innerHTML = `<td><strong>${name}</strong></td><td class="${ok ? 'ok' : 'no'}">${ok ? 'available' : 'unavailable'}</td><td>${note}</td>`;
    tbody.appendChild(tr);
  }

  const ua = document.createElement('tr');
  ua.innerHTML = `<td><strong>User agent</strong></td><td colspan="2"><code>${navigator.userAgent}</code></td>`;
  tbody.appendChild(ua);

  if (navigator.userAgentData) {
    const brands = document.createElement('tr');
    brands.innerHTML = `<td><strong>UA brands</strong></td><td colspan="2"><code>${JSON.stringify(navigator.userAgentData.brands || [])}</code></td>`;
    tbody.appendChild(brands);
  }
})();
</script>
</body>
</html>
)KNOGN");
}
