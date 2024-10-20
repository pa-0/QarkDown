#!/bin/bash

set -euo pipefail

TRUE=0
FALSE=1

QTVERSION="6.8.0"
QT_SDK_PATH="${HOME}/Qt"

echo "Remember to create a _fresh_ release build in Qt Creator first!"
echo "Assuming Qt SDK is at: ${QT_SDK_PATH}"
echo "Using Qt version: ${QTVERSION}"
echo "Press return to continue."
read

find_qarkdown_app()
{
    find . -name 'qarkdown.app' | grep '\-Release' | grep "${1//./_}" | head -n 1
}
find_macdeployqt()
{
    find "$1" -name macdeployqt | head -n 1
}

APPBUNDLE=$(find_qarkdown_app "$QTVERSION")
MACDEPLOYQT=$(find_macdeployqt "$QT_SDK_PATH")

if [ "$APPBUNDLE" == "" ];then
    echo "Error: cannot find a release build for given Qt version." >&2
    exit 1
fi
if [ "$MACDEPLOYQT" == "" ];then
    echo "Error: cannot find macdeployqt." >&2
    exit 1
fi

if [ -e "${APPBUNDLE}/Contents/Frameworks/QtCore.framework" ];then
    echo ">>>>>>> Looks like macdeployqt has already been run; skipping."
else
    echo ">>>>>>> Running macdeployqt"
    "${MACDEPLOYQT}" "$APPBUNDLE"

    echo ">>>>>>> Removing unused frameworks from app bundle"
    for framework in "$APPBUNDLE"/Contents/Frameworks/*.framework; do
        bn=$(basename "${framework}")
        bn=${bn%.*} # remove extension
        remove=${TRUE}
        for tokeep in "QtCore" "QtGui" "QtWidgets" "QtNetwork" "QtWebKit" \
                      "QtWebKitWidgets" "QtQuick" "QtQml" "QtOpenGL" \
                      "QtPrintSupport" "QtSql" "QtV8" "QtSensors" "QtDBus"; do
            [[ "${bn}" == "${tokeep}" ]] && remove=${FALSE}
        done
        if [[ $remove -eq ${TRUE} ]];then
            rm -rf "${framework}"
        fi
    done
fi

TEMP_DIR="QarkDown"
echo ">>>>>>> Making temp installation directory"
mkdir -p "$TEMP_DIR"

ROOTDIR=$(pwd)
BUNDLEBASENAME=$(basename "$APPBUNDLE")
BUNDLEDIRNAME=$(dirname "$APPBUNDLE")

echo ">>>>>>> Copying .app to temp installation directory"
cp -R "$APPBUNDLE" "$TEMP_DIR/QarkDown.app"

echo ">>>>>>> Copying other files to temp installation directory"
cp "README.md" "$TEMP_DIR/."
cp "LICENSE.md" "$TEMP_DIR/."

VERSION=$(./get_version.py)
ZIPFILE="QarkDown-macOS-${VERSION}.zip"
if [ -e "$ZIPFILE" ];then
    echo ">>>>>>> Deleting existing zip file: $ZIPFILE"
    rm -f "$ZIPFILE"
fi

echo ">>>>>>> Zipping app to: $ZIPFILE"
zip -r "${ZIPFILE}" "$TEMP_DIR"

echo ">>>>>>> Removing the temp installation directory"
rm -rf "$TEMP_DIR"

