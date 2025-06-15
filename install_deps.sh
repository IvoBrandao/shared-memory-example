#!/usr/bin/env bash
#
# install_ubuntu.sh — Install dependencies on Ubuntu for IPC capabilities
#

set -euo pipefail

if [[ $EUID -eq 0 ]]; then
  echo "Running as root, installing packages..."
else
  echo "Elevating to root with sudo..."
fi

sudo apt-get update
sudo apt-get install -y libcap2-bin

echo
echo "✅ Installed libcap2-bin. You can now use setcap to grant IPC capabilities."
