#!/usr/bin/env bash
#
# enable_hugepages.sh  — Reserve and mount 2 MiB huge pages
#
# Usage: sudo ./enable_hugepages.sh <NUM_PAGES>
# Example: sudo ./enable_hugepages.sh 128   # reserves 256 MiB

set -euo pipefail

if [[ $EUID -ne 0 ]]; then
  echo "⚠️  Must run as root: sudo $0 <NUM_PAGES>"
  exit 1
fi

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <NUM_PAGES>"
  exit 1
fi

NUM_PAGES=$1

echo "1) Reserving $NUM_PAGES huge pages (2 MiB each)…"
echo "$NUM_PAGES" > /proc/sys/vm/nr_hugepages

echo
echo "2) Verifying pool:"
grep HugePages_Total /proc/meminfo
grep Hugepagesize    /proc/meminfo

echo
echo "3) Mounting hugetlbfs at /mnt/huge (if not already)…"
if ! mountpoint -q /mnt/huge; then
  mkdir -p /mnt/huge
  mount -t hugetlbfs none /mnt/huge
  echo "   Mounted hugetlbfs at /mnt/huge."
else
  echo "   /mnt/huge is already mounted."
fi

echo
echo "✅ Huge pages are now available. You can verify with:"
echo "   # cat /proc/meminfo | grep Huge"
echo "   # mount | grep hugetlbfs"
