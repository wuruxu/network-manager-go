# Autonomous Wi-Fi Direct GO: downstream NetworkManager 1.58.1

This independent source tree is based on Debian network-manager 1.58.1-1,
including its four distribution patches. It is a development implementation.
No Python helper is used by the implementation. Debian packages have been built
with autoninja and installed for local hardware validation. See VALIDATION.md
for the tested scope and remaining limitations.

## Source provenance

The Debian source description was fetched with `apt-get source --download-only
network-manager=1.58.1-1` and extracted using `dpkg-source -x`. Archive SHA-256:

- upstream tarball: `50f0beefa02ff16159177e88379d7819edf6527b2cc25639ab4e62d8a15ca127`
- Debian tarball: `4580cb366a62c68d04df14a23cf4d4deaddff4187bfb299dc24f209429e16da4`

The initial Git commit records the unmodified, Debian-patched baseline.
Downstream package version: `1.58.1-1+crquickshare2`. New exported symbols use
`libnm_crquickshare_go_1`, not an upstream ABI version namespace.

## Connection API

Use the standard `org.freedesktop.NetworkManager.AddAndActivateConnection2`
method, targeting the NM **P2P device**, not the original station device.
Select its path by DeviceType (30) and its relationship to the desired radio;
do not hardcode the numeric object path. The example keyfile uses the commonly
exposed `p2p-dev-wlp3s0` name; confirm the actual name on the target system.

The connection dictionary includes:

| Setting | Property | D-Bus type | Value |
| --- | --- | --- | --- |
| connection | type | s | wifi-p2p |
| connection | id | s | Quick Share GO |
| connection | autoconnect | b | false |
| wifi-p2p | autonomous | b | true |
| wifi-p2p | frequency | u | 0 or a valid frequency in MHz |
| ipv4 | method | s | shared |
| ipv4 | never-default | b | true |
| ipv6 | method | s | link-local |
| ipv6 | never-default | b | true |

Do not set `wifi-p2p.peer` or supply a peer-specific object. Restrict
`connection.permissions` to the application user's account when credentials
must not be accessible to other authorized users. Use AddAndActivateConnection2
options `persist="volatile"` and `bind-activation="dbus-client"` from a
**long-lived application D-Bus connection**. A one-shot `gdbus` command with
client-bound activation will lose its connection immediately and deactivate it.

Activation uses NM's existing authorization, active connection objects and
state changes. Wait for ACTIVATED before requesting credentials or publishing
a Quick Share upgrade path. Read the GO's actual addresses from the P2P
device's Ip4Config/Ip6Config and its interface from IpInterface. Bind the
application's TCP listener before sending credentials to the phone. Do not
assume a fixed GO subnet or port.

Deactivate with the normal `DeactivateConnection(active_connection)` method.
The existing NM P2P disconnect, IP teardown and shared DHCP lifecycle are used.
No public ad-hoc Create/Delete daemon API or separate privileged service is added.

## New configuration properties

`wifi-p2p.autonomous` (boolean, default false) selects a non-persistent,
peer-independent GO. False preserves the peer connection path. Autonomous
connections remain active without clients. This extension supports only the
wpa_supplicant backend; IWD rejects such profiles explicitly.

`wifi-p2p.frequency` (uint32, default 0) is valid for autonomous groups and credential clients.
Zero uses the currently connected station's frequency when available. An
explicit conflicting frequency, or inability to determine the frequency of a
connected station, fails activation. With no associated station, zero delegates
frequency choice to supplicant. Regulatory and driver constraints still apply.
A separate group interface is required; NoGroupIface configurations are rejected.

Both properties are available through libnm GObject properties, D-Bus settings,
keyfiles and the patched nmcli property parser. Stock nmcli/libnm may reject
these downstream properties; use matching downstream clients.

## Credential API

`org.freedesktop.NetworkManager.Device.WifiP2P.GetGroupCredentials()` returns
`a{sv}` with:

- `ssid`: `ay`, raw SSID bytes (not necessarily UTF-8).
- `passphrase`: `s`, the generated WPA passphrase. Do not log it.
- `frequency`: `u`, actual MHz.
- `interface`: `s`, actual GO interface name.

This method requires the connection ACL and existing `network-control` Polkit
permission. Authorization does not grant arbitrary supplicant access. It only
returns credentials for an active autonomous group. If the group changes while
authorization is pending, it returns NotActive. Credentials are not published as
D-Bus properties/signals, persisted in the profile, or written to diagnostic logs.
Cached passphrases are cleared when the group path changes or the interface ends.

