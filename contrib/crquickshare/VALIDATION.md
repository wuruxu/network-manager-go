# P2P Client update: 2026-10-15

Installed version: **1.58.1-1+crquickshare2**, built with autoninja through
Debian packaging. All six previously installed components were upgraded:
network-manager, network-manager-tui, network-manager-dev, libnm0, libnm-dev,
gir1.2-nm-1.0. dpkg reports ii for all six; dpkg --audit is empty. The service
is active. Chromium/crFiles was neither changed nor built in this update.

Seven selected existing checks passed (exports, three metadata/docs checks,
supplicant configuration, setting validation, keyfiles). No unit tests added.
The complete upstream suite was not run. A preliminary check ran before the
clean build's test artifacts existed; final checks were run after those targets
and the libnm metadata target were built.

## Client hardware validation

A memory-only profile with a nonexistent test SSID and test PSK was activated
through nmcli and NetworkManager's normal activation API:

- NM accepted wifi-p2p.client/ssid/psk/frequency; ordinary profile output hid PSK.
- Supplicant created p2p-wlp3s0-0 (ifindex 348), kernel type P2P-client.
- NM remained in CONFIG while searching for the deliberately absent group.
- wlp3s0 remained connected on 2462 MHz. Its address and default route remained
  10.46.45.79/24 and via 10.46.45.16 with metric 600.
- Explicit deactivation removed the independent client interface; the test
  connection was deleted. Supplicant PersistentGroups was empty (ao 0).

This verifies the credential API, independent interface creation and cancellation
cleanup. It does **not** verify association/authentication to a real phone GO,
DHCP lease acquisition, TCP connectivity or Quick Share transfer interoperability.
Those require live phone credentials and the separate crQuickShare client adapter.
Raw 64-digit PSK, secret-agent interaction, timeout/rfkill/suspend and very early
activation cancellation have not been hardware-tested.

## GO regression

After the client test, an autonomous GO activated successfully on
p2p-wlp3s0-1 with 10.42.0.1/24 and shared DHCP. GetGroupCredentials returned all
four fields; uid 65534 was denied by the connection ACL. Deactivation removed
the interface and its dnsmasq process, and credential access then failed.
The original station stayed connected. No verification profiles remained.

## Packages and rollback

- artifacts/debs-client: version 2 debs, build metadata and SHA256SUMS.
- artifacts/rollback-go1: preserved version 1 debs and rollback.sh, which selects
  only the six installed packages instead of installing optional components.
- artifacts/logs/nm-client-*: packaging, checks, install and hardware observations.
- contrib/crquickshare/README.md: client configuration API and integration policy.

To return to the previous GO-only version (NM restarts):

```sh
sh /home/user/dev/chromium/src/network-manager-go/artifacts/rollback-go1/rollback.sh
```

---

The following is the historical GO-only validation record, not the scope of the
new client interoperability validation.

# Local validation: 2026-10-15

Source: Debian NetworkManager 1.58.1-1. Downstream packages:
1.58.1-1+crquickshare1, amd64. Built with autoninja (local Ninja backend) through
dpkg-buildpackage. The complete upstream suite was not run; no new unit tests
were written. Seven selected upstream checks passed:

- libnm symbol exports
- generated libnm-core setting metadata
- generated nmcli setting metadata
- generated setting descriptions
- supplicant configuration
- setting validation
- keyfile handling

## Hardware observations

The existing wlp3s0 station remained associated on 2462 MHz throughout all
three successful GO activation/deactivation cycles. The first GO remained
active with no clients for 30 seconds, followed by two 5-second cycles.

- Group interfaces: p2p-wlp3s0-0, p2p-wlp3s0-1, p2p-wlp3s0-2.
- Kernel interface type: P2P-GO; actual frequency: 2462 MHz.
- NM P2P device reached ACTIVATED (100).
- Shared IPv4 address: 10.42.0.1/24; dnsmasq range 10.42.0.10-10.42.0.254.
- GetGroupCredentials returned ssid, passphrase, frequency and interface.
  Password contents were not logged; only length was checked.
- With connection.permissions=user:root, uid 65534 was denied credentials.
- After deactivation, credential requests returned No active autonomous group.
- All three group interfaces and their NM-managed dnsmasq processes disappeared
  after deactivation. Test profiles were deleted. Existing libvirt/LXC dnsmasq
  processes were preserved.
- Final NetworkManager service state: active; original station: connected;
  P2P management device: disconnected. All six installed packages have dpkg
  state ii; dpkg --audit reports no pending configuration.

The initial installed development build failed before group creation because
NMSupplicantInterface.ifindex is a write-only GObject property. This was fixed
with an explicit internal getter, rebuilt and reinstalled. Packages currently
in artifacts/debs contain this fix. Intermediate failed builds are not release
artifacts. Runtime logs record the final successful validation.

## Not yet validated

No phone joined these test groups. DHCP server startup is verified, but a phone
obtaining a lease, TCP connectivity from the phone, and Quick Share transfer
interoperability are not verified. The crQuickShare adapter has not been changed
to consume this API. Client-bound activation, cancellation during creation,
rfkill, suspend/resume, and regular negotiated P2P regression testing remain.
This is a local development validation, not a general hardware compatibility
claim. Shared mode retains NM's normal forwarding/NAT policy.

## Artifacts and recovery

- artifacts/debs: generated Debian packages and build metadata.
- artifacts/rollback: official 1.58.1-1 packages for all six upgraded packages,
  verified against apt repository SHA256 metadata.
- artifacts/logs: build, selected checks, installation and redacted runtime logs.
- SHA256SUMS in each package directory records the final package contents.

Restore the original packages with:

```sh
cd /home/user/dev/chromium/src/network-manager-go/artifacts/rollback
sudo apt-get install --allow-downgrades ./*.deb
```

This may restart NetworkManager and briefly interrupt the network. Do not add
--no-download: this apt version rejects local-path installs with that option.
