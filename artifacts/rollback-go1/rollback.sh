#!/bin/sh
set -eu
cd "$(dirname "$0")"
exec sudo apt-get install --allow-downgrades ./network-manager_1.58.1-1+crquickshare1_amd64.deb ./libnm0_1.58.1-1+crquickshare1_amd64.deb ./libnm-dev_1.58.1-1+crquickshare1_amd64.deb ./gir1.2-nm-1.0_1.58.1-1+crquickshare1_amd64.deb ./network-manager-dev_1.58.1-1+crquickshare1_all.deb ./network-manager-tui_1.58.1-1+crquickshare1_amd64.deb