The patched libnm exposes async `nm_device_wifi_p2p_get_group_credentials()` and
its finish function. Integrators can also use the D-Bus method directly and
handle UnknownMethod on an unpatched daemon.

## Network behavior and validation limits

IPv4 shared mode uses NM's existing address selection and DHCP implementation;
it also follows NM's normal sharing/forwarding/firewall behavior. It is not a
new DHCP-only, no-NAT mode. If local-only sharing is required, that needs a
separate NM networking policy design. IPv6 link-local avoids IPv6 sharing.

The application does not change station Managed state. Same-channel selection
reduces radio contention but cannot guarantee that every driver preserves the
station connection. No capability claim substitutes for hardware validation.

Required manual acceptance after an explicitly authorized build/deployment:

1. Activate an autonomous profile without a peer; verify GO survives with no
   clients and the original STA association remains active.
2. Read credentials through an authorized client; verify denied ACLs fail and
   a deactivated/replaced group cannot return stale credentials.
3. Connect an Android phone; verify DHCP, TCP and the Quick Share encrypted
   upgrade barrier/heartbeat before transferring files.
4. Test normal deactivation, application exit with client binding, cancellation
   during creation, creation failure, repeated activation, rfkill and suspend.
   Verify no orphan group interfaces or shared DHCP processes remain.
5. Confirm regular negotiated P2P connections and normal Wi-Fi are unaffected.

The crQuickShare adapter has not been switched to these APIs in this change.
This directory implements the NM side only; the existing helper is neither
invoked nor required by it.

## Local Debian build and rollback

Build from this directory with depot_tools available:

```sh
AUTONINJA=/home/user/dev/depot_tools/autoninja DEB_BUILD_OPTIONS="parallel=8 nocheck" dpkg-buildpackage -b -us -uc
```

The build uses the upstream Meson configuration and autoninja's local Ninja
backend. `nocheck` skips the complete upstream suite; selected existing checks
are run separately and reported in VALIDATION.md. No new unit tests were added.
The local packaging rules supply multiarch library paths because this system's
ld.so.conf.d does not enable its multiarch configuration file.

The GO-only packages are in `artifacts/debs`; GO + Client packages are in `artifacts/debs-client`. Original Debian
1.58.1-1 packages for the six upgraded packages are in `artifacts/rollback`.
To restore them (NetworkManager may restart and interrupt connectivity):

```sh
cd artifacts/rollback
sudo apt-get install --allow-downgrades ./*.deb
```

Do not use `--no-download` with these local file arguments: the installed apt
version reports an internal pathname error with that combination. The rollback
packages themselves are local; review apt's proposed actions before accepting.

## Credential-based independent P2P client (crquickshare2)

Use AddAndActivateConnection2 on the same P2P management device with:

- wifi-p2p.client = true (boolean), autonomous = false, no peer or WPS.
- wifi-p2p.ssid = raw SSID bytes (ay, 1-32 bytes).
- wifi-p2p.psk = WPA passphrase (8-63 characters) or 64 hexadecimal PSK digits.
- wifi-p2p.psk-flags = 0 for credentials supplied with a volatile profile;
  the property is a normal NM secret and supports secret agents.
- wifi-p2p.frequency = the phone's advertised frequency, or zero for scanning.
- ipv4.method = auto, never-default = true, ignore-auto-dns = true,
  ignore-auto-routes = true; ipv6.method = link-local, never-default = true.
- connection.autoconnect = false; persist = volatile and bind-activation =
  dbus-client on a long-lived D-Bus connection, as for GO activation.

Wait for ACTIVATED and read IpInterface/Ip4Config before connecting to the phone's
advertised TCP endpoint. NM owns DHCP, addresses and interface teardown. The
station interface is not reconfigured. Network policy above prevents a phone's
DHCP server from replacing the existing default route or DNS configuration.

The supplicant backend temporarily creates an in-memory persistent-client
credential object, calls GroupAdd with that object, then removes the template.
No SaveConfig is called. The running independent P2P client keeps its own network
configuration. Deactivation cancels formation or disconnects the active group.
IWD does not implement this extension. Only one GO/client activation can occupy
the P2P device at once; release the local GO before following a phone GO path.

This adds the NM API only. crQuickShare must separately consume the offered
Direct path and activate this client profile. End-to-end phone interoperability
must be verified with current phone GO credentials.

The previously installed GO-only packages are retained in artifacts/rollback-go1:

```sh
sh artifacts/rollback-go1/rollback.sh
```
