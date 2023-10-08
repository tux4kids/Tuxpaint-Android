#!/bin/bash

set +x
set +e

srcdir=$( pwd )
builddir=$1
backend=$2

meson test -C ${builddir}

# Store the exit code for the CI run, but always
# generate the reports
exit_code=$?

cd ${builddir}

${srcdir}/.gitlab/scripts/meson-junit-report.py \
        --project-name=gdk-pixbuf \
        --job-id="${CI_JOB_NAME}" \
        --output=report-${CI_JOB_NAME}.xml \
        meson-logs/testlog.json

exit $exit_code
