#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SRC_DIR="${SCRIPT_DIR}/../.."
export GIT_WORK_TREE="${SRC_DIR}"

if ! [ -x "./python" -a -e "Modules/Setup" ]; then
    echo 1>&2 "Error: This script should be invoked from a cpython build directory"
    exit 1
fi

# Sanity check: Should have no changes yet.
CHANGED_FILES="$(git status --porcelain --untracked-files=no)"
if [ "$CHANGED_FILES" != "" ]; then
    echo 1>&2 "Error: Sanity check failed: Checkout already has changed files:"
    echo 1>&2 "${CHANGED_FILES}"
    exit 1
fi

set -x
# Run clinic manually with -f switch.
./python ${SCRIPT_DIR}/../../Tools/clinic/clinic.py --make -f --srcdir "${SRC_DIR}"
make regen-all
set +x

CHANGED_FILES="$(git status --porcelain --untracked-files=no)"
if [ "${CHANGED_FILES}" != "" ]; then
    cat << __EOM__ >&2


Error: Changed files detected after \`make regen-all\`:
${CHANGED_FILES}

You can usually fix this with \`make regen-all\` and appending to your change!
__EOM__
    exit 1
fi
