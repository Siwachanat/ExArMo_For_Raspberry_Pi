#!/bin/bash
# Pull the newest version from GitHub (only if this folder was cloned with git).
cd "$(dirname "$0")/.." || exit 1
git pull --ff-only && echo "Updated. Restart ExArMo Touch."
