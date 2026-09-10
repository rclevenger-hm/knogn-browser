from pathlib import Path

root = Path(__file__).resolve().parents[1]
settings = (root / "src/appsettings.cpp").read_text(encoding="utf-8")
profile = (root / "src/privacyprofile.cpp").read_text(encoding="utf-8")
identity = (root / "src/identitydetails.cpp").read_text(encoding="utf-8")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
manifest = (root / "packaging/windows/knogn.manifest.in").read_text(encoding="utf-8")

assert "VERSION 0.2.1" in cmake
assert 'privacy/allowFederatedIdentityState' in settings
assert 'privacy/allowFederatedIdentityState"), true' in settings

# General third-party state must remain blocked; the compatibility exception is
# narrow and provider-owned rather than turning tracking state back on globally.
assert "if (!blockThirdParty || !request.thirdParty) return true;" in profile
assert "isFederatedIdentityOrigin(request.origin)" in profile
for provider in [
    "accounts.google.com",
    "login.microsoftonline.com",
    "appleid.apple.com",
    "github.com",
]:
    assert provider in profile, f"missing identity provider compatibility host: {provider}"

# Do not spoof Chrome/Firefox simply to evade an identity provider's embedded-
# browser policy. The product should solve this at the engine boundary.
assert "setHttpUserAgent" not in profile

assert "IdentityCredential" in identity
assert "PublicKeyCredential" in identity
assert "requestStorageAccess" in identity
assert "identity-credentials-get" in identity

# Qt documents that without a Windows compatibility manifest its default UA
# reports an old Windows generation. Keep the modern supportedOS declaration.
assert "8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a" in manifest
assert "knogn.manifest.in" in cmake

print("identity compatibility contract: pass")
